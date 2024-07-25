#ifndef LOG_H
#define LOG_H

#include <cstdarg>
#include <cstdio>

#include <iostream>
#include <string>

#include "../config/config.h"
#include "../utils/utils.h"
#include "blockQueue.h"

class Log {
 private:
  std::string mDirName;  // 路径名
  std::string mLogName;  // log文件名
  int mSplitLines;       // 日志最大行数
  int mLogBufSize;       // 日志缓冲区大小
  long long mCount;      // 日志行数记录
  int mToday;            // 因为按天分类,记录当前时间是那一天
  FILE* mFp;             // 打开log的文件指针
  char* mBuf;            // log 缓冲区
  blockQueue<std::string>* mLogQueue;  // 阻塞队列
  std::mutex mMutex;

  Log();
  virtual ~Log();

  void asyncWriteLog() {
    std::string singleLog;
    // 从阻塞队列中取出一个日志 string，写入文件
    while (mLogQueue->pop(singleLog)) {
      fputs(singleLog.c_str(), mFp);
    }
    return;
  }

 public:
  // 单例
  static Log* getInstance();
  static void flushLogThread(void* args) {
    Log::getInstance()->asyncWriteLog();
    return;
  }

 public:
  // 初始化
  void init();
  // 可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列

  bool initLog(std::string& pathName, std::string& logName, int closeLog,
               int logBufSize = 8192, int splitLines = 5000000,
               int maxQueueSize = 0);

  // 写 log
  void writeLog(int level, const char* format, ...);

  // 生成 log 文件的名字
  std::string generateLogName(std::string& pathName, std::string& logName);

  // 刷新当前 log 文件的缓冲区
  void flush(void);
};

#define LOG_DEBUG(format, ...)                              \
  if (false == Config::getInstance()->isLogClosed) {        \
    Log::getInstance()->writeLog(0, format, ##__VA_ARGS__); \
    Log::getInstance()->flush();                            \
  }

#define LOG_INFO(format, ...)                               \
  if (false == Config::getInstance()->isLogClosed) {        \
    Log::getInstance()->writeLog(1, format, ##__VA_ARGS__); \
    Log::getInstance()->flush();                            \
  }

#define LOG_WARN(format, ...)                                 \
  if (false == Config::getInstance()->isLogClosed)) {         \
      Log::getInstance()->writeLog(2, format, ##__VA_ARGS__); \
      Log::getInstance()->flush();                            \
    }

#define LOG_ERROR(format, ...)                              \
  if (false == Config::getInstance()->isLogClosed) {        \
    Log::getInstance()->writeLog(3, format, ##__VA_ARGS__); \
    Log::getInstance()->flush();                            \
  }

#endif
