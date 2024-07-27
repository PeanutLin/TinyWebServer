#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <functional>

class timerNode;

// 客户端数据
struct clientData {
  // 客户端地址数据
  sockaddr_in address;
  // 客户端在 epoll 的 fd
  int sockFd;
  // 客户端对应的 timer 节点
  timerNode* timer;
};

// timer 节点
class timerNode {
 public:
  timerNode() : prev(nullptr), next(nullptr) {}

 public:
  // 超时时间点
  time_t expire;

  // 回调函数
  std::function<void(clientData* userData)> callBackFunction;
  // timer 节点对应的客户端数据
  clientData* userData;

  // 链表节点指针
  timerNode *next, *prev;
};

class timerSortedList {
 public:
  // 构造函数
  timerSortedList();
  // 析构函数
  ~timerSortedList();

  // 添加 timer 到 sortedList 中 first 的后面
  void addTimer(timerNode* first, timerNode* timer);
  // 调整 timer 在 sortedList 上的位置，时间调整是单向的
  void adjustTimer(timerNode* timer);
  // 在 sorted 删除 timer
  void deleteTimer(timerNode* timer);
  // 超时处理，将 curTime（包含）前的 Timer 清除
  void tick(time_t curTime);

  // 虚拟链表头尾节点
  timerNode *head, *tail;
  int timerCount;
};
