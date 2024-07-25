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
#include <unistd.h>

#include <atomic>
#include <map>

#include "../db/sqlConnPool.h"
#include "../types/types.h"
#include "../utils/utils.h"

class httpConn {
  // 静态变量
 public:
  static const int FILENAME_LEN = 200;
  static const int READ_BUFFER_SIZE = 2048;
  static const int WRITE_BUFFER_SIZE = 1024;
  // Epoll fd
  static int mEpollfd;
  // 当前连接服务人数
  static std::atomic<int> userCount;

  enum METHOD {
    GET = 0,
    POST,
    HEAD,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    PATH
  };
  enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0,
    CHECK_STATE_HEADER,
    CHECK_STATE_CONTENT
  };
  enum HTTP_CODE {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
  };
  enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

 public:
  MYSQL* mysql;
  // 当前请求状态
  REQUEST_STEATE mState;

 private:
  // 客户端连接 fd
  int mSockfd;
  // 客户端信息
  sockaddr_in mAddress;
  char mReadBuf[READ_BUFFER_SIZE];
  long mReadIndex;
  long mCheckedIndex;
  int mStartLine;
  char mWriteBuf[WRITE_BUFFER_SIZE];
  int mWriteIndex;
  CHECK_STATE mCheckState;
  METHOD mMethod;
  char mRealFile[FILENAME_LEN];
  char* mURL;
  char* mVersion;
  char* mHost;
  long mContentLength;
  bool mLinger;
  char* mFileAddress;

  // 文件处理
  struct stat mFileStat;
  struct iovec mIV[2];
  int mIVCount;

  int cgi;        // 是否启用的 POST
  char* mString;  // 存储请求头数据
  int bytesToSend;
  int bytesHaveSend;
  char* docRoot;

  std::map<std::string, std::string> mUsers;
  int mTRIGMode;

  char sqlUser[100];
  char sqlPasswd[100];
  char sqlName[100];

 private:
  void init();
  // 解析数据包-解析请求
  HTTP_CODE processRead();
  // 解析数据包-准备写的信息
  bool processWrite(HTTP_CODE ret);
  // 解析请求行
  HTTP_CODE parseRequestLine(char* text);
  // 解析请求头
  HTTP_CODE parseHeaders(char* text);
  // 解析请求内容
  HTTP_CODE parseContent(char* text);
  // 解析 URL
  HTTP_CODE doRequest();
  char* getLine() { return mReadBuf + mStartLine; };
  // 从状态机，用于分析出一行内容
  LINE_STATUS parseLine();
  void unmap();

  bool addResponse(const char* format, ...);
  // 添加响应主体的内容
  bool addContent(const char* content);
  // 添加状态行
  bool addStatusLine(int status, const char* title);
  // 添加 HTTP 头部信息
  bool addHeaders(int content_length);
  // 添加 Content-Type 头部字段，指定响应内容的媒体类型
  bool addContentType();
  // 添加 Content-Length 头部字段，指定响应内容的长度
  bool addContentLength(int content_length);
  // 添加 Connection: keep-alive 或Connection: close头部字段，表示是否在发送响应后保持连接
  bool addLinger();
  // 用于添加一个空行，标志着HTTP头部的结束和响应主体的开始。
  bool addBlankLine();

 public:
  httpConn() {}
  ~httpConn() {}

 public:
  void init(int sockfd, const sockaddr_in& addr, char*, int, int,
            std::string user, std::string passwd, std::string sqlname);
  // 主动关闭连接
  void closeConn(bool real_close = true);
  // 解析数据包
  void process();
  // 循环读取客户数据
  bool readOnce();
  // 向客户端写数据
  bool write();
  // 获取客户端信息
  sockaddr_in* getAddress();
  // 初始化 mysql 数据查询
  void initMysqlResult(std::shared_ptr<connPool>& connPool);
  int timerFlag;
  int improv;
};

void callBackFunction(clientData* user_data);