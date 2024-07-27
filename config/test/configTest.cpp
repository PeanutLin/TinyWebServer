#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "../config.h"

TEST(ConfigTest, ConfigParseTest) {
  Config config;
  std::string filePath = "./config.yaml";
  config.loadConfigFile(filePath);
  EXPECT_EQ(config.Port, 6666);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}