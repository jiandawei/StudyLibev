# libev Tutorial Examples

这是一个包含 libev 库源代码和详细教程示例的仓库，适合学习 libev 事件循环库的使用。

## 仓库结构

```
.
├── .gitignore              # Git 忽略文件配置
├── README.md              # 本文件
├── libev-master/          # libev 源代码和官方文档
├── examples/              # 示例程序
│   ├── io_multiplex/     # I/O 多路复用教程
│   └── libev_tutorial/   # libev 完整教程示例
└── md/                   # 中文文档
    ├── io多路复用模型对比.md
    └── libev学习指南.md
```

## libev 教程示例

包含了 11 种不同类型 watcher 的示例程序，从基础到高级：

### 基础示例
- `timer_example.c` - 定时器 watcher 示例
- `io_example.c` - I/O watcher 示例
- `signal_example.c` - 信号 watcher 示例

### 高级示例
- `idle_example.c` - Idle watcher 示例
- `periodic_example.c` - 周期性定时器示例
- `prepare_check_example.c` - Prepare/Check watcher 示例
- `stat_example.c` - 文件状态 watcher 示例
- `cleanup_example.c` - Cleanup watcher 示例
- `async_example.c` - 异步事件示例
- `child_example.c` - 子进程监控示例
- `fork_example.c` - Fork 事件示例
- `embed_example.c` - 嵌入循环示例

### 综合示例
- `mixed_example.c` - 混合使用多种 watcher 的示例
- `echo_server.c` - TCP 回显服务器实现

## 编译和运行

### 使用提供的编译脚本

```bash
cd examples/libev_tutorial
./build.sh
```

### 手动编译

```bash
cd examples/libev_tutorial
gcc -Wall -I../../libev-master -L../../libev-master -lev -o timer_example timer_example.c
```

### 运行示例

```bash
# 基础示例
./timer_example
./io_example
./signal_example

# 高级示例（需要线程支持的）
./async_example

# 子进程示例
./child_example
./fork_example

# 嵌入循环示例
./embed_example
```

### 使用交互式测试脚本

```bash
cd examples/libev_tutorial
./interactive_test.sh
```

## Git 仓库信息

这个仓库已经初始化并准备好推送到远程。当前状态：

- 分支: `main`
- 提交: 2 个提交
- 远程: 未配置 (需要手动添加)

### 配置和推送到远程

```bash
# 设置你的 Git 用户信息（如果还没有设置）
git config user.name "你的名字"
git config user.email "你的邮箱"

# 添加远程仓库
git remote add origin <你的远程仓库URL>

# 推送到远程
git push -u origin main
```

### 推送到 GitHub

如果你使用 GitHub：

1. 在 GitHub 上创建一个新的仓库
2. 复制仓库 URL
3. 运行：

```bash
git remote add origin https://github.com/你的用户名/你的仓库名.git
git branch -M main
git push -u origin main
```

## 学习资源

### I/O 多路复用
- `md/io多路复用模型对比.md` - 详细的 I/O 多路复用模型对比
- `examples/io_multiplex/` - 包含 select、poll、epoll、kqueue 的实现示例

### libev 学习指南
- `md/libev学习指南.md` - libev 学习指南
- `libev-master/ev.3` - libev 官方手册页
- `libev-master/README` - libev 官方文档

## 开发环境要求

- C 编译器 (gcc/clang)
- libev 库
- pthread 库 (用于 async_example)
- 系统：Linux/macOS/Unix-like 系统

## 注意事项

1. 某些示例需要特定权限（如信号处理）
2. 某些示例需要手动交互（如 io_example）
3. async_example 需要正确配置 pthread 库
4. 在 macOS 上运行时，部分系统调用可能略有不同

## 许可证

libev 库遵循其原始许可证（参见 libev-master/LICENSE）。
教程示例为教育目的而创建，可以自由使用。

## 贡献

欢迎提交问题报告和改进建议！

---

## 最近更新

- 2024-05-28: 初始化仓库，添加完整的 libev 教程示例
- 包含 11 种不同类型 watcher 的详细示例
- 添加中英文文档和 I/O 多路复用对比教程# 同步测试
