#include "httpConn.h"

#include <mysql/mysql.h>

#include <fstream>

// 定义 http 响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form =
    "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form =
    "You do not have permission to get file form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form =
    "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form =
    "There was an unusual problem serving the request file.\n";

std::mutex sqlMutex;
std::map<std::string, std::string> users;

// 初始化全局静态变量
int httpConn::mEpollfd = -1;
std::atomic<int> httpConn::userCount(0);

// 初始化数据建库数据
void httpConn::initMysqlResult(std::shared_ptr<connPool>& connPool) {
  // 连接池中取一个连接
  MYSQL* mysql = nullptr;
  connRAII mysqlCon(&mysql, connPool);

  // 在 user 表中检索 username，passwd 数据，浏览器端输入
  if (mysql_query(mysql, "SELECT username,passwd FROM user")) {
    LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
  }

  // 从表中检索完整的结果集
  MYSQL_RES* result = mysql_store_result(mysql);

  // 返回结果集中的列数
  int num_fields = mysql_num_fields(result);

  // 返回所有字段结构的数组
  MYSQL_FIELD* fields = mysql_fetch_fields(result);

  // 从结果集中获取下一行，将对应的用户名和密码，存入map中
  while (MYSQL_ROW row = mysql_fetch_row(result)) {
    std::string temp1(row[0]);
    std::string temp2(row[1]);
    users[temp1] = temp2;
  }
}

// 关闭连接，关闭一个连接，客户总量减一
void httpConn::closeConn(bool realClose) {
  if (realClose && (mSockfd != -1)) {
    UTILS::removefd(mEpollfd, mSockfd);
    mSockfd = -1;
    userCount.fetch_sub(1);
  }
}

// 初始化连接,外部调用初始化套接字地址
void httpConn::init(int sockfd, const sockaddr_in& addr, char* assets,
                    int TRIGMode, int close_log, std::string user,
                    std::string passwd, std::string sqlname) {
  mSockfd = sockfd;
  mAddress = addr;

  UTILS::addfd(mEpollfd, sockfd, true, mTRIGMode);
  userCount.fetch_add(1);

  // 当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
  docRoot = assets;
  mTRIGMode = TRIGMode;

  strcpy(sqlUser, user.c_str());
  strcpy(sqlPasswd, passwd.c_str());
  strcpy(sqlName, sqlname.c_str());

  // 初始化新接受的连接
  init();
}

// 初始化新接受的连接
// check_state 默认为分析请求行状态
void httpConn::init() {
  mysql = NULL;
  bytesToSend = 0;
  bytesHaveSend = 0;
  mCheckState = CHECK_STATE_REQUESTLINE;
  mLinger = false;
  mMethod = GET;
  mURL = 0;
  mVersion = 0;
  mContentLength = 0;
  mHost = 0;
  mStartLine = 0;
  mCheckedIndex = 0;
  mReadIndex = 0;
  mWriteIndex = 0;
  cgi = 0;
  mState = REQUEST_STEATE::READ;
  timerFlag = 0;
  improv = 0;

  memset(mReadBuf, '\0', READ_BUFFER_SIZE);
  memset(mWriteBuf, '\0', WRITE_BUFFER_SIZE);
  memset(mRealFile, '\0', FILENAME_LEN);
}

// 从状态机，用于分析出一行内容
// 返回值为行的读取状态，有 LINE_OK,LINE_BAD,LINE_OPEN
httpConn::LINE_STATUS httpConn::parseLine() {
  char temp;
  for (; mCheckedIndex < mReadIndex; ++mCheckedIndex) {
    temp = mReadBuf[mCheckedIndex];
    if (temp == '\r') {
      if ((mCheckedIndex + 1) == mReadIndex)
        return LINE_OPEN;
      else if (mReadBuf[mCheckedIndex + 1] == '\n') {
        mReadBuf[mCheckedIndex++] = '\0';
        mReadBuf[mCheckedIndex++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    } else if (temp == '\n') {
      if (mCheckedIndex > 1 && mReadBuf[mCheckedIndex - 1] == '\r') {
        mReadBuf[mCheckedIndex - 1] = '\0';
        mReadBuf[mCheckedIndex++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
  }
  return LINE_OPEN;
}

// 循环读取客户数据，直到无数据可读或对方关闭连接
// 非阻塞ET工作模式下，需要一次性将数据读完
bool httpConn::readHTTPRequest() {
  // 缓冲区已满
  if (mReadIndex >= READ_BUFFER_SIZE) {
    return false;
  }

  int bytes_read = 0;
  // LT读取数据
  if (TRIGGER_MODE::LT == mTRIGMode) {
    bytes_read =
        recv(mSockfd, mReadBuf + mReadIndex, READ_BUFFER_SIZE - mReadIndex, 0);
    mReadIndex += bytes_read;

    if (bytes_read <= 0) {
      return false;
    }

    return true;
  }
  // ET读数据
  else {
    while (true) {
      bytes_read = recv(mSockfd, mReadBuf + mReadIndex,
                        READ_BUFFER_SIZE - mReadIndex, 0);
      if (bytes_read == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          break;
        }
        return false;
      } else if (bytes_read == 0) {
        return false;
      }
      mReadIndex += bytes_read;
    }
    return true;
  }
}

// 解析 http 请求行，获得请求方法，目标 url 及 http 版本号
httpConn::HTTP_CODE httpConn::parseRequestLine(char* text) {
  mURL = strpbrk(text, " \t");
  if (!mURL) {
    return BAD_REQUEST;
  }
  *mURL++ = '\0';
  char* method = text;
  if (strcasecmp(method, "GET") == 0)
    mMethod = GET;
  else if (strcasecmp(method, "POST") == 0) {
    mMethod = POST;
    cgi = 1;
  } else
    return BAD_REQUEST;
  mURL += strspn(mURL, " \t");
  mVersion = strpbrk(mURL, " \t");
  if (!mVersion)
    return BAD_REQUEST;
  *mVersion++ = '\0';
  mVersion += strspn(mVersion, " \t");
  if (strcasecmp(mVersion, "HTTP/1.1") != 0)
    return BAD_REQUEST;
  if (strncasecmp(mURL, "http://", 7) == 0) {
    mURL += 7;
    mURL = strchr(mURL, '/');
  }

  if (strncasecmp(mURL, "https://", 8) == 0) {
    mURL += 8;
    mURL = strchr(mURL, '/');
  }

  if (!mURL || mURL[0] != '/')
    return BAD_REQUEST;
  // 当url为/时，显示判断界面
  if (strlen(mURL) == 1)
    strcat(mURL, "judge.html");
  mCheckState = CHECK_STATE_HEADER;
  return NO_REQUEST;
}

// 解析http请求的一个头部信息
httpConn::HTTP_CODE httpConn::parseHeaders(char* text) {
  if (text[0] == '\0') {
    if (mContentLength != 0) {
      mCheckState = CHECK_STATE_CONTENT;
      return NO_REQUEST;
    }
    return GET_REQUEST;
  } else if (strncasecmp(text, "Connection:", 11) == 0) {
    text += 11;
    text += strspn(text, " \t");
    if (strcasecmp(text, "keep-alive") == 0) {
      mLinger = true;
    }
  } else if (strncasecmp(text, "Content-length:", 15) == 0) {
    text += 15;
    text += strspn(text, " \t");
    mContentLength = atol(text);
  } else if (strncasecmp(text, "Host:", 5) == 0) {
    text += 5;
    text += strspn(text, " \t");
    mHost = text;
  } else {
    LOG_INFO("oop!unknow header: %s", text);
  }
  return NO_REQUEST;
}

// 判断 http 请求是否被完整读入
httpConn::HTTP_CODE httpConn::parseContent(char* text) {
  if (mReadIndex >= (mContentLength + mCheckedIndex)) {
    text[mContentLength] = '\0';
    // POST请求中最后为输入的用户名和密码
    mString = text;
    return GET_REQUEST;
  }
  return NO_REQUEST;
}

httpConn::HTTP_CODE httpConn::parseHTTPRequest() {
  LINE_STATUS line_status = LINE_OK;
  HTTP_CODE ret = NO_REQUEST;
  char* text = 0;

  while ((mCheckState == CHECK_STATE_CONTENT && line_status == LINE_OK) ||
         ((line_status = parseLine()) == LINE_OK)) {
    text = getLine();
    mStartLine = mCheckedIndex;
    LOG_INFO("%s", text);
    switch (mCheckState) {
      case CHECK_STATE_REQUESTLINE: {
        ret = parseRequestLine(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        }
        break;
      }
      case CHECK_STATE_HEADER: {
        ret = parseHeaders(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        } else if (ret == GET_REQUEST) {
          return doRequest();
        }
        break;
      }
      case CHECK_STATE_CONTENT: {
        ret = parseContent(text);
        if (ret == GET_REQUEST) {
          return doRequest();
        }
        line_status = LINE_OPEN;
        break;
      }
      default:
        return INTERNAL_ERROR;
    }
  }
  return NO_REQUEST;
}

// 解析 URL
httpConn::HTTP_CODE httpConn::doRequest() {
  strcpy(mRealFile, docRoot);
  int len = strlen(docRoot);
  const char* p = strrchr(mURL, '/');

  // 处理 cgi
  if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {
    // 根据标志判断是登录检测还是注册检测
    char flag = mURL[1];

    char* realURL = (char*)malloc(sizeof(char) * 200);
    strcpy(realURL, "/");
    strcat(realURL, mURL + 2);
    strncpy(mRealFile + len, realURL, FILENAME_LEN - len - 1);
    free(realURL);

    // 将用户名和密码提取出来（注意下标）
    // username=123&passwd=123
    char name[100], password[100];
    int i;
    std::cout << mString << std::endl;
    for (i = 9; mString[i] != '&'; ++i) {
      name[i - 9] = mString[i];
    }
    name[i] = '\0';

    int j = 0;
    for (i = i + 10; mString[i] != '\0'; ++i, ++j) {
      password[j] = mString[i];
    }
    password[j] = '\0';

    if (*(p + 1) == '3') {
      // 如果是注册，先检测数据库中是否有重名的
      // 没有重名的，进行增加数据
      char* sql_insert = (char*)malloc(sizeof(char) * 200);
      strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
      strcat(sql_insert, "'");
      strcat(sql_insert, name);
      strcat(sql_insert, "', '");
      strcat(sql_insert, password);
      strcat(sql_insert, "')");
      std::cout << name << std::endl;
      std::cout << sql_insert << std::endl;

      if (users.find(name) == users.end()) {
        // 正常注册
        sqlMutex.lock();
        int res = mysql_query(mysql, sql_insert);
        users.insert(std::pair<std::string, std::string>(name, password));
        sqlMutex.unlock();

        if (!res) {
          strcpy(mURL, "/log.html");
        } else {
          strcpy(mURL, "/registerError.html");
        }
      } else {
        // 数据库中有重名的
        strcpy(mURL, "/registerError.html");
      }
    }
    // 如果是登录，直接判断
    // 若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
    else if (*(p + 1) == '2') {
      if (users.find(name) != users.end() && users[name] == password)
        strcpy(mURL, "/welcome.html");
      else
        strcpy(mURL, "/logError.html");
    }
  }

  if (*(p + 1) == '0') {
    char* realURL = (char*)malloc(sizeof(char) * 200);
    strcpy(realURL, "/register.html");
    strncpy(mRealFile + len, realURL, strlen(realURL));

    free(realURL);
  } else if (*(p + 1) == '1') {
    char* realURL = (char*)malloc(sizeof(char) * 200);
    strcpy(realURL, "/log.html");
    strncpy(mRealFile + len, realURL, strlen(realURL));

    free(realURL);
  } else if (*(p + 1) == '5') {
    char* realURL = (char*)malloc(sizeof(char) * 200);
    strcpy(realURL, "/picture.html");
    strncpy(mRealFile + len, realURL, strlen(realURL));

    free(realURL);
  } else if (*(p + 1) == '6') {
    char* realURL = (char*)malloc(sizeof(char) * 200);
    strcpy(realURL, "/video.html");
    strncpy(mRealFile + len, realURL, strlen(realURL));

    free(realURL);
  } else if (*(p + 1) == '7') {
    char* realURL = (char*)malloc(sizeof(char) * 200);
    strcpy(realURL, "/fans.html");
    strncpy(mRealFile + len, realURL, strlen(realURL));

    free(realURL);
  } else {
    strncpy(mRealFile + len, mURL, FILENAME_LEN - len - 1);
  }

  // 通过 stat 得到文件的基本信息
  if (stat(mRealFile, &mFileStat) < 0) {
    return NO_RESOURCE;
  }

  if (!(mFileStat.st_mode & S_IROTH)) {
    return FORBIDDEN_REQUEST;
  }

  if (S_ISDIR(mFileStat.st_mode)) {
    return BAD_REQUEST;
  }

  int fd = open(mRealFile, O_RDONLY);

  // 内存映射得到文件数据
  mFileAddress =
      (char*)mmap(0, mFileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return FILE_REQUEST;
}

// 取消文件的内存映射
void httpConn::unmap() {
  if (mFileAddress) {
    munmap(mFileAddress, mFileStat.st_size);
    mFileAddress = nullptr;
  }
}

// 向客户端写数据
bool httpConn::writeHTTPResponse() {
  int temp = 0;

  // 没有数据传输，就退出
  if (bytesToSend == 0) {
    UTILS::modfd(mEpollfd, mSockfd, EPOLLIN, mTRIGMode);
    init();
    return true;
  }

  // 传输数据
  while (true) {
    temp = writev(mSockfd, mIV, mIVCount);

    // 错误处理
    if (temp < 0) {
      if (errno == EAGAIN) {
        UTILS::modfd(mEpollfd, mSockfd, EPOLLOUT, mTRIGMode);
        return true;
      }
      unmap();
      return false;
    }

    bytesHaveSend += temp;
    bytesToSend -= temp;
    if (bytesHaveSend >= mIV[0].iov_len) {
      mIV[0].iov_len = 0;
      mIV[1].iov_base = mFileAddress + (bytesHaveSend - mWriteIndex);
      mIV[1].iov_len = bytesToSend;
    } else {
      mIV[0].iov_base = mWriteBuf + bytesHaveSend;
      mIV[0].iov_len = mIV[0].iov_len - bytesHaveSend;
    }

    if (bytesToSend <= 0) {
      unmap();
      UTILS::modfd(mEpollfd, mSockfd, EPOLLIN, mTRIGMode);

      if (mLinger) {
        init();
        return true;
      } else {
        return false;
      }
    }
  }
}

bool httpConn::addResponse(const char* format, ...) {
  if (mWriteIndex >= WRITE_BUFFER_SIZE)
    return false;
  va_list arg_list;
  va_start(arg_list, format);
  int len = vsnprintf(mWriteBuf + mWriteIndex,
                      WRITE_BUFFER_SIZE - 1 - mWriteIndex, format, arg_list);
  if (len >= (WRITE_BUFFER_SIZE - 1 - mWriteIndex)) {
    va_end(arg_list);
    return false;
  }
  mWriteIndex += len;
  va_end(arg_list);

  LOG_INFO("request:%s", mWriteBuf);

  return true;
}

bool httpConn::addStatusLine(int status, const char* title) {
  return addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool httpConn::addHeaders(int content_len) {
  return addContentLength(content_len) && addLinger() && addBlankLine();
}

bool httpConn::addContentLength(int content_len) {
  return addResponse("Content-Length:%d\r\n", content_len);
}

bool httpConn::addContentType() {
  return addResponse("Content-Type:%s\r\n", "text/html");
}

bool httpConn::addLinger() {
  return addResponse("Connection:%s\r\n",
                     (mLinger == true) ? "keep-alive" : "close");
}

bool httpConn::addBlankLine() {
  return addResponse("%s", "\r\n");
}

bool httpConn::addContent(const char* content) {
  return addResponse("%s", content);
}

bool httpConn::prepareHTTPResponse(HTTP_CODE ret) {
  switch (ret) {
    case INTERNAL_ERROR: {
      addStatusLine(500, error_500_title);
      addHeaders(strlen(error_500_form));
      if (!addContent(error_500_form))
        return false;
      break;
    }
    case BAD_REQUEST: {
      addStatusLine(404, error_404_title);
      addHeaders(strlen(error_404_form));
      if (!addContent(error_404_form))
        return false;
      break;
    }
    case FORBIDDEN_REQUEST: {
      addStatusLine(403, error_403_title);
      addHeaders(strlen(error_403_form));
      if (!addContent(error_403_form))
        return false;
      break;
    }
    case FILE_REQUEST: {
      addStatusLine(200, ok_200_title);
      if (mFileStat.st_size != 0) {
        addHeaders(mFileStat.st_size);
        // 请求报头
        mIV[0].iov_base = mWriteBuf;
        mIV[0].iov_len = mWriteIndex;
        // 文件数据
        mIV[1].iov_base = mFileAddress;
        mIV[1].iov_len = mFileStat.st_size;
        // 总共两个文件
        mIVCount = 2;
        bytesToSend = mWriteIndex + mFileStat.st_size;
        return true;
      } else {
        const char* ok_string = "<html><body></body></html>";
        addHeaders(strlen(ok_string));
        if (!addContent(ok_string))
          return false;
      }
    }
    default:
      return false;
  }
  // 没有实际的文件请求，只返回简单的响应报文
  mIV[0].iov_base = mWriteBuf;
  mIV[0].iov_len = mWriteIndex;
  mIVCount = 1;
  bytesToSend = mWriteIndex;
  return true;
}

// 解析数据包
void httpConn::handleHTTPRequest() {
  HTTP_CODE readResult = parseHTTPRequest();
  if (readResult == NO_REQUEST) {
    UTILS::modfd(mEpollfd, mSockfd, EPOLLIN, mTRIGMode);
    return;
  }

  bool prepareResult = prepareHTTPResponse(readResult);
  if (!prepareResult) {
    // closeConn();
  }

  // 监听写请求
  UTILS::modfd(mEpollfd, mSockfd, EPOLLOUT, mTRIGMode);
}

sockaddr_in* httpConn::getAddress() {
  return &mAddress;
}

void callBackFunction(clientData* userData) {
  if (userData->sockFd == -1) {
    return;
  }
  epoll_ctl(Utils::mEpollfd, EPOLL_CTL_DEL, userData->sockFd, nullptr);
  close(userData->sockFd);
  httpConn::userCount.fetch_sub(1);
}