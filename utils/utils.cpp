#include "utils.h"

int* Utils::mPipefd = 0;
int Utils::mEpollfd = 0;

// 创建文件夹
bool UTILS::createDirectories(const std::string& path) {
  if (path.empty()) {
    return false;
  }

  int pos = 0, ret = 0;
  while ((pos = path.find_first_of("/\\", pos)) != std::string::npos) {
    std::string dir = path.substr(0, pos++);
    ret = mkdir(dir.c_str(), 0777);
    if (ret != 0 && errno != EEXIST) {
      return false;
    }
  }

  ret = mkdir(path.c_str(), 0777);
  return ret == 0 || errno == EEXIST;
}

// 对文件描述符设置非阻塞
int UTILS::setnonblocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

// 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void UTILS::addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
  epoll_event event;
  event.data.fd = fd;

  if (1 == TRIGMode)
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  else
    event.events = EPOLLIN | EPOLLRDHUP;

  if (one_shot)
    event.events |= EPOLLONESHOT;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  UTILS::setnonblocking(fd);
}

// 从内核时间表删除描述符
void UTILS::removefd(int epollfd, int fd) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
  close(fd);
}

// 将事件重置为 EPOLLONESHOT
void UTILS::modfd(int epollfd, int fd, int ev, int TRIGMode) {
  epoll_event event;
  event.data.fd = fd;

  if (1 == TRIGMode)
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
  else
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

  epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void Utils::init(int timeSlot) {
  mTimeSlot = timeSlot;
}

// 对文件描述符设置非阻塞
int Utils::setNonBlocking(int fd) {
  int old_option = fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_option);
  return old_option;
}

// 将内核事件表注册读事件，ET 模式，选择开启 EPOLLONESHOT
void Utils::addFd(int epollFd, int fd, bool oneShot, TRIGGER_MODE triMode) {
  epoll_event ev;
  ev.data.fd = fd;

  if (TRIGGER_MODE::ET == triMode) {
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  } else {
    ev.events = EPOLLIN | EPOLLRDHUP;
  }

  if (oneShot) {
    ev.events |= EPOLLONESHOT;
  }
  epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
  setNonBlocking(fd);
}

// 信号处理函数
void Utils::sigHandler(int sig) {
  // 为保证函数的可重入性，保留原来的 errno
  int tmpErrno = errno;
  int msg = sig;
  send(mPipefd[1], (char*)&msg, 1, 0);
  errno = tmpErrno;
}

// 设置信号函数
void Utils::addSig(int sig, void(handler)(int), bool restart) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  // 设置信号处理函数
  sa.sa_handler = handler;
  // 使被信号打断的系统调用自动重新发起
  if (restart) {
    sa.sa_flags |= SA_RESTART;
  }
  // 信号处理函数执行期间，临时屏蔽其他信号
  sigfillset(&sa.sa_mask);
  assert(sigaction(sig, &sa, nullptr) != -1);
}

void Utils::showError(int connfd, const char* info) {
  send(connfd, info, strlen(info), 0);
  close(connfd);
}

// 定时处理任务，重新定时以不断触发 SIGALRM 信号
void Utils::timerHandler() {
  mTimerList.tick(time(nullptr));
  // 重新触发定时
  alarm(mTimeSlot);
}
