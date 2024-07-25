#include "../webserver/webserver.h"

std::string configPath = "./config.yaml";

int main(int argc, char* argv[]) {
  // 初始化配置
  Config::getInstance()->init(configPath);

  // 初始化 Log
  Log::getInstance()->init();

  // 初始化 WebServer
  WebServer::getInstance()->init();

  // WebServer 事件循环
  WebServer::getInstance()->eventLoop();
  return 0;
}