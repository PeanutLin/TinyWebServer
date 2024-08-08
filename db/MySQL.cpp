#include "MySQL.h"
#include "../log/log.h"

// 构造函数
MySQLConn::MySQLConn() {
  auto configPtr = Config::getInstance();
  // 初始化数据库连接信息
  this->con = nullptr;
  this->con = mysql_init(this->con);
  if (this->con == nullptr) {
    LOG_ERROR("mysql_init Error");
    exit(1);
  }

  // 建立 mysql 连接
  this->con = mysql_real_connect(
      this->con, "localhost", configPtr->mysqlUser.c_str(),
      configPtr->mysqlPasswd.c_str(), configPtr->databaseName.c_str(),
      configPtr->mysqlPort, nullptr, 0);
  if (this->con == nullptr) {
    LOG_ERROR("mysql_real_connect Error");
    exit(1);
  }
}

// 析构函数
MySQLConn::~MySQLConn() {
  // 关闭数据库连接
  mysql_close(this->con);
}

bool MySQLConn::Insert(std::string& sql) {
  int res = mysql_query(this->con, sql.c_str());

  return res == 0;
}

bool MySQLConn::Exist(std::string& sql) {
  // 连接池中取一个连接

  // 在 user 表中检索 username，passwd 数据，浏览器端输入
  if (mysql_query(this->con, sql.c_str())) {
    LOG_ERROR("SELECT error:%s\n", mysql_error(this->con));
  }

  // 从表中检索完整的结果集
  MYSQL_RES* result = mysql_store_result(this->con);

  // 返回结果集中的列数
  int num_fields = mysql_num_fields(result);

  // 返回所有字段结构的数组
  MYSQL_FIELD* fields = mysql_fetch_fields(result);

  // 从结果集中获取下一行，将对应的用户名和密码，存入map中
  while (MYSQL_ROW row = mysql_fetch_row(result)) {
    return true;
  }
  return false;
}

bool MySQLConn::IsTimeout() {
  return mysql_ping(this->con) != 0;
}
