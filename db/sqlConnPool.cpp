#include "sqlConnPool.h"

#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <list>
#include <string>

// 构造函数
connPool::connPool() {
  // 当前连接和空闲连接均为 0
  mCurConn = 0;
  mFreeConn = 0;
}

// 初始化连接池
void connPool::init(std::string url, std::string User, std::string PassWord,
                    std::string DBName, int Port, int MaxConn) {
  mURL = url;
  mPort = Port;
  mUser = User;
  mPassWord = PassWord;
  mDatabaseName = DBName;
  // 建立 MYSQL 连接池
  for (int i = 0; i < MaxConn; i++) {
    MYSQL* con = nullptr;
    con = mysql_init(con);

    if (con == nullptr) {
      LOG_ERROR("MySQL Error");
      exit(1);
    }

    // 建立 mysql 连接
    con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(),
                             DBName.c_str(), Port, nullptr, 0);

    if (con == nullptr) {
      LOG_ERROR("MySQL Error");
      exit(1);
    }

    mConnList.push_back(con);
    ++mFreeConn;
  }
  mMaxConn = mFreeConn;
}

// 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* connPool::GetConnection() {
  MYSQL* con = nullptr;

  if (0 == mConnList.size()) {
    return nullptr;
  }
  std::unique_lock<std::mutex> locker(mMutex);
  mCond.wait(locker, [this] { return mFreeConn > 0; });

  con = mConnList.front();
  mConnList.pop_front();

  --mFreeConn;
  ++mCurConn;
  mCond.notify_all();
  return con;
}

// 释放当前使用的连接
bool connPool::ReleaseConnection(MYSQL* con) {
  if (nullptr == con) {
    return false;
  }
  std::unique_lock<std::mutex> locker(mMutex);

  mConnList.push_back(con);
  ++mFreeConn;
  --mCurConn;
  mCond.notify_all();
  return true;
}

// 销毁数据库连接池
void connPool::DestroyPool() {
  std::unique_lock<std::mutex> locker(mMutex);
  if (mConnList.size() > 0) {
    std::list<MYSQL*>::iterator it;
    for (it = mConnList.begin(); it != mConnList.end(); ++it) {
      MYSQL* con = *it;
      mysql_close(con);
    }
    mCurConn = 0;
    mFreeConn = 0;
    mConnList.clear();
  }
}

// 当前空闲的连接数
int connPool::GetFreeConn() {
  return this->mFreeConn;
}

connPool::~connPool() {
  DestroyPool();
}

connRAII::connRAII(MYSQL** SQL, std::shared_ptr<connPool> connPool) {
  *SQL = connPool->GetConnection();

  conRAII = *SQL;
  poolRAII = connPool;
}

connRAII::~connRAII() {
  poolRAII->ReleaseConnection(conRAII);
}