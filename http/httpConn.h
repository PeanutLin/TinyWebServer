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

  // http 状态机行检查下标
  long mCheckedIndex;
  int mStartLine;

  CHECK_STATE mCheckState;
  METHOD mMethod;
  char mRealFile[FILENAME_LEN];
  char* mURL;
  char* mVersion;
  char* mHost;
  long mContentLength;
  bool mLinger;
  // 待返回的文件数据地址（mmap）
  char* mFileAddress;

  // 文件处理 ----------------------
  struct stat mFileStat;
  // mIV[0]: httpResponse
  // mIV[1]: 文件数据
  struct iovec mIV[2];
  // 实际有效的 iovec 数量
  int mIVCount;

  // 是否启用的 POST
  int cgi;
  // 存储请求头数据
  char* mString;

  // 数据接收发送相关 ----------------------
  // 客户端 tcp Read 缓冲区
  char mReadBuf[READ_BUFFER_SIZE];
  // 读缓冲区下标
  long mReadIndex;
  // 报文头部写缓冲区
  char mWriteBuf[WRITE_BUFFER_SIZE];
  // 写缓冲区下标
  int mWriteIndex;
  // 一共要发送给客户端的数据大小
  int bytesToSend;
  // 已经发送给客户端的数据大小
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
  HTTP_CODE parseHTTPRequest();
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
  // 取消文件的内存映射
  void unmap();

  // 添加响应的封装 ---------------------------------------------
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
  // 循环读取客户数据
  bool readHTTPRequest();
  // 解析 HTTP 数据包
  void handleHTTPRequest();
  // 准备写的信息
  bool prepareHTTPResponse(HTTP_CODE ret);
  // 向客户端写数据
  bool writeHTTPResponse();
  // 获取客户端信息
  sockaddr_in* getAddress();
  // 初始化 mysql 数据查询
  void initMysqlResult(std::shared_ptr<connPool>& connPool);
  int timerFlag;
  int improv;
};

void callBackFunction(clientData* user_data);