#!/bin/bash

# libev示例程序编译脚本
# 用于编译examples/libev_tutorial目录下的所有示例程序

echo "开始编译 libev 示例程序..."

# 检查是否在正确的目录
if [ ! -f "Makefile" ]; then
    echo "错误: 未找到 Makefile，请确保在 examples/libev_tutorial 目录下运行此脚本"
    exit 1
fi

# 清理旧的编译文件
echo "清理旧的编译文件..."
make clean

# 编译所有示例程序
echo "编译所有示例程序..."
make

if [ $? -eq 0 ]; then
    echo ""
    echo "编译成功!"
    echo "你可以运行以下命令来测试示例："
    echo ""
    echo "基础示例:"
    echo "./timer_example           - 定时器示例"
    echo "./io_example              - I/O示例"
    echo "./signal_example          - 信号示例"
    echo "./idle_example            - 空闲示例"
    echo "./periodic_example        - 周期性定时器示例"
    echo ""
    echo "高级示例:"
    echo "./prepare_check_example   - Prepare/Check示例"
    echo "./stat_example            - 文件状态示例"
    echo "./cleanup_example         - 清理示例"
    echo "./async_example           - 异步事件示例"
    echo "./child_example           - 子进程监控示例"
    echo "./fork_example            - Fork事件示例"
    echo "./embed_example           - 嵌入循环示例"
    echo ""
    echo "综合示例:"
    echo "./mixed_example           - 混合示例"
    echo "./echo_server             - 回显服务器"
    echo ""
else
    echo "编译失败!"
    exit 1
fi