#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "../config.h"

TEST(ConfigTest, ConfigParseTest) {
  Config config;
  std::string filePath = "./config.yaml";
  config.init(filePath);
  EXPECT_EQ(config.Port, 8080);
  EXPECT_EQ(config.threadNum, 8);
  EXPECT_EQ(config.actorModel, ACTOR_MODE::PROACTOR);
  EXPECT_EQ(config.gClosed, false);
  EXPECT_EQ(config.listenTrigmode, TRIGGER_MODE::LT);
  EXPECT_EQ(config.connTrigmode, TRIGGER_MODE::LT);

  EXPECT_EQ(config.isLogClosed, false);
  EXPECT_EQ(config.isLogAsync, false);
  EXPECT_EQ(config.pathName, "./WebServerLog/");
  EXPECT_EQ(config.logName, "ServerLog");

  EXPECT_EQ(config.mysqlHost, "127.0.0.1");
  EXPECT_EQ(config.mysqlPort, 3306);
  EXPECT_EQ(config.mysqlUser, "root");
  EXPECT_EQ(config.mysqlPasswd, "030608");
  EXPECT_EQ(config.databaseName, "webserverdb");
  EXPECT_EQ(config.sqlNum, 8);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}