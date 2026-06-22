#!/bin/bash
set -e

TARGET="memory_demo"
SOURCE="main_memory_order_demo.cpp"
BUILD_DIR="./build"

# 清理函数
clean_build() {
    echo "=== 开始清理编译产物 ==="
    rm -rf "${BUILD_DIR}"
    rm -f "${TARGET}"
    echo "清理完成"
    exit 0
}

# 处理清理参数
if [[ "$1" == "--clean" ]]; then
    clean_build
fi

# 创建构建目录
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# C++17 开启O2优化，更容易复现内存乱序现象
clang++ -std=c++17 -O3 -Wall -Wextra -o "${TARGET}" ../"${SOURCE}"

echo -e "\n=== 编译成功 ==="