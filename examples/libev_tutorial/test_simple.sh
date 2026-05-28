#!/bin/bash

# 快速测试脚本 - 运行简单示例
# 注意：某些示例可能需要手动交互

echo "libev 示例程序测试"
echo "==================="
echo ""

# 测试timer_example（运行2秒后自动退出）
echo "1. 测试 timer_example（2秒后自动退出）："
timeout 2 ./timer_example
echo ""
echo ""

# 测试idle_example（会自动退出）
echo "2. 测试 idle_example："
./idle_example
echo ""
echo ""

# 测试periodic_example（运行5秒后自动退出）
echo "3. 测试 periodic_example（5秒后自动退出）："
timeout 5 ./periodic_example
echo ""
echo ""

# 测试prepare_check_example
echo "4. 测试 prepare_check_example："
./prepare_check_example
echo ""
echo ""

# 测试cleanup_example
echo "5. 测试 cleanup_example："
./cleanup_example
echo ""
echo ""

# 测试async_example
echo "6. 测试 async_example："
./async_example
echo ""
echo ""

# 测试child_example
echo "7. 测试 child_example："
./child_example
echo ""
echo ""

# 测试fork_example
echo "8. 测试 fork_example："
./fork_example
echo ""
echo ""

# 测试embed_example
echo "9. 测试 embed_example："
./embed_example
echo ""
echo ""

echo "简单示例测试完成！"
echo ""
echo "以下示例需要手动交互："
echo "- io_example:       运行 ./io_example，然后输入内容"
echo "- signal_example:   运行 ./signal_example，然后按 Ctrl+C 测试"
echo "- stat_example:     运行 ./stat_example，然后在另一个终端修改文件"
echo "- mixed_example:    运行 ./mixed_example 测试多种watcher"
echo "- echo_server:      运行 ./echo_server，然后用 nc localhost 12345 连接"