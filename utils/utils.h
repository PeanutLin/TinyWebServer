#pragma once
#include <errno.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <string>

#include "../timer/timerList.h"
#include "../types/types.h"

class Utils {
 public:
  static int* mPipefd;
  timerSortedList mTimerList;
  static int mEpollfd;
  int mTimeSlot;

 public:
  Utils() {}
  ~Utils() {}

  // 初始化
  void init(int timeslot);

  // 对文件描述符设置非阻塞
  int setNonBlocking(int fd);

  // 将内核事件表注册读事件，ET 模式，选择开启 EPOLLONESHOT
  void addFd(int epollFd, int fd, bool oneShot, TRIGGER_MODE triMode);

  // 设置信号函数
  void addSig(int sig, void(handler)(int), bool restart = true);

  // 定时处理任务，重新定时以不断触发 SIGALRM 信号
  void timerHandler();

  void showError(int connfd, const char* info);

  // 信号处理函数
  static void sigHandler(int sig);
};

namespace UTILS {
// 创建文件夹
bool createDirectories(const std::string& path);

// 对文件描述符设置非阻塞
int setnonblocking(int fd);

// 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

// 从内核时间表删除描述符
void removefd(int epollfd, int fd);

// 将事件重置为 EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int TRIGMode);
};  // namespace UTILS
