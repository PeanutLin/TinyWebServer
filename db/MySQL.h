#pragma once
#include <mysql/mysql.h>
#include <string>

class MySQLConn {
 private:
  MYSQL* con;

 public:
  MySQLConn();
  ~MySQLConn();

 public:
  // 执行 sql 查询语句
  bool Exist(std::string& sql);
  // 执行 sql 插入语句
  bool Insert(std::string& sql);
  // 检查当前连接是否超时
  bool IsTimeout();
};