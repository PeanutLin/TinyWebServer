#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>

template <class T>
class blockQueue {
 private:
  // 保护队列
  std::mutex mMutex;
  // 条件变量，生产者消费者模型队列
  std::condition_variable mCond;
  // 目前元素数量
  int mSize;
  // 循环数组能容纳的最大数量
  int mMaxSize;
  // 队首指针
  int mFront;
  // 队尾指针
  int mBack;
  // 循环数组指针 (mFront, mBack]
  T* mPtr;

 public:
  // 构造函数
  blockQueue(int maxSize = 1000) {
    if (maxSize <= 0) {
      exit(-1);
    }

    mMaxSize = maxSize;
    mPtr = new T[maxSize];
    mSize = 0;
    mFront = -1;
    mBack = -1;
  }

  // 清空队列
  void clear() {
    std::unique_lock<std::mutex> locker(mMutex);
    mSize = 0;
    mFront = -1;
    mBack = -1;
  }

  // 析构函数
  ~blockQueue() {
    if (mPtr != nullptr) {
      delete[] mPtr;
    }
  }

  // 队列是否满
  bool full() {
    std::unique_lock<std::mutex> locker(mMutex);
    return mSize >= mMaxSize;
  }

  // 队列是否为空
  bool empty() {
    std::unique_lock<std::mutex> locker(mMutex);
    return 0 == mSize;
  }

  // 返回队首元素
  bool front(T& value) {
    std::unique_lock<std::mutex> locker(mMutex);
    if (0 == mSize) {
      return false;
    }
    value = mPtr[mFront];
    return true;
  }

  // 返回队尾元素
  bool back(T& value) {
    std::unique_lock<std::mutex> locker(mMutex);
    if (0 == mSize) {
      return false;
    }
    value = mPtr[mBack];
    return true;
  }

  // 返回当前队列元素个数
  int size() {
    std::unique_lock<std::mutex> locker(mMutex);
    return mSize;
  }

  // 返回当前队列能容纳的元素个数
  int max_size() {
    std::unique_lock<std::mutex> locker(mMutex);
    return mMaxSize;
  }

  // 添加元素
  bool push(const T& item) {
    std::unique_lock<std::mutex> locker(mMutex);
    mCond.wait(locker, [this] { return mSize < mMaxSize; });

    mBack = (mBack + 1) % mMaxSize;
    mPtr[mBack] = item;

    mSize++;
    mCond.notify_all();
    return true;
  }

  // 弹出元素
  bool pop(T& item) {
    std::unique_lock<std::mutex> locker(mMutex);
    mCond.wait(locker, [this] { return mSize > 0; });

    mFront = (mFront + 1) % mMaxSize;
    item = mPtr[mFront];
    mSize--;
    mCond.notify_all();
    return true;
  }
};
