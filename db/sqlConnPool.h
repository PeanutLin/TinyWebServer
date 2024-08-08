#pragma once
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <string>

// 连接池
template <typename ConnType>
class connPool {
 private:
  int MaxConn;                                    // 最大连接数
  int CurConn;                                    // 当前已使用的连接数
  int FreeConn;                                   // 当前空闲的连接数
  std::list<std::shared_ptr<ConnType>> ConnList;  // 连接池

  // 连接池同步
  std::mutex mMutex;
  std::condition_variable mCond;

 public:
  // 构造函数
  connPool();
  // 析构函数
  ~connPool();

 public:
  // 从连接池获取一个可用的连接
  std::shared_ptr<ConnType> GetConnection();
  // 释放连接，将连接放回到连接池
  bool ReleaseConnection(std::shared_ptr<ConnType>);
  // 获取空闲连接个数
  int GetFreeConnNum();
  // 销毁连接池，并销毁里面所有连接
  void DestroyPool();
  // 初始化连接池
  void init(int MaxConn);
};

// 构造函数
template <typename ConnType>
connPool<ConnType>::connPool() {
  // 当前连接和空闲连接均为 0
  CurConn = 0;
  FreeConn = 0;
}

// 析构函数
template <typename ConnType>
connPool<ConnType>::~connPool() {
  DestroyPool();
}

// 初始化连接池
template <typename ConnType>
void connPool<ConnType>::init(int MaxConn) {
  for (int i = 0; i < MaxConn; i++) {
    std::shared_ptr<ConnType> con = std::make_shared<ConnType>();
    ConnList.push_back(con);
    ++FreeConn;
  }
  MaxConn = FreeConn;
}

// 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
template <typename ConnType>
std::shared_ptr<ConnType> connPool<ConnType>::GetConnection() {
  if (0 == ConnList.size()) {
    return nullptr;
  }

  std::unique_lock<std::mutex> locker(mMutex);
  mCond.wait(locker, [this] { return FreeConn > 0; });

  auto con = ConnList.front();
  ConnList.pop_front();
  if (con->IsTimeout()) {
    con = std::make_shared<ConnType>();
  }
  --FreeConn;
  ++CurConn;
  mCond.notify_all();
  return con;
}

// 释放当前使用的连接
template <typename ConnType>
bool connPool<ConnType>::ReleaseConnection(std::shared_ptr<ConnType> con) {

  std::unique_lock<std::mutex> locker(mMutex);

  ConnList.push_back(con);
  ++FreeConn;
  --CurConn;
  mCond.notify_all();
  return true;
}

// 销毁数据库连接池
template <typename ConnType>
void connPool<ConnType>::DestroyPool() {
  std::unique_lock<std::mutex> locker(mMutex);
  if (ConnList.size() > 0) {
    ConnList.clear();
    CurConn = 0;
    ConnList.clear();
    FreeConn = 0;
  }
}

// 当前空闲的连接数
template <typename ConnType>
int connPool<ConnType>::GetFreeConnNum() {
  return this->FreeConn;
}
