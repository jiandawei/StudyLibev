#!/bin/bash

echo "编译 IPC 示例程序..."

CC=${CC:-gcc}
CFLAGS="-Wall -Wextra"

if [ ! -f "fifo_server.c" ] || [ ! -f "fifo_client.c" ]; then
    echo "错误: 请在 examples/IPC 目录下运行此脚本"
    exit 1
fi

echo "编译 fifo_server..."
$CC $CFLAGS fifo_server.c -o fifo_server || { echo "fifo_server 编译失败"; exit 1; }

echo "编译 fifo_client..."
$CC $CFLAGS fifo_client.c -o fifo_client || { echo "fifo_client 编译失败"; exit 1; }

echo ""
echo "编译成功!"
echo ""
echo "运行方式:"
echo "  终端1: ./fifo_server"
echo "  终端2: ./fifo_client"
