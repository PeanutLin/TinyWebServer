#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <memory>

#include "../config/config.h"
#include "../db/sqlConnPool.h"
#include "../http/httpConn.h"
#include "../threadPool/threadPool.h"
#include "../types/types.h"
#include "../utils/utils.h"

const int MAX_FD = 65536;            // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000;  // 最大事件数
const int TIMESLOT = 30;             // 最小超时单位

class WebServer {

 public:
  // Server 相关
  // 服务器监听套接字
  int mListenfd;
  // epoll 结构体套接字
  int mEpollfd;

  // 管道，用于主线程与子线程之间的通信
  // mPipefd[0]：读管道；mPipefd[1]：写管道
  int mPipefd[2];
  // epoll 事件
  epoll_event events[MAX_EVENT_NUMBER];
  // 资源路径
  char* assetsPath;

  // 客户端数据
  httpConn* users;

  // 数据库连接池
  std::shared_ptr<connPool> mConnPool;

  // 线程池
  std::shared_ptr<threadPool<httpConn>> mPool;

  // 定时器相关
  clientData* usersTimer;
  Utils utils;

 public:
  // 构造函数
  WebServer();
  // 析构函数
  ~WebServer();
  // 单例
  static WebServer* getInstance();
  // 服务器初始化
  void init();
  // 事件主循环
  void eventLoop();

  // 初始化线程池
  void initThreadPool();
  // 初始化数据库连接池
  void initSqlPool();
  // 初始化监听
  void initEventListen();
  // 初始化连接
  void initConn(int connFd, sockaddr_in clientAddress);
  // 调整客户端定时器
  void adjustTimer(timerNode* timer);
  // 使用回调函数处理 timer
  void dealTimer(timerNode* timer, int sockFd);
  // 处理客户端数据
  bool dealClientData();
  // 处理信号
  bool dealWithSignal(bool& timeout, bool& stopServer);
  // 处理写请求
  void dealWithRead(int sockFd);
  // 处理读请求
  void dealWithWrite(int sockFd);
};