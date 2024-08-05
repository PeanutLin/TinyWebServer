#pragma once
#include <string>

class MockMySQLConn {
 private:
  int* con;

 public:
  MockMySQLConn();
  ~MockMySQLConn();

 public:
  // 执行 sql 查询语句
  // 执行 sql 插入语句
  bool Exist(std::string& sql);
  bool Insert(std::string& sql);
};