#include "log.h"

#include <sys/time.h>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <mutex>
#include <thread>

using namespace std;

Log* Log::getInstance() {
  static Log log;
  return &log;
}

Log::Log() {
  mCount = 0;
}

Log::~Log() {
  if (mFp != nullptr) {
    fclose(mFp);
  }
}

// 初始化
void Log::init() {
  auto configPtr = Config::getInstance();
  if (false == configPtr->isLogClosed) {
    if (true == configPtr->isLogAsync) {
      initLog(configPtr->pathName, configPtr->logName, configPtr->isLogClosed,
              2000, 800000, 800);
    } else {
      initLog(configPtr->pathName, configPtr->logName, configPtr->isLogClosed,
              2000, 800000, 0);
    }
  }
}

// 异步需要设置阻塞队列的长度，同步不需要设置
bool Log::initLog(std::string& pathName, std::string& logName, int closeLog,
                  int logBufSize, int splitLines, int maxQueueSize) {

  mLogBufSize = logBufSize;
  mBuf = new char[mLogBufSize];
  memset(mBuf, '\0', mLogBufSize);
  mSplitLines = splitLines;

  // 创建文件夹
  UTILS::createDirectories(pathName);
  // 获得完整的 log 文件名字
  string logFullName = generateLogName(pathName, logName);

  mFp = fopen(logFullName.c_str(), "a");
  if (mFp == nullptr) {
    return false;
  }

  // 如果设置了maxQueueSize,则设置为异步
  if (maxQueueSize >= 1) {
    mLogQueue = new blockQueue<string>(maxQueueSize);

    // 启动 log 线程
    thread logThread(flushLogThread, nullptr);
    logThread.detach();
  }

  return true;
}

// 写 log
void Log::writeLog(int level, const char* format, ...) {
  char s[16] = {0};
  switch (level) {
    case 0:
      strcpy(s, "[debug]:");
      break;
    case 1:
      strcpy(s, "[info]:");
      break;
    case 2:
      strcpy(s, "[warn]:");
      break;
    case 3:
      strcpy(s, "[erro]:");
      break;
    default:
      strcpy(s, "[info]:");
      break;
  }

  // 写入一个log，对 mCount++, mSplitLines最大行数
  mMutex.lock();
  mCount++;

  // 根据天数或者行数实现分页 log
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t t = now.tv_sec;
  tm* sys_tm = localtime(&t);
  tm myTm = *sys_tm;
  if (mToday != myTm.tm_mday || mCount % mSplitLines == 0) {
    char newLog[256] = {0};
    fflush(mFp);
    fclose(mFp);
    char tail[16] = {0};

    snprintf(tail, 16, "%d_%02d_%02d_", myTm.tm_year + 1900, myTm.tm_mon + 1,
             myTm.tm_mday);

    if (mToday != myTm.tm_mday) {
      snprintf(newLog, 255, "%s%s%s", mDirName.c_str(), tail, mLogName.c_str());
      mToday = myTm.tm_mday;
      mCount = 0;
    } else {
      snprintf(newLog, 255, "%s%s%s.%lld", mDirName.c_str(), tail,
               mLogName.c_str(), mCount / mSplitLines);
    }
    mFp = fopen(newLog, "a");
  }

  mMutex.unlock();

  // 可变参数开始----------
  va_list valst;
  va_start(valst, format);

  string logStr;
  mMutex.lock();

  // 写入的具体时间内容格式
  int n = snprintf(mBuf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                   myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday,
                   myTm.tm_hour, myTm.tm_min, myTm.tm_sec, now.tv_usec, s);

  int m = vsnprintf(mBuf + n, mLogBufSize - n - 1, format, valst);
  mBuf[n + m] = '\n';
  mBuf[n + m + 1] = '\0';
  logStr = mBuf;

  mMutex.unlock();

  if (Config::getInstance()->isLogAsync && !mLogQueue->full()) {
    mLogQueue->push(logStr);
  } else {
    mMutex.lock();
    fputs(logStr.c_str(), mFp);
    mMutex.unlock();
  }

  va_end(valst);
  // 可变参数结束----------
}

// 刷新 log 缓冲区
void Log::flush(void) {
  std::unique_lock<std::mutex> locker(mMutex);
  // 强制刷新写入流缓冲区
  fflush(mFp);
}

// 生成 log 文件的名字
std::string Log::generateLogName(std::string& pathName, std::string& logName) {

  time_t t = time(nullptr);
  tm* sysTm = localtime(&t);
  tm myTm = *sysTm;

  // 设置当前 log 的日期
  mToday = myTm.tm_mday;

  std::string logFullName;
  int year = myTm.tm_year + 1900;
  int month = myTm.tm_mon + 1;
  int day = myTm.tm_mday;
  logFullName = pathName + std::to_string(year) + "_" +
                (month < 10 ? "0" : "") + std::to_string(month) + "_" +
                (day < 10 ? "0" : "") + std::to_string(day) + "_" + logName;
  return logFullName;
}