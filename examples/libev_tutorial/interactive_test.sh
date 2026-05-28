#!/bin/bash

# 交互式测试脚本
# 提供菜单来选择要运行的示例

PS3='请选择要运行的示例程序 (或按 0 退出): '

options=(
    "timer_example - 定时器示例"
    "io_example - I/O示例"
    "signal_example - 信号示例"
    "idle_example - 空闲示例"
    "periodic_example - 周期性定时器示例"
    "prepare_check_example - Prepare/Check示例"
    "stat_example - 文件状态示例"
    "cleanup_example - 清理示例"
    "async_example - 异步示例"
    "child_example - 子进程监控示例"
    "fork_example - Fork事件示例"
    "embed_example - 嵌入循环示例"
    "mixed_example - 混合示例"
    "echo_server - 回显服务器"
    "退出"
)

select opt in "${options[@]}"
do
    case $REPLY in
        1) 
            echo "运行 timer_example (按 Ctrl+C 退出):"
            ./timer_example
            ;;
        2)
            echo "运行 io_example (输入内容，按 Ctrl+D 退出):"
            ./io_example
            ;;
        3)
            echo "运行 signal_example (按 Ctrl+C 测试):"
            ./signal_example
            ;;
        4)
            echo "运行 idle_example:"
            ./idle_example
            ;;
        5)
            echo "运行 periodic_example (按 Ctrl+C 退出):"
            ./periodic_example
            ;;
        6)
            echo "运行 prepare_check_example:"
            ./prepare_check_example
            ;;
        7)
            echo "运行 stat_example (按 Ctrl+C 退出):"
            ./stat_example
            ;;
        8)
            echo "运行 cleanup_example:"
            ./cleanup_example
            ;;
        9)
            echo "运行 async_example:"
            ./async_example
            ;;
        10)
            echo "运行 mixed_example (按 Ctrl+C 退出):"
            ./mixed_example
            ;;
        11)
            echo "运行 echo_server:"
            ./echo_server
            ;;
        12)
            echo "退出"
            break
            ;;
        *)
            echo "无效选择 $REPLY"
            ;;
    esac
    echo ""
    echo "按 Enter 继续..."
    read
done