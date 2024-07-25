#pragma once

#include <iostream>
#include "../types/types.h"

class Config {
 public:
  // 服务器相关 -----------------
  // 端口号
  int Port;
  // 并发模型选择
  ACTOR_MODE actorModel;
  // 线程池内的线程数量
  int threadNum;
  // 优雅关闭链接
  bool gClosed;
  // listenfd 触发模式
  TRIGGER_MODE listenTrigmode;
  // connfd 触发模式
  TRIGGER_MODE connTrigmode;

  // 日志相关 -----------------
  // 是否关闭日志
  bool isLogClosed;
  // 是否异步
  bool isLogAsync;
  // 日志存放目录
  std::string pathName;
  // 日志名称
  std::string logName;

  // 数据库相关 -----------------
  // 数据库连接池数量
  int sqlNum;
  // 数据库地址
  std::string mysqlHost;
  // 数据库端口
  int mysqlPort;
  // 数据库用户
  std::string mysqlUser;
  // 数据库密码
  std::string mysqlPasswd;
  // 数据库名
  std::string databaseName;

 public:
  //  单例
  static Config* getInstance();

 public:
  // 构造函数
  Config();
  // 初始化
  void init(std::string& fileName);

 private:
  // 加载配置文件
  bool loadConfigFile(std::string& fileName);
};
