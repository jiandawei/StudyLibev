# IO 多路复用示例程序

本目录包含四种 IO 多路复用机制的示例程序：select、poll、epoll、kqueue。

## 🔄 代码结构（重构版本）

### 公共代码
```
io_common.h    - 公共函数声明
io_common.c    - 公共函数实现
```

### 特定机制代码
```
select_example.c    - Select 特有逻辑（端口 8888）
poll_example.c      - Poll 特有逻辑（端口 8889）
epoll_example.c     - Epoll 特有逻辑（端口 8890）
kqueue_example.c    - kqueue 特有逻辑（端口 8891）
```

### 配置文件
```
Makefile      - 编译脚本
test_io.sh    - 自动化测试脚本
README.io     - 本文件
```

## 🎯 重构优势

### 代码简化

| 文件 | 重构前行数 | 重构后行数 | 减少量 |
|------|-----------|-----------|--------|
| `select_example.c` | ~200 | ~150 | -25% |
| `poll_example.c` | ~200 | ~150 | -25% |
| `epoll_example.c` | ~200 | ~140 | -30% |
| `kqueue_example.c` | ~200 | ~160 | -20% |
| **总计** | **~800** | **~600** | **-25%** |

### 公共函数

所有示例共用以下功能：
- `io_set_nonblocking()` - 设置非阻塞模式
- `io_create_server_socket()` - 创建服务器 socket
- `io_getClientAddrStr()` - 获取客户端地址
- `io_send_welcome_message()` - 发送欢迎消息
- `io_handle_client_data()` - Echo 处理（核心逻辑）
- `io_close_socket()` - 优雅关闭

### 机制特定逻辑

每个示例专注于自己的 IO 机制：

**Select 特有**：
- `fd_set` 位图管理
- `get_max_fd()` 函数
- 手动维护 `readfds`

**Poll 特有**：
- `struct pollfd` 数组
- `add_poll_fd()` / `remove_poll_fd()`
- 事件掩码处理

**Epoll 特有**：
- `epoll_ctl()` 操作
- 边缘触发（EPOLLET）
- 批量事件获取

**kqueue 特有**：
- `kevent()` 事件过滤
- 事件类型（EVFILT_READ 等）
- 事件数据统计

## 📦 编译和使用

### 编译所有示例
```bash
cd examples/io
make
```

### 编译单个示例
```bash
make select_example
make poll_example
make epoll_example  # 仅限 Linux
make kqueue_example
```

### 清理编译文件
```bash
make clean
```

## 🚀 运行示例

### 基础测试

```bash
# 启动任意服务器
./select_example

# 在另一个终端连接
nc localhost 8888
Hello
# 应该收到：Hello
```

### 比较不同机制

| 机制 | 文件大小 | 特殊特性 | 学习要点 |
|------|---------|----------|----------|
| **Select** | 最小 | 位图管理、FD 限制 | 基础概念 |
| **Poll** | 较小 | 无 FD 限制 | 中等规模 |
| **Epoll** | 中等 | 边缘触发、高并发 | 高性能 |
| **kqueue** | 较小 | 多事件类型、优雅设计 | BSD/macOS |

## 🧪 测试脚本

使用自动化测试脚本：
```bash
./test_io.sh
```

这会：
1. 编译所有示例
2. 检查程序启动
3. 显示测试方法

## 📊 对比不同机制

运行不同服务器，观察差异：

```bash
# 终端 1: Select
./select_example

# 终端 2: Poll
./poll_example

# 终端 3: kqueue  
./kqueue_example

# 然后分别连接测试
```

## 🔧 代码学习路径

### 初学者路径
1. **从 select 开始**：概念简单，容易理解
2. **学习 poll**：解决 FD 限制问题
3. **根据平台**：
   - Linux: 学习 epoll
   - macOS/BSD: 学习 kqueue

### 进阶路径
- 比较不同机制的输出信息
- 理解事件传递差异
- 实验边缘触发 vs 水平触发
- 添加自定义功能

## 📝 编码技巧

### 如何使用公共代码

```c
// 1. 包含公共头文件
#include "io_common.h"

// 2. 使用公共函数
int server_fd = io_create_server_socket(8888);

// 3. 处理客户端数据
int ret = io_handle_client_data(client_fd);

// 4. 优雅关闭
io_close_socket(fd);
```

### 专注特定机制

```c
// Select 特有：维护 fd_set
FD_ZERO(&readfds);
FD_SET(server_fd, &readfds);
select(..., &tempfds, ...);

// Poll 特有：管理数组
struct pollfd fds[MAX];
poll(fds, count, timeout);

// Epoll 特有：内核事件
epoll_wait(epfd, events, MAX, timeout);

// kqueue 特有：事件过滤
kevent(kq, NULL, 0, events, MAX, &timeout);
```

## 🎯 学习重点

### 公共部分（理解一次即可）
- TCP 服务器创建
- 非阻塞 IO 设置
- Echo 服务实现
- 错误处理
- 资源清理

### 机制特定部分（需要对比学习）
- FD 管理方式
- 事件检测机制
- 性能差异
- 平台限制

### 重构学习（实战技能）
- 代码复用技巧
- 接口设计
- 模块化编程
- 跨平台适配

## 💡 扩展思路

基于公共代码，可以轻松扩展：

1. **添加 HTTP 支持**
   - 修改 `io_handle_client_data()` 解析 HTTP
   - 保持 IO 机制不变

2. **添加 SSL/TLS**
   - 使用公共的 IO 函数
   - 在 `io_create_server_socket()` 后启用 SSL

3. **添加连接池**
   - 在 `io_handle_client_data()` 添加客户端管理系统
   - 所有机制通用

4. **性能统计**
   - 在公共函数中添加统计
   - 比较不同机制性能

## ⚠️ 注意事项

### 平台兼容性
- Epoll 仅限 Linux
- kqueue 仅限 BSD/macOS
- Select/Poll 跨平台

### 性能考虑
- 小规模（<100 FD）：Select 足够
- 中等规模（100-1000 FD）：Poll 良好
- 大规模（>1000 FD）：Epoll/kqueue

### 编译顺序
```bash
# 先编译公共代码，再编译各个示例
make clean
make all  # 会自动处理依赖关系
```

## 📚 参考资源

- [libev 文档](../libev学习指南.md) - 深入学习 libev
- [IO 多路复用对比](../io多路复用机制对比.md) - 详细技术对比
- 系统编程书籍：第 11 章
- 在线文档：`man 2 select`, `man 2 poll`, `man 7 epoll`, `man 2 kqueue`

## 🎓 总结

通过这次重构：
- ✅ 代码量减少 25%
- ✅ 消除重复逻辑
- ✅ 便于维护和扩展
- ✅ 更容易理解不同机制的差异
- ✅ 公共代码质量更高

每个示例现在更加专注，更容易学习和理解不同 IO 多路复用机制的核心特性！

## 编译

###编译所有示例：
```bash
make
```

###编译单个示例：
```bash
make select_example
make poll_example
make epoll_example  # 仅限 Linux
make kqueue_example
```

###清理编译文件：
```bash
make clean
```

## 运行

每个示例都是独立的服务器程序，监听不同的端口。建议在一个终端运行服务器，另一个终端使用 `nc` 测试。

### Select 示例
```bash
# 终端 1: 运行服务器
./select_example

# 终端 2: 连接服务器
nc localhost 8888

# 在 nc 中输入文本，服务器会 echo 回去
```

### Poll 示例
```bash
# 终端 1: 运行服务器
./poll_example

# 终端 2: 连接服务器
nc localhost 8889
```

### Epoll 示例（仅限 Linux）
```bash
# 终端 1: 运行服务器
./epoll_example

# 终端 2: 连接服务器
nc localhost 8890
```

### kqueue 示例
```bash
# 终端 1: 运行服务器
./kqueue_example

# 终端 2: 连接服务器
nc localhost 8891
```

## 同时运行多个服务器

如果你想在不同的终端测试多种机制：

```bash
# 终端 1: Select
./select_example

# 终端 2: Poll
./poll_example

# 终端 3: kqueue
./kqueue_example

# 然后分别连接：
# nc localhost 8888  # Select
# nc localhost 8889  # Poll
# nc localhost 8891  # kqueue
```

## 示例功能

所有示例程序都具有以下功能：

1. ✅ TCP 服务器，监听指定端口
2. ✅ 非阻塞 IO，支持并发连接
3. ✅ 欢迎消息
4. ✅ Echo 服务（回射收到的数据）
5. ✅ 连接统计信息
6. ✅ 优雅的错误处理
7. ✅ Ctrl+C 退出

## 平台兼容性

| 示例 | Linux | macOS (BSD) | 说明 |
|------|-------|------------|------|
| select_example | ✅ | ✅ | 所有 Unix 平台 |
| poll_example | ✅ | ✅ | 大多数 Unix 平台 |
| epoll_example | ✅ | ❌ | 仅 Linux |
| kqueue_example | ❌ | ✅ | BSD/macOS |

## 测试方法

### 基础测试
```bash
# 连接服务器并发送消息
echo "Hello World" | nc localhost 8888

# 交互式测试
nc localhost 8888
# 然后输入文本，按回车发送
```

### 并发连接测试
```bash
# 使用多个 nc 实例同时连接
nc localhost 8888 &  # 后台运行
nc localhost 8888 &
nc localhost 8888 &

# 发送数据到不同的连接
echo "Message 1" > /proc/$(pgrep nc | head -1)/fd/1
echo "Message 2" > /proc/$(pgrep nc | tail -1)/fd/1
```

### 压力测试（简单）
```bash
# 使用脚本创建多个连接
for i in {1..10}; do
    (echo "Client $i" | nc localhost 8888) &
done
```

## 代码特点

### Select 示例
- 使用 `fd_set` 位图管理 FD
- 遵循经典 select 模式
- 展示 FD_SET/FD_ISSET 的使用

### Poll 示例
- 使用 `struct pollfd` 数组
- 动态管理 FD 集合
- 支持任意数量的 FD

### Epoll 示例
- 使用边缘触发（EPOLLET）
- 展示 `epoll_wait` 的批处理能力
- O(1) 复杂度

### kqueue 示例
- 使用 `struct kevent`
- 展示 kevent 的事件过滤机制
- 支持多种事件类型

## 性能对比

你可以通过观察每种机制的输出消息来对比：

```bash
# Select 遍历所有 FD
# Poll 遍历所有 FD
# Epoll 只遍历就绪的 FD
# kqueue 只遍历就绪的 FD
```

## 学习要点

### 1. 非阻塞 IO
所有示例都使用非阻塞 socket，这是高性能服务的基础。
```c
set_nonblocking(fd);
```

### 2. 事件循环
每种机制都有类似的循环模式：
```c
while (running) {
    // 等待事件
    ret = mechanism_wait(...);
    
    // 处理就绪事件
    for (i = 0; i < ret; i++) {
        // 处理事件
    }
}
```

### 3. 错误处理
包括信号中断（EINTR）、连接关闭、读写错误等。

### 4. 资源管理
正确的 FD 关闭和清理。

## 常见问题

### Q: 为什么 epoll 在 macOS 上无法编译？
A: Epoll 是 Linux 特有的系统调用，macOS 使用 kqueue。

### Q: 如何处理大量的并发连接？
A: 在 Linux 上使用 epoll，在 macOS 上使用 kqueue。

### Q: 这些示例能处理多少连接？
A: 理论上非常取决于配置，默认选择可以轻松处理上千连接。

### Q: 如何修改端口？
A:编辑源码中的端口号常量：
```c
#define SERVER_PORT 8888  // 修改这里
```

## 扩展练习

1. **添加日志**：记录每个连接的生命周期
2. **超时处理**：添加连接超时机制
3. **数据统计**：统计传输的字节数、连接数等
4. **多线程**：使用多线程处理读写
5. **HTTP 服务**：实现简单的 HTTP 服务器

## 参考资料

- [Select 文档](https://man7.org/linux/man-pages/man2/select.2.html)
- [Poll 文档](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Epoll 文档](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [Kqueue 教程](https://www.freebsd.org/cgi/man.cgi?query=kqueue&sektion=2)

## 作者

这些示例程序用于学习和理解不同的 IO 多路复用机制。

**注意**：这些是教学示例，生产环境建议使用更成熟的开源库（如 libev, libevent, libuv 等）。