#!/bin/bash

echo "编译消息队列 IPC 示例程序..."

CC=${CC:-gcc}
CFLAGS="-Wall -Wextra"

if [ ! -f "mq_server.c" ] || [ ! -f "mq_client.c" ]; then
    echo "错误: 请在 examples/IPC/message_queue 目录下运行此脚本"
    exit 1
fi

OS=$(uname -s)
if [ "$OS" = "Linux" ]; then
    echo "检测到 Linux，使用 POSIX 消息队列 (mqueue)"
    LIBS="-lrt"
else
    echo "检测到 $OS，使用 System V 消息队列 (sys/msg)"
    LIBS=""
fi

echo "编译 mq_server..."
$CC $CFLAGS mq_server.c -o mq_server $LIBS || { echo "mq_server 编译失败"; exit 1; }

echo "编译 mq_client..."
$CC $CFLAGS mq_client.c -o mq_client $LIBS || { echo "mq_client 编译失败"; exit 1; }

echo ""
echo "编译成功!"
echo ""
echo "运行方式:"
echo "  终端1: ./mq_server"
echo "  终端2: ./mq_client [优先级/消息类型]"
echo ""
echo "示例:"
echo "  ./mq_client         # 默认优先级/类型"
echo "  ./mq_client 3       # 优先级3（Linux）或 类型3（macOS）"
