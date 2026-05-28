# libev Tutorial 示例程序

本目录包含了各种 libev watcher 的示例程序，帮助你学习和理解 libev 的使用。

## 示例程序列表

### 基础示例

1. **timer_example.c** - 定时器 watcher 示例
   - 演示如何使用 ev_timer 创建周期性定时器
   - 每秒触发一次，打印当前时间

2. **io_example.c** - I/O watcher 示例
   - 演示如何使用 ev_io 监控标准输入
   - 当用户在终端输入内容时，程序会读取并显示输入的内容

3. **signal_example.c** - 信号 watcher 示例
   - 演示如何使用 ev_signal 捕获系统信号
   - 可以捕获 SIGINT 和 SIGTERM 信号

### 高级示例

4. **idle_example.c** - Idle watcher 示例
   - 演示如何使用 ev_idle 在事件循环空闲时执行任务
   - 执行10次后自动退出

5. **periodic_example.c** - 周期性定时器示例
   - 演示如何使用 ev_periodic 创建更精确的周期性定时器
   - 每2秒触发一次，比 ev_timer 更适合需要严格周期的场景

6. **prepare_check_example.c** - Prepare/Check watcher 示例
   - 演示如何使用 ev_prepare 和 ev_check 在事件循环的特定阶段执行回调
   - prepare 在每次事件迭代开始前调用
   - check 在每次事件迭代结束后调用

7. **stat_example.c** - 文件状态 watcher 示例
   - 演示如何使用 ev_stat 监控文件的变化
   - 当文件大小或修改时间发生变化时触发回调

8. **cleanup_example.c** - Cleanup watcher 示例
   - 演示如何使用 ev_cleanup 在事件循环退出前执行清理工作
   - 适合用于资源释放和清理操作

9. **async_example.c** - Async watcher 示例
   - 演示如何使用 ev_async 从其他线程触发事件循环中的回调
   - 适合用于线程间通信
   - 演示如何使用 ev_cleanup 在事件循环退出前执行清理工作
   - 适合用于资源释放和清理操作

### 综合示例

10. **mixed_example.c** - 混合 watcher 示例
   - 演示如何在一个程序中同时使用多种类型的 watcher
   - 展示不同类型 watcher 之间的协作

11. **echo_server.c** - 回显服务器示例
    - 实现了一个简单的 TCP 回显服务器
    - 综合使用 I/O watcher 处理客户端连接

## 编译和运行

### 编译所有示例

```bash
make
```

### 编译单个示例

```bash
gcc -Wall -I../.. -L../.. -lev -o io_example io_example.c
```

### 运行示例

```bash
./timer_example
./io_example
./signal_example
```

## 注意事项

1. 编译时需要确保 libev 库已经安装在系统中
2. 某些示例可能需要特定的环境或权限
3. signal_example 的运行可能需要额外的信号操作知识
4. stat_example 需要同时运行另一个程序来修改文件才能看到效果

## 学习路径

建议按以下顺序学习这些示例：

1. timer_example - 最简单的开始
2. io_example - 学习 I/O 事件处理
3. idle_example - 理解空闲时的事件处理
4. periodic_example - 学习更精确的定时器
5. prepare_check_example - 理解事件循环的各个阶段
6. stat_example - 学习文件系统事件
7. cleanup_example - 学习资源清理
8. signal_example - 学习信号处理
9. mixed_example - 综合运用多种 watcher
10. echo_server - 实际应用案例

## 更多信息

关于 libev 的详细文档，请参考：
- libev 官方文档
- 源代码中的注释和示例