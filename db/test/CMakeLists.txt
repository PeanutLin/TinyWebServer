cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)

# 查找 GTest 库
find_package(GTest REQUIRED)
# GTest 的头文件
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(dbPoolTest
        dbPoolTest.cpp
        ../MockMySQL.cpp
        ../sqlConnPool.h
)

# 链接测试库
target_link_libraries( dbPoolTest
        ${GTEST_BOTH_LIBRARIES}
)

# 添加到测试
gtest_discover_tests(dbPoolTest)
