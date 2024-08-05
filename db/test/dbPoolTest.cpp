#include <gtest/gtest.h>
#include "../MockMySQL.h"
#include "../connRAII.h"
#include "../sqlConnPool.h"

TEST(dbPoolTest, dbPoolRAIIberTest) {
  std::shared_ptr<connPool<MockMySQLConn>> pool =
      std::make_shared<connPool<MockMySQLConn>>();
  pool->init(10);
  EXPECT_EQ(pool->GetFreeConnNum(), 10);
  std::shared_ptr<MockMySQLConn> con;

  connRAII<MockMySQLConn> CR0(con, pool);
  EXPECT_EQ(pool->GetFreeConnNum(), 9);

  {
    std::shared_ptr<MockMySQLConn> con;
    connRAII<MockMySQLConn> CR1(con, pool);
    EXPECT_EQ(pool->GetFreeConnNum(), 8);
    connRAII<MockMySQLConn> CR2(con, pool);
    EXPECT_EQ(pool->GetFreeConnNum(), 7);
  }
  EXPECT_EQ(pool->GetFreeConnNum(), 9);
  pool->DestroyPool();
  EXPECT_EQ(pool->GetFreeConnNum(), 0);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}