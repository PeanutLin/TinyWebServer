cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)

# 查找 GTest 库
find_package(GTest REQUIRED)
find_package(yaml-cpp REQUIRED)

# GTest 的头文件
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(configTest 
        configTest.cpp
        ../config.cpp
        ../config.h
        )

# 链接测试库
target_link_libraries( configTest
        ${GTEST_BOTH_LIBRARIES}
        pthread
        yaml-cpp

        )

# 添加到测试
gtest_discover_tests(configTest)
