#include "config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

Config::Config() {
  // 服务器相关 -----------------
  // 端口号, 默认 9006
  Port = 9006;
  // 并发模型选择，默认 PROACTOR
  actorModel = ACTOR_MODE::PROACTOR;
  // 线程池内的线程数量，默认 8
  threadNum = 8;
  // 优雅关闭链接，默认关闭
  gClosed = false;
  // listenfd触发模式，默认 LT
  listenTrigmode = TRIGGER_MODE::LT;
  // connfd触发模式，默认 LT
  connTrigmode = TRIGGER_MODE::LT;

  // 日志相关 -----------------
  // 是否关闭日志，默认开启
  isLogClosed = false;
  // 是否异步，默认同步
  isLogAsync = false;

  // 数据库相关 -----------------
  // 数据库连接池数量，默认 8
  sqlNum = 8;
  // mysql 地址
  mysqlHost = "127.0.0.1";
  // 数据库端口
  mysqlPort = 3306;
  // 数据库用户
  mysqlUser = "root";
  // 数据库密码
  mysqlPasswd = "root";
  // 数据库名
  databaseName = "webserverdb";
}

Config* Config::getInstance() {
  static Config config;
  return &config;
}

void Config::init(std::string& fileName) {
  loadConfigFile(fileName);
  std::cout << "Port: " << Port << std::endl;
  std::cout << "ThreadNum: " << threadNum << std::endl;
  std::cout << "ActorModel: " << actorModel << std::endl;
  std::cout << "GracefulClose: " << gClosed << std::endl;
  std::cout << "ListenTrigmode: " << listenTrigmode << std::endl;
  std::cout << "ConnTrigmode: " << connTrigmode << std::endl;
  std::cout << "LogClosed: " << isLogClosed << std::endl;
  std::cout << "LogAsync: " << isLogAsync << std::endl;
  std::cout << "LogPathName: " << pathName << std::endl;
  std::cout << "LogName: " << logName << std::endl;
  std::cout << "Host: " << mysqlHost << std::endl;
  std::cout << "Port: " << mysqlPort << std::endl;
  std::cout << "User: " << mysqlUser << std::endl;
  std::cout << "Password: " << mysqlPasswd << std::endl;
  std::cout << "DatabaseName: " << databaseName << std::endl;
  std::cout << "SqlThreadNum: " << sqlNum << std::endl;
}

bool Config::loadConfigFile(std::string& fileName) {
  YAML::Node config = YAML::LoadFile(fileName);
  if (!config) {
    std::cout << "Open config File:" << fileName << " failed.";
    return false;
  }

  if (config["WebServer"]["Port"]) {
    Port = config["WebServer"]["Port"].as<int>();
  }

  if (config["WebServer"]["ThreadNum"]) {
    threadNum = config["WebServer"]["ThreadNum"].as<int>();
  }

  if (config["WebServer"]["ActorModel"]) {
    if (config["WebServer"]["ActorModel"].as<std::string>() == "Proactor") {
      actorModel = ACTOR_MODE::PROACTOR;
    } else {
      actorModel = ACTOR_MODE::REACTOR;
    }
  }

  if (config["WebServer"]["GracefulClose"]) {
    std::string gracefulCloseStr =
        config["WebServer"]["GracefulClose"].as<std::string>();
    gClosed = (gracefulCloseStr == "True");
  }

  if (config["WebServer"]["ListenTrigmode"]) {
    if (config["WebServer"]["ListenTrigmode"].as<std::string>() == "LT") {
      listenTrigmode = TRIGGER_MODE::LT;
    } else {
      listenTrigmode = TRIGGER_MODE::ET;
    }
  }

  if (config["WebServer"]["ConnTrigmode"]) {
    if (config["WebServer"]["ConnTrigmode"].as<std::string>() == "LT") {
      connTrigmode = TRIGGER_MODE::LT;
    } else {
      connTrigmode = TRIGGER_MODE::ET;
    }
  }

  if (config["Log"]["Close"]) {
    std::string logCloseStr = config["Log"]["Close"].as<std::string>();
    isLogClosed = (logCloseStr == "True");
  }

  if (config["Log"]["PathName"]) {
    pathName = config["Log"]["PathName"].as<std::string>();
  }

  if (config["Log"]["LogName"]) {
    logName = config["Log"]["LogName"].as<std::string>();
  }

  if (config["Log"]["Async"]) {
    std::string logAsyncStr = config["Log"]["Async"].as<std::string>();
    isLogAsync = (logAsyncStr == "True");
  }

  if (config["MySql"]["Host"]) {
    mysqlHost = config["MySql"]["Host"].as<std::string>();
  }

  if (config["MySql"]["Port"]) {
    mysqlPort = config["MySql"]["Port"].as<int>();
  }

  if (config["MySql"]["User"]) {
    mysqlUser = config["MySql"]["User"].as<std::string>();
  }

  if (config["MySql"]["Password"]) {
    mysqlPasswd = config["MySql"]["Password"].as<std::string>();
  }

  if (config["MySql"]["DatabaseName"]) {
    databaseName = config["MySql"]["DatabaseName"].as<std::string>();
  }

  if (config["MySql"]["SqlNum"]) {
    sqlNum = config["MySql"]["SqlNum"].as<int>();
  }

  return true;
}