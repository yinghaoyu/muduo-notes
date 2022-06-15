#!/bin/bash

# 执行时显示指令及参数
set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${SOURCE_DIR:-/build}
BUILD_TYPE=${BUILD_TYPE:-release}
CXX={CXX:-g++}

# DCMAKE_EXPORT_COMPILE_COMMANDS=ON开启后，其生成的文件compile_commands.json，包含所有编译单元所执行的指令

mkdir -p $BUILD_DIR/$BUILD_TYPE-cpp11 \
  && cd $BUILD_DIR/$BUILD_TYPE-cpp11 \
  && cmake \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
           $SOURCE_DIR \
  && make $*
