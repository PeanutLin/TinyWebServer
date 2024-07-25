#!/bin/bash

# 创建目录
DirName="TinyWebServer"
mkdir -p $DirName

# 复制文件和目录到目录
cp -r ./assets $DirName
cp ./myserver $DirName
cp ./config.yaml $DirName

# 压缩临时目录到 zip 文件
tar -cvf TinyWebServer.tar $DirName

# 删除临时目录
rm -rf $DirName

echo "TinyWebServer.tar build"
