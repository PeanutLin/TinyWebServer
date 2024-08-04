#pragma once

#include <pthread.h>

#include <cstdio>
#include <exception>
#include <list>

#include "../db/sqlConnPool.h"
#include "../types/types.h"

template <typename T>
class threadPool {
 public:
  // 构造析构函数
  threadPool(ACTOR_MODE actor_model, std::shared_ptr<connPool> connPool,
             int threadNumber = 8, int maxRequest = 10000);
  ~threadPool();

 public:
  // 添加任务到任务队列
  bool append(T* request, REQUEST_STEATE state);

 private:
  // 工作线程运行的函数，它不断从工作队列中取出任务并执行之
  static void* worker(void* arg);
  // 线程运行
  void run();

 private:
  // 数据结构 ----------------------------------------
  std::list<T*> mWorkQueue;             // 请求队列
  std::shared_ptr<connPool> mConnPool;  // 数据库
  // 配置变量 ----------------------------------------
  ACTOR_MODE mActorModel;  // 模型
  int mThreadNumber;       // 线程池中的线程数
  int mMaxRequests;        // 请求队列中允许的最大请求数
  pthread_t* mThreads;  // 描述线程池的数组，其大小为m_threadNumber
  // 同步变量 ----------------------------------------
  std::mutex mMutex;              // 保护请求队列的互斥锁
  std::condition_variable mCond;  // 条件变量
};

template <typename T>
threadPool<T>::threadPool(ACTOR_MODE actor_model,
                          std::shared_ptr<connPool> connPool, int threadNumber,
                          int maxRequests)
    : mActorModel(actor_model),
      mThreadNumber(threadNumber),
      mMaxRequests(maxRequests),
      mThreads(NULL),
      mConnPool(connPool) {
  if (threadNumber <= 0 || maxRequests <= 0) {
    throw std::exception();
  }
  mThreads = new pthread_t[mThreadNumber];
  if (!mThreads) {
    throw std::exception();
  }

  for (int i = 0; i < threadNumber; ++i) {
    if (pthread_create(mThreads + i, NULL, worker, this) != 0) {
      delete[] mThreads;
      throw std::exception();
    }
    if (pthread_detach(mThreads[i])) {
      delete[] mThreads;
      throw std::exception();
    }
  }
}

template <typename T>
threadPool<T>::~threadPool() {
  delete[] mThreads;
}

template <typename T>
bool threadPool<T>::append(T* request, REQUEST_STEATE state) {
  std::unique_lock<std::mutex> locker(mMutex);
  mCond.wait(locker, [this] { return mWorkQueue.size() < mMaxRequests; });
  request->mState = state;
  mWorkQueue.push_back(request);
  mCond.notify_all();
  return true;
}

template <typename T>
void* threadPool<T>::worker(void* arg) {
  threadPool* pool = (threadPool*)arg;
  pool->run();
  return pool;
}

template <typename T>
void threadPool<T>::run() {

  // 循环从链表中读取任务
  while (true) {
    // 同步
    std::unique_lock<std::mutex> locker(mMutex);
    mCond.wait(locker, [this] { return mWorkQueue.size() > 0; });

    // 取任务
    T* request = mWorkQueue.front();
    mWorkQueue.pop_front();

    // 模式
    if (ACTOR_MODE::REACTOR == mActorModel) {
      if (REQUEST_STEATE::READ == request->mState) {
        if (request->readHTTPRequest()) {
          request->improv = 1;
          connRAII mysqlcon(&request->mysql, mConnPool);
          request->handleHTTPRequest();
        } else {
          request->improv = 1;
          request->timerFlag = 1;
        }
      } else if (REQUEST_STEATE::WRITE == request->mState) {
        if (request->writeHTTPResponse()) {
          request->improv = 1;
        } else {
          request->improv = 1;
          request->timerFlag = 1;
        }
      } else {
        // none
      }
    } else if (ACTOR_MODE::PROACTOR == mActorModel) {
      connRAII mysqlcon(&request->mysql, mConnPool);
      request->handleHTTPRequest();
    } else {
      // none
    }
  }
}
