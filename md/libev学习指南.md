# libev 学习指南

## 概述

libev 是一个高性能的事件循环库，支持多种后端机制（epoll, kqueue, select, poll, port 等）。

## 源码学习路径

### 1. 核心文件结构

```
├── ev.h           # 主要头文件，API定义
├── ev.c           # 核心实现（~5000行）
├── ev++.h         # C++ 接口封装
├── ev_vars.h      # 事件循环变量定义（自动生成）
├── ev_wrap.h      # 宏包装（自动生成，不要手动编辑）
├── event.h/event.c # libevent 兼容层
└── ev_*_backends  # 各平台后端实现
    ├── ev_epoll.c    # Linux epoll
    ├── ev_kqueue.c   # BSD/macOS kqueue
    ├── ev_port.c     # Solaris event ports
    ├── ev_poll.c     # poll
    └── ev_select.c   # select
```

### 2. 学习顺序建议

**第一阶段：基础概念**
- 阅读 `ev.h` 的前 200 行，理解基本概念：
  - `struct ev_loop`：事件循环结构
  - Watcher 类型：`ev_io`, `ev_timer`, `ev_signal`, `ev_child` 等
  - 优先级系统：EV_MINPRI 到 EV_MAXPRI

**第二阶段：核心机制**
- 学习 `ev.c` 中的关键函数：
  - `ev_run()`：主事件循环
  - `ev_loop_init()`：循环初始化
  - `ev_io_start()/ev_io_stop()`：IO watcher 管理
  - 后端选择机制：`ev_supported_backends()`, `ev_recommended_backends()`

**第三阶段：后端实现**
- 选择一个后端深入学习（推荐从 `ev_select.c` 开始，最简单）：
  - 理解如何注册事件
  - 理解如何等待和获取事件
  - 理解边界情况处理

**第四阶段：高级特性**
- 定时器管理
- 信号处理
- 子进程监控
- 跨平台兼容性处理

### 3. 关键代码位置

| 功能 | 文件 | 行数范围 |
|------|------|----------|
| 主循环 | ev.c | ~3000-4000 |
| IO watcher | ev.c | ~2000-2500 |
| Timer watcher | ev.c | ~1500-2000 |
| 后端注入 | ev.c | ~4000-4500 |
| 配置系统 | ev.c | 40-100 |
| 变量定义 | ev_vars.h | 全部 |
| API定义 | ev.h | 600-850 |

### 4. 学习注意事项

1. **条件编译**：大量使用 `#ifdef EV_USE_*` 宏来控制特性
2. **自动生成文件**：`ev_vars.h` 和 `ev_wrap.h` 是自动生成的，不要直接编辑
3. **多后端支持**：代码在运行时根据平台选择最优后端
4. **兼容性**：支持 libevent API（event.c/h）

## 可执行示例

### 示例 1：基础 IO 监控

监听标准输入，当有数据可读时打印消息。

```c
#include <ev.h>
#include <stdio.h>
#include <unistd.h>

struct ev_io io_watcher;

static void io_cb(EV_P_ struct ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("收到输入: %s", buf);
    } else {
        ev_io_stop(EV_A_ w);
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_io_init(&io_watcher, io_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &io_watcher);

    printf("请输入内容（Ctrl+D 退出）：\n");
    ev_run(loop, 0);

    return 0;
}
```

### 示例 2：定时器

创建一个定时器，每秒打印一次消息。

```c
#include <ev.h>
#include <stdio.h>
#include <time.h>

struct ev_timer timer_watcher;

static void timer_cb(EV_P_ struct ev_timer *w, int revents) {
    time_t now = time(NULL);
    printf("定时器触发: %s", ctime(&now));
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_timer_init(&timer_watcher, timer_cb, 1.0, 1.0);  // 1秒后开始，之后每1秒触发
    ev_timer_start(loop, &timer_watcher);

    printf("定时器启动，按 Ctrl+C 退出\n");
    ev_run(loop, 0);

    return 0;
}
```

### 示例 3：信号处理

捕获 SIGINT 和 SIGTERM 信号。

```c
#include <ev.h>
#include <stdio.h>
#include <signal.h>

struct ev_signal signal_watcher;

static void signal_cb(EV_P_ struct ev_signal *w, int revents) {
    printf("收到信号 %d，准备退出...\n", w->signum);
    ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_signal_init(&signal_watcher, signal_cb, SIGINT);
    ev_signal_start(loop, &signal_watcher);

    printf("等待信号（按 Ctrl+C）...\n");
    ev_run(loop, 0);

    printf("程序退出\n");
    return 0;
}
```

### 示例 4：混合使用（IO + 定时器）

同时监听键盘输入和定时器。

```c
#include <ev.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

struct ev_io io_watcher;
struct ev_timer timer_watcher;

static void io_cb(EV_P_ struct ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf(">>> 你的输入: %s", buf);
    }
}

static void timer_cb(EV_P_ struct ev_timer *w, int revents) {
    time_t now = time(NULL);
    printf("[定时器] 当前时间: %s", ctime(&now));
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    // 设置 IO watcher
    ev_io_init(&io_watcher, io_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &io_watcher);

    // 设置定时器
    ev_timer_init(&timer_watcher, timer_cb, 2.0, 2.0);  // 2秒后开始，之后每2秒触发
    ev_timer_start(loop, &timer_watcher);

    printf("程序启动（按 Ctrl+C 退出）：\n");
    ev_run(loop, 0);

    // 清理
    ev_io_stop(loop, &io_watcher);
    ev_timer_stop(loop, &timer_watcher);

    return 0;
}
```

### 示例 5：简单的 TCP 服务器

一个简单的 Echo 服务器。

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 10

int listen_fd;
struct ev_io accept_watcher;
struct ev_io *client_watchers[MAX_CLIENTS] = {0};

static void client_cb(EV_P_ struct ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf));

    if (n <= 0) {
        printf("客户端断开连接\n");
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }

    printf("收到数据 (%d 字节): %.*s\n", n, n, buf);

    // Echo 回去
    write(w->fd, buf, n);
}

static void accept_cb(EV_P_ struct ev_io *w, int revents) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(w->fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd < 0) {
        perror("accept");
        return;
    }

    printf("新客户端连接: %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // 创建客户端 watcher
    struct ev_io *client_watcher = malloc(sizeof(struct ev_io));
    ev_io_init(client_watcher, client_cb, client_fd, EV_READ);
    ev_io_start(loop, client_watcher);

    // 保存到数组
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_watchers[i] == NULL) {
            client_watchers[i] = client_watcher;
            break;
        }
    }
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);
    struct sockaddr_in addr;

    // 创建监听 socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Echo 服务器启动，监听端口 %d\n", PORT);

    // 设置 accept watcher
    ev_io_init(&accept_watcher, accept_cb, listen_fd, EV_READ);
    ev_io_start(loop, &accept_watcher);

    // 运行事件循环
    ev_run(loop, 0);

    // 清理
    printf("服务器关闭\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_watchers[i]) {
            ev_io_stop(loop, client_watchers[i]);
            close(client_watchers[i]->fd);
            free(client_watchers[i]);
        }
    }
    close(listen_fd);

    return 0;
}
```

## 编译和运行

### 编译命令

```bash
# 编译基础示例
gcc -o io_example io_example.c -lev

# 编译定时器示例
gcc -o timer_example timer_example.c -lev

# 编译信号示例
gcc -o signal_example signal_example.c -lev

# 编译混合示例
gcc -o mixed_example mixed_example.c -lev

# 编译 TCP 服务器
gcc -o echo_server echo_server.c -lev
```

### 运行示例

```bash
# 运行 IO 示例
./io_example

# 运行定时器（Ctrl+C 退出）
./timer_example

# 运行信号处理（Ctrl+C 触发）
./signal_example

# 运行混合示例
./mixed_example

# 运行 TCP 服务器（新终端连接：nc localhost 8080）
./echo_server
```

## 构建系统

### 从源码构建 libev

```bash
# 1. 生成 configure 脚本
./autogen.sh

# 2. 配置
./configure

# 3. 编译和安装
make
sudo make install
```

### 关键特性标志

编译时可以通过定义以下宏来控制特性：

- `EV_STANDALONE`：不依赖 autoconf，使用默认配置
- `EV_USE_EPOLL`：强制使用 epoll 后端
- `EV_USE_KQUEUE`：强制使用 kqueue 后端
- `EV_FEATURES`：控制特性集（0x7f 全开，0x7c 关闭 API）

## 调试建议

1. **启用详细输出**：
```c
#define EV_VERIFY 1  // 启用验证
#include "ev.h"
```

2. **检查后端支持**：
```c
printf("支持的后端: %08x\n", ev_supported_backends());
printf("推荐的后端: %08x\n", ev_recommended_backends());
```

3. **使用 ev_verify()** 在关键点检查事件循环状态

## 参考资料

- 官方文档：http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod
- 主页：http://software.schmorp.de/pkg/libev
- 性能基准：http://libev.schmorp.de/bench.html