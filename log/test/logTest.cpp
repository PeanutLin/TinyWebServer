#include "../log.h"

#include <gtest/gtest.h>

TEST(LogTest, LogNamePathParseTest) {
  std::string pathName("./mylog/test/");
  std::string logName("MyLog");
  Log::getInstance()->initLog(pathName, logName, false, 2000, 800000, 800);
  // EXPECT_EQ(add(1, 2), "./MyPath/ServerLog");
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}