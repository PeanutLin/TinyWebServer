#include "webserver.h"

// 构造函数
WebServer::WebServer() {
  // 创建 httpConn 类对象
  users = new httpConn[MAX_FD];

  // assets
  char serverPath[200];
  getcwd(serverPath, 200);
  char root[20] = "/assets";
  assetsPath = (char*)malloc(strlen(serverPath) + strlen(root) + 1);
  strcpy(assetsPath, serverPath);
  strcat(assetsPath, root);

  // 定时器
  usersTimer = new clientData[MAX_FD];
}

// 析构函数
WebServer::~WebServer() {
  close(mEpollfd);
  close(mListenfd);
  close(mPipefd[1]);
  close(mPipefd[0]);
}

// 单例
WebServer* WebServer::getInstance() {
  static WebServer webServer;
  return &webServer;
}

// 服务器初始化
void WebServer::init() {
  // 初始化数据库连接池
  initSqlPool();
  // 初始化线程池
  initThreadPool();
  // 初始化监听服务器
  initEventListen();
  std::cout << "assetsPath: " << assetsPath << std::endl;
}

// 初始化数据库连接池
void WebServer::initSqlPool() {
  mConnPool = std::make_shared<connPool>();
  auto configPtr = Config::getInstance();
  mConnPool->init("localhost", configPtr->mysqlUser, configPtr->mysqlPasswd,
                  configPtr->databaseName, configPtr->mysqlPort,
                  configPtr->sqlNum);
  // 初始化数据库读取表
  users->initMysqlResult(mConnPool);
}

// 初始化线程池
void WebServer::initThreadPool() {
  auto configPtr = Config::getInstance();
  mPool = std::make_shared<threadPool<httpConn>>(
      configPtr->actorModel, mConnPool, configPtr->threadNum);
}

// 初始化监听服务器
void WebServer::initEventListen() {
  auto configPtr = Config::getInstance();
  // 创建监听的套接字
  mListenfd = socket(PF_INET, SOCK_STREAM, 0);
  if (mListenfd == -1) {
    perror("socket error");
    exit(1);
  }

  // 优雅关闭连接
  if (false == configPtr->gClosed) {
    struct linger tmp = {0, 1};
    setsockopt(mListenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
  } else {
    struct linger tmp = {1, 1};
    setsockopt(mListenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
  }

  // 绑定
  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  serverAddress.sin_port = htons(configPtr->Port);

  // 设置端口复用
  int flag = 1;
  setsockopt(mListenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

  // 绑定端口
  int ret =
      bind(mListenfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
  if (ret == -1) {
    perror("bind error");
    exit(1);
  }

  //监听
  ret = listen(mListenfd, 64);
  if (ret == -1) {
    perror("listen error");
    exit(1);
  }

  // 创建 epoll
  mEpollfd = epoll_create(100);
  if (mEpollfd == -1) {
    perror("epoll create");
    exit(0);
  }

  utils.init(TIMESLOT);

  // epoll 中添加服务器 listen socket
  utils.addFd(mEpollfd, mListenfd, false, configPtr->listenTrigmode);

  httpConn::mEpollfd = mEpollfd;

  ret = socketpair(PF_UNIX, SOCK_STREAM, 0, mPipefd);
  if (ret == -1) {
    perror("epoll_create");
    exit(0);
  }

  utils.setNonBlocking(mPipefd[1]);
  utils.addFd(mEpollfd, mPipefd[0], false, TRIGGER_MODE::LT);

  // 信号处理函数
  // 忽略对已关闭的 socket 或管道写产生的报错信号的护理
  utils.addSig(SIGPIPE, SIG_IGN);
  // 定时器信号处理
  utils.addSig(SIGALRM, utils.sigHandler, false);
  // 程序终止信号处理
  utils.addSig(SIGTERM, utils.sigHandler, false);

  // TIMESLOT 后产生时钟信号
  alarm(TIMESLOT);

  // 工具类, 信号和描述符基础操作
  Utils::mPipefd = mPipefd;
  Utils::mEpollfd = mEpollfd;
}

void WebServer::initConn(int connfd, struct sockaddr_in clientAddress) {
  auto configPtr = Config::getInstance();
  // 初始化连接
  users[connfd].init(connfd, clientAddress, assetsPath, configPtr->connTrigmode,
                     configPtr->isLogClosed, configPtr->mysqlUser,
                     configPtr->mysqlPasswd, configPtr->databaseName);

  // 初始化 client_data 数据
  // 创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
  usersTimer[connfd].address = clientAddress;
  usersTimer[connfd].sockFd = connfd;
  timerNode* timer = new timerNode;
  timer->userData = &usersTimer[connfd];

  // 设置回调函数
  timer->callBackFunction = callBackFunction;

  // 设置超时时间
  time_t cur = time(nullptr);
  timer->expire = cur + 3 * TIMESLOT;
  usersTimer[connfd].timer = timer;

  // 加入到时间轮
  utils.mTimerList.addTimer(utils.mTimerList.head, timer);
}

// 若有数据传输，则将定时器往后延迟3个单位
// 并对新的定时器在链表上的位置进行调整
void WebServer::adjustTimer(timerNode* timer) {
  time_t cur = time(nullptr);
  // 设置超时时间
  timer->expire = cur + 3 * TIMESLOT;
  utils.mTimerList.adjustTimer(timer);

  LOG_INFO("%s", "adjust timer once");
}

// 使用回调函数处理 timer
void WebServer::dealTimer(timerNode* timer, int sockfd) {
  timer->callBackFunction(&usersTimer[sockfd]);
  if (timer) {
    utils.mTimerList.deleteTimer(timer);
  }

  LOG_INFO("close fd %d", usersTimer[sockfd].sockFd);
}

// 处理客户端连接事件
bool WebServer::dealClientData() {
  auto configPtr = Config::getInstance();
  struct sockaddr_in clientAddress;
  socklen_t client_addrlength = sizeof(clientAddress);

  do {
    int connfd =
        accept(mListenfd, (struct sockaddr*)&clientAddress, &client_addrlength);
    if (connfd < 0) {
      LOG_ERROR("%s:errno is:%d", "accept error", errno);
      return false;
    }
    if (httpConn::userCount.load() >= MAX_FD) {
      utils.showError(connfd, "Internal server busy");
      LOG_ERROR("%s", "Internal server busy");
      return false;
    }
    // 初始化客户端连接数据
    initConn(connfd, clientAddress);
  } while (TRIGGER_MODE::ET == configPtr->listenTrigmode);
  return true;
}

bool WebServer::dealWithSignal(bool& checkTime, bool& stopServer) {
  int ret = 0;
  char signals[1024];
  ret = recv(mPipefd[0], signals, sizeof(signals), 0);
  if (ret == -1) {
    return false;
  } else if (ret == 0) {
    return false;
  } else {
    for (int i = 0; i < ret; ++i) {
      switch (signals[i]) {
        case SIGALRM: {
          checkTime = true;
          break;
        }
        case SIGTERM: {
          stopServer = true;
          break;
        }
      }
    }
  }
  return true;
}

// 处理读请求
void WebServer::dealWithRead(int sockfd) {
  timerNode* timer = usersTimer[sockfd].timer;
  auto configPtr = Config::getInstance();
  // Reactor 模式
  if (ACTOR_MODE::REACTOR == configPtr->actorModel) {
    if (timer) {
      adjustTimer(timer);
    }

    // 标记一个读事件，并加入请求队列
    mPool->append(users + sockfd, REQUEST_STEATE::READ);

    while (true) {
      if (1 == users[sockfd].improv) {
        if (1 == users[sockfd].timerFlag) {
          dealTimer(timer, sockfd);
          users[sockfd].timerFlag = 0;
        }
        users[sockfd].improv = 0;
        break;
      }
    }
  } else {
    // Proactor 模式
    if (users[sockfd].readOnce()) {
      LOG_INFO("deal with the client(%s)",
               inet_ntoa(users[sockfd].getAddress()->sin_addr));
      // 若监测到读事件，将该事件放入请求队列
      mPool->append(users + sockfd, REQUEST_STEATE::WRITE);
      if (timer) {
        adjustTimer(timer);
      }
    } else {
      dealTimer(timer, sockfd);
    }
  }
}

void WebServer::dealWithWrite(int sockfd) {
  auto configPtr = Config::getInstance();
  timerNode* timer = usersTimer[sockfd].timer;

  if (ACTOR_MODE::REACTOR == configPtr->actorModel) {
    if (timer) {
      adjustTimer(timer);
    }

    mPool->append(users + sockfd, REQUEST_STEATE::WRITE);

    // 新加入的请求进行处理，避免争夺
    while (true) {
      if (1 == users[sockfd].improv) {
        if (1 == users[sockfd].timerFlag) {
          dealTimer(timer, sockfd);
          users[sockfd].timerFlag = 0;
        }
        users[sockfd].improv = 0;
        break;
      }
    }
  } else if (ACTOR_MODE::PROACTOR == configPtr->actorModel) {
    if (users[sockfd].write()) {
      LOG_INFO("send data to the client(%s)",
               inet_ntoa(users[sockfd].getAddress()->sin_addr));

      if (timer) {
        adjustTimer(timer);
      }
    } else {
      dealTimer(timer, sockfd);
    }
  }
}

// 主事件循环
void WebServer::eventLoop() {
  bool checkTime = false;
  bool stopServer = false;

  while (!stopServer) {
    // 执行 Epoll 监听
    int number = epoll_wait(mEpollfd, events, MAX_EVENT_NUMBER, -1);
    if (number < 0 && errno != EINTR) {
      LOG_ERROR("%s", "epoll failure");
      break;
    }
    // 处理 epoll 监听事件
    for (int i = 0; i < number; i++) {
      // 获取响应的 fd
      int sockfd = events[i].data.fd;
      // 响应 fd 为服务器 Listend fd
      if (sockfd == mListenfd) {
        bool flag = dealClientData();
        if (false == flag) {
          continue;
        }
      } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        // 服务器端关闭连接，处理对应的 fd
        timerNode* timer = usersTimer[sockfd].timer;
        dealTimer(timer, sockfd);
      }
      // 处理父子进程之间信号
      else if ((sockfd == mPipefd[0]) && (events[i].events & EPOLLIN)) {
        bool flag = dealWithSignal(checkTime, stopServer);
        if (false == flag)
          LOG_ERROR("%s", "dealclientdata failure");
      }
      // 处理客户端连接上接收到的数据
      else if (events[i].events & EPOLLIN) {
        dealWithRead(sockfd);
      } else if (events[i].events & EPOLLOUT) {
        // 处理客户端写的数据
        dealWithWrite(sockfd);
      }
    }
    if (checkTime) {
      utils.timerHandler();
      LOG_INFO("%s", "timer tick");
      LOG_INFO("alive connection: %d", httpConn::userCount.load())
      checkTime = false;
    }
  }
}
