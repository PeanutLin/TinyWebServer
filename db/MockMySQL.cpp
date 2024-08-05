
#include "MockMySQL.h"
#include <memory>

MockMySQLConn::MockMySQLConn() {
  con = new int{-1};
}

MockMySQLConn::~MockMySQLConn() {
  delete con;
  con = nullptr;
}

bool MockMySQLConn::Insert(std::string& sql) {
  *(this->con) = 0;
  return true;
}

bool MockMySQLConn::Exist(std::string& sql) {
  *(this->con) = 1;
  return true;
}
