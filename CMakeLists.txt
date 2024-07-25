cmake_minimum_required(VERSION 3.0.0)
project(WebServer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS   "-g")                     # 调试信息

find_package(yaml-cpp REQUIRED)

add_executable(myserver
  ./log/log.cpp
  ./config/config.cpp
  ./utils/utils.cpp
  ./timer/timerList.cpp
  ./http/httpConn.cpp
  ./db/sqlConnPool.cpp
  ./webserver/webserver.cpp
  ./app/main.cpp
)

target_link_libraries(myserver
  pthread
  mysqlclient
  yaml-cpp
)