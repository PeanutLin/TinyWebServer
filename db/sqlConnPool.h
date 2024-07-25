#pragma once

#include <error.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

#include <condition_variable>
#include <iostream>
#include <list>
#include <mutex>
#include <string>

#include "../log/log.h"

// 连接池
class connPool {
 private:
  int mMaxConn;                 // 最大连接数
  int mCurConn;                 // 当前已使用的连接数
  int mFreeConn;                // 当前空闲的连接数
  std::list<MYSQL*> mConnList;  // 连接池

  // 连接池同步
  std::mutex mMutex;
  std::condition_variable mCond;

 public:
  std::string mURL;           // 主机地址
  std::string mPort;          // 数据库端口号
  std::string mUser;          // 登陆数据库用户名
  std::string mPassWord;      // 登陆数据库密码
  std::string mDatabaseName;  // 使用数据库名
  bool mIsClosed;             // 日志开关

 public:
  // 构造函数
  connPool();
  // 析构函数
  ~connPool();

 public:
  MYSQL* GetConnection();               // 获取数据库连接
  bool ReleaseConnection(MYSQL* conn);  // 释放连接
  int GetFreeConn();                    // 获取连接
  void DestroyPool();                   // 销毁所有连接

  // 初始化连接池
  void init(std::string url, std::string User, std::string PassWord,
            std::string DBName, int Port, int MaxConn);
};

// 使用 RALL 管理 connection
class connRAII {
 public:
  connRAII(MYSQL** con, std::shared_ptr<connPool> connPool);
  ~connRAII();

 private:
  MYSQL* conRAII;
  std::shared_ptr<connPool> poolRAII;
};
