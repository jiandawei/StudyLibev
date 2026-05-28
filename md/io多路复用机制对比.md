# IO 多路复用机制对比

## 概述

IO 多路复用是一种同步 IO 模型，允许一个进程同时监控多个文件描述符（FD），当某个 FD 就绪时通知应用程序。常见的四种机制：select、poll、epoll、kqueue。

## 四种机制对比

| 特性 | select | poll | epoll (Linux) | kqueue (BSD/macOS) |
|------|--------|------|---------------|---------------------|
| **平台** | 所有 Unix | 所有 Unix | Linux 2.6+ | BSD, macOS |
| **性能** | O(n) | O(n) | O(1) | O(1) |
| **FD 上限** | 1024 (默认) | 无限制 | 无限制 | 无限制 |
| **事件驱动** | 否 | 否 | 是 | 是 |
| **边缘触发** | 不支持 | 不支持 | 支持 | 支持 |
| **跨进程传递** | 支持 | 支持 | 支持（较复杂）| 支持 |
| **复杂度** | 简单 | 简单 | 中等 | 中等 |
| **主要缺点** | 性能差、FD 限制 | 性能差 | 仅限 Linux | 仅限 BSD/macOS |

## Select

### 原理

Select 使用位图（bitmap）来表示一组文件描述符，通过遍历整个位图来检查哪些 FD 就绪。

### 函数接口

```c
#include <sys/select.h>
#include <sys/time.h>

int select(int nfds,
           fd_set *readfds,
           fd_set *writefds,
           fd_set *exceptfds,
           struct timeval *timeout);
```

### 主要特点

- ✅ 跨平台兼容性最好
- ✅ API 简单易用
- ❌ FD 数量有限（默认 1024）
- ❌ 每次调用都要重置 FD_SET
- ❌ O(n) 复杂度，性能随 FD 数量下降

### 适用场景

- FD 数量较少（< 100）
- 需要跨平台兼容
- 简单的并发连接

## Poll

### 原理

Poll 使用结构体数组来监控多个 FD，遍历数组检查就绪状态。

### 函数接口

```c
#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout);

struct pollfd {
    int   fd;         // 文件描述符
    short events;     // 关注的事件
    short revents;    // 返回的事件
};
```

### 主要特点

- ✅ 无 FD 数量限制
- ✅ 跨平台兼容性较好
- ✅ API 比 select 简单
- ❌ O(n) 复杂度
- ❌ 需要遍历整个数组

### 适用场景

- FD 数量中等（100-1000）
- 需要跨平台兼容
- 不需要极高的性能

## Epoll

### 原理

Epoll 基于事件驱动，内核维护一个就绪链表，只有就绪的 FD 会被通知。

### 函数接口

```c
#include <sys/epoll.h>

// 创建 epoll 实例
int epoll_create1(int flags);

// 控制 epoll 实例
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

// 等待事件
int epoll_wait(int epfd, struct epoll_event *events,
               int maxevents, int timeout);

struct epoll_event {
    uint32_t     events;     // 事件类型
    epoll_data_t data;       // 用户数据
};
```

### 工作模式

**水平触发（LT，默认）**：
- 只要 FD 就绪，epoll_wait 就会返回
- 每次返回后如果没有处理完，下次还会返回
- 行为与 select/pol 相同

**边缘触发（ET）**：
- 只有 FD 状态改变时才返回一次
- 需要一次性处理完所有数据
- 性能更高，但编程更复杂

### 主要特点

- ✅ O(1) 复杂度，性能优秀
- ✅ 支持大量 FD
- ✅ 支持边缘触发
- ✅ 支持 mmap 和信号通知
- ❌ 仅限 Linux
- ❌ API 相对复杂

### 适用场景

- 海量连接（10K+ 连接）
- 高性能服务器
- Linux 专用应用

## Kqueue

### 原理

Kqueue 是 BSD/macOS 的高性能事件通知机制，内核维护事件队列。

### 函数接口

```c
#include <sys/event.h>

// 创建 kqueue 实例
int kqueue(void);

// 控制 kqueue 实例
int kevent(int kq,
           const struct kevent *changelist, int nchanges,
           struct kevent *eventlist, int nevents,
           const struct timespec *timeout);

struct kevent {
    uintptr_t  ident;  // 文件描述符或信号
    short      filter; // 事件过滤器
    u_short    flags;  // 操作标志
    u_int      fflags; // 过滤器特定标志
    int64_t    data;   // 过滤器特定数据
    void       *udata; // 用户数据
};
```

### 主要特点

- ✅ O(1) 复杂度
- ✅ 支持 IO、信号、定时器、文件系统事件
- ✅ API 设计优雅
- ✅ 跨进程传递简单
- ❌ 仅限 BSD/macOS

### 适用场景

- BSD/macOS 系统开发
- 复杂事件处理（包括非 IO 事件）
- 高性能服务

## 性能对比

### 时间复杂度

| 机制 | FD 添加/删除 | FD 就绪检查 | 总体性能 |
|------|------------|------------|----------|
| select | O(1) | O(n) | ↓ 随 FD 增加下降 |
| poll | O(1) | O(n) | ↓ 随 FD 增加下降 |
| epoll | O(log n) | O(1) | ⚡ 良好，不受 FD 数量影响 |
| kqueue | O(log n) | O(1) | ⚡ 良好，不受 FD 数量影响 |

### 实际测试场景

假设处理 1000 个 FD：

| 进程 | FD 数量 | select (ms) | poll (ms) | epoll (ms) | kqueue (ms) |
|------|--------|-------------|-----------|-------------|-------------|
| 单次遍历 | 100 | 0.5 | 0.4 | 0.3 | 0.3 |
| 单次遍历 | 1000 | 15 | 12 | 0.5 | 0.5 |
| 频繁事件 | 1000 | 25 | 20 | 1.5 | 1.5 |
| 稀疏事件 | 10000/+ | >100 | >100 | 2 | 2 |

*注意：实际性能取决于硬件、内核版本、事件模式等*

## 选择指南

### 选择 Select

```c
// 当满足以下条件时选择：
// 1. FD 数量 < 100
// 2. 需要最大跨平台兼容性
// 3. 代码简单性是首要考虑
// 4. 不需要高性能

fd_set readfds;
FD_ZERO(&readfds);
FD_SET(fd, &readfds);
select(fd + 1, &readfds, NULL, NULL, NULL);
```

### 选择 Poll

```c
// 当满足以下条件时选择：
// 1. FD 数量 100 - 1000
// 2. 需要 Unix 跨平台兼容（但不需要 Windows）
// 3. 超过 select 的 FD 限制
// 4. 需要监控大量特殊 FD

struct pollfd fds[100];
fds[0].fd = fd;
fds[0].events = POLLIN;
poll(fds, 1, timeout);
```

### 选择 Epoll

```c
// 当满足以下条件时选择：
// 1. FD 数量 > 1000
// 2. Linux 平台
// 3. 需要高性能
// 4. 连接数动态变化

int epfd = epoll_create1(0);
struct epoll_event ev;
ev.events = EPOLLIN;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
epoll_wait(epfd, &ev, 1, timeout);
```

### 选择 Kqueue

```c
// 当满足以下条件时选择：
// 1. BSD/macOS 平台
// 2. 需要监控 IO、信号、定时器等多种事件
// 3. 需要高性能
// 4. 需要 watch 文件系统变化

int kq = kqueue();
struct kevent change;
EV_SET(&change, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
kevent(kq, &change, 1, &event, 1, &timeout);
```

## 最佳实践

### 性能优化

1. **避免不必要的 FD 监控**
   ```c
   // ❌ 错误：监控从不活动的 FD
   select(maxfd + 1, &all_fds, NULL, NULL, NULL);
   
   // ✅ 正确：只监控活跃的 FD
   select(max_active_fd + 1, &active_fds, NULL, NULL, NULL);
   ```

2. **合理设置超时**
   ```c
   // 示例：根据业务需求调整超时
   struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
   select(maxfd + 1, &readfds, NULL, NULL, &tv);
   ```

3. **使用边缘触发（epoll）**
   ```c
   // 高性能场景使用 ET 模式
   struct epoll_event ev;
   ev.events = EPOLLIN | EPOLLET;  // 边缘触发
   epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
   ```

### 错误处理

```c
// 统一错误处理示例
if (select_result < 0) {
    if (errno == EINTR) {
        // 被信号中断，可以继续
        continue;
    } else {
        // 其他错误
        perror("select");
        exit(EXIT_FAILURE);
    }
}
```

### 跨平台适配

```c
// 平台检测
#ifdef __linux__
    // 使用 epoll
    int epfd = epoll_create1(0);
#elif defined(__APPLE__) || defined(__FreeBSD__)
    // 使用 kqueue
    int kq = kqueue();
#else
    // 回退到 poll
    poll(fds, nfds, timeout);
#endif
```

## 总结

| 机制 | 推荐场景 | 不推荐场景 |
|------|---------|-----------|
| **select** | 小规模、跨平台 | 大规模、高性能 |
| **poll** | 中等规模、Unix 跨平台 | Linux/BSD 专用、高性能 |
| **epoll** | Linux、大规模连接 | Windows/跨平台 |
| **kqueue** | BSD/macOS、复杂事件 | Linux/跨平台 |

### 性能排序（从高到低）

epoll ≈ kqueue > poll > select

### 兼容性排序（从高到低）

select > poll > epoll (Linux) > kqueue (BSD/macOS)

### 复杂度排序（从低到高）

select ≈ poll < epoll < kqueue

选择合适的 IO 多路复用机制需要根据平台、性能需求、开发成本等因素综合考虑。在现代高性能服务器开发中，Linux 首选 epoll，BSD/macOS 首选 kqueue，跨平台需求考虑 poll（中等规模）或使用 libev 等库。