#!/bin/bash

# 删除 ./build 文件夹
if [ -d "./build" ]; then
    echo "Deleting ./build directory..."
    rm -rf ./build
fi

# 新建 ./build 文件夹
echo "Creating ./build directory..."
mkdir ./build

# 进入到 ./build 文件夹中执行 cmake ..
cd ./build
echo "Running cmake .."
cmake ..

# 编译项目
echo "Building the project..."
make

echo "Build and copy complete."

# 运行项目
./timerTest
