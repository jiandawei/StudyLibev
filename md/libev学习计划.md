# libev 学习计划

## 概述

本学习计划旨在帮助你系统地掌握 libev 事件循环库，从基础概念到高级特性，通过代码阅读、示例编写和项目实践三个维度进行深入学习。

### 学习目标

1. **理解事件驱动编程模型**
2. **掌握 libev 核心 API 的使用**
3. **深入理解 IO 多路复用机制**
4. **掌握定时器、信号、子进程等高级特性**
5. **能够独立开发基于 libev 的高性能网络应用**

### 前置条件

1. 熟悉 C 语言编程
2. 了解基本的 Unix/Linux 系统调用
3. 了解 TCP/IP 网络编程基础
4. 熟悉命令行工具和 Makefile

---

## 学习资源

### 官方资源
- **官方文档**: http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod
- **源码仓库**: https://github.com/enki/libev
- **官方示例**: `libev-master/examples/`

### 辅助资源
- **UNIX 网络编程**: 卷 1 第 6、16、17 章（IO 多路复用）
- **Linux 高性能服务器编程**: 第 4、5 章
- **man 手册**: `man epoll`, `man kqueue`, `man select`

---

## 学习计划（4 周）

### 第一周：基础入门

#### Day 1-2：环境搭建与基础概念

**任务目标**：搭建开发环境，理解 libev 核心概念

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 从源码编译安装 libev | `./autogen.sh && ./configure && make && sudo make install` |
| 上午 | 阅读 `ev.h` 前 200 行，理解数据结构定义 | `libev-master/libev-master/ev.h` |
| 下午 | 理解 Watcher 类型体系：ev_io, ev_timer, ev_signal | `ev.h: 200-400` |
| 下午 | 编写第一个示例：Hello Libev | 参考下方示例 1 |
| 晚上 | 编译运行示例，验证环境是否正常 | `gcc -o hello hello.c -lev` |

**示例 1：Hello Libev**

```c
#include <ev.h>
#include <stdio.h>

static void timer_cb(EV_P_ ev_timer *w, int revents) {
    printf("Hello Libev!\n");
    ev_break(EV_A_ EVBREAK_ALL);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer;
    ev_timer_init(&timer, timer_cb, 1.0, 0.0);
    ev_timer_start(loop, &timer);
    ev_run(loop, 0);
    return 0;
}
```

**学习要点**：
- `ev_loop` 事件循环结构
- `ev_timer` 定时器 Watcher
- `ev_run` 启动事件循环
- `ev_break` 退出事件循环

---

#### Day 3-4：IO 事件处理

**任务目标**：掌握 ev_io 的使用，理解 IO 多路复用原理

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中 ev_io_start/stop 实现 | `ev.c: 4007-4049` |
| 上午 | 理解 ANFD 结构和文件描述符管理 | `ev.c: 1770-1786` |
| 下午 | 编写示例：标准输入监控 | 参考下方示例 2 |
| 下午 | 编写示例：简单 TCP 客户端 | 参考下方示例 3 |
| 晚上 | 阅读 `ev_select.c`，理解 select 后端实现 | `libev-master/libev-master/ev_select.c` |

**示例 2：标准输入监控**

```c
#include <ev.h>
#include <stdio.h>
#include <unistd.h>

static void io_cb(EV_P_ ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = '\0';
        printf("收到: %s", buf);
    } else {
        ev_io_stop(EV_A_ w);
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    ev_io io_watcher;
    ev_io_init(&io_watcher, io_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &io_watcher);
    printf("输入内容（Ctrl+D 退出）：\n");
    ev_run(loop, 0);
    return 0;
}
```

**示例 3：简单 TCP 客户端**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

struct ev_io conn_watcher;
int sockfd;

static void conn_cb(EV_P_ ev_io *w, int revents) {
    char buf[1024];
    int n;
    
    if (revents & EV_READ) {
        n = read(sockfd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            printf("服务器响应: %s", buf);
        } else {
            printf("连接关闭\n");
            ev_io_stop(EV_A_ w);
            ev_break(EV_A_ EVBREAK_ALL);
        }
    }
}

int main() {
    struct sockaddr_in addr;
    struct ev_loop *loop = ev_default_loop(0);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    
    ev_io_init(&conn_watcher, conn_cb, sockfd, EV_READ);
    ev_io_start(loop, &conn_watcher);
    
    write(sockfd, "Hello Server!\n", 14);
    ev_run(loop, 0);
    
    close(sockfd);
    return 0;
}
```

**学习要点**：
- `ev_io` 的初始化和启动流程
- EV_READ/EV_WRITE 事件类型
- 文件描述符的生命周期管理
- select 后端的实现原理

---

#### Day 5-6：定时器机制

**任务目标**：掌握 ev_timer 和 ev_periodic 的使用

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中定时器相关实现 | `ev.c: 4051-4100` |
| 上午 | 理解最小堆数据结构 | `ev.c: 1805-1823` |
| 下午 | 编写示例：周期性定时器 | 参考下方示例 4 |
| 下午 | 编写示例：延迟任务队列 | 参考下方示例 5 |
| 晚上 | 阅读 `ev.c` 中的堆操作函数 | `ev.c: 2100-2200` |

**示例 4：周期性定时器**

```c
#include <ev.h>
#include <stdio.h>
#include <time.h>

static void periodic_cb(EV_P_ ev_periodic *w, int revents) {
    time_t now = time(NULL);
    printf("周期性任务: %s", ctime(&now));
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    ev_periodic periodic;
    
    // 每分钟的第0秒触发
    ev_periodic_init(&periodic, periodic_cb, 0., 60., 0);
    ev_periodic_start(loop, &periodic);
    
    printf("周期性任务启动（Ctrl+C 退出）\n");
    ev_run(loop, 0);
    return 0;
}
```

**示例 5：延迟任务队列**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    ev_timer timer;
    int task_id;
} DelayedTask;

static void task_cb(EV_P_ ev_timer *w, int revents) {
    DelayedTask *task = (DelayedTask*)w;
    printf("执行任务: %d\n", task->task_id);
    free(task);
}

static void add_delayed_task(EV_P_ double delay, int task_id) {
    DelayedTask *task = malloc(sizeof(DelayedTask));
    task->task_id = task_id;
    ev_timer_init(&task->timer, task_cb, delay, 0.);
    ev_timer_start(EV_A_ &task->timer);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    add_delayed_task(loop, 1.0, 1);  // 1秒后执行
    add_delayed_task(loop, 2.5, 2);  // 2.5秒后执行
    add_delayed_task(loop, 5.0, 3);  // 5秒后执行
    
    printf("延迟任务队列启动\n");
    ev_run(loop, 0);
    return 0;
}
```

**学习要点**：
- `ev_timer` vs `ev_periodic` 的区别
- 相对时间和绝对时间的处理
- 最小堆在定时器管理中的应用
- 定时器的启动、停止和重置

---

#### Day 7：信号处理

**任务目标**：掌握 ev_signal 的使用，理解信号处理机制

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中信号处理相关代码 | `ev.c: 4300-4400` |
| 上午 | 理解 pipe 机制在信号处理中的作用 | `ev.c: 3042-3046` |
| 下午 | 编写示例：优雅退出程序 | 参考下方示例 6 |
| 下午 | 编写示例：信号处理与资源清理 | 参考下方示例 7 |
| 晚上 | 总结本周学习内容，整理笔记 | |

**示例 6：优雅退出**

```c
#include <ev.h>
#include <stdio.h>
#include <signal.h>

struct ev_signal sigint_watcher;
struct ev_signal sigterm_watcher;

static void sigint_cb(EV_P_ ev_signal *w, int revents) {
    printf("收到 SIGINT，准备退出...\n");
    ev_break(EV_A_ EVBREAK_ALL);
}

static void sigterm_cb(EV_P_ ev_signal *w, int revents) {
    printf("收到 SIGTERM，准备退出...\n");
    ev_break(EV_A_ EVBREAK_ALL);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    ev_signal_init(&sigint_watcher, sigint_cb, SIGINT);
    ev_signal_start(loop, &sigint_watcher);
    
    ev_signal_init(&sigterm_watcher, sigterm_cb, SIGTERM);
    ev_signal_start(loop, &sigterm_watcher);
    
    printf("等待信号（Ctrl+C 或 kill 命令）\n");
    ev_run(loop, 0);
    
    printf("程序已退出\n");
    return 0;
}
```

**示例 7：信号处理与资源清理**

```c
#include <ev.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

struct ev_loop *loop;
int *resource;

static void cleanup() {
    printf("清理资源...\n");
    if (resource) free(resource);
}

static void signal_cb(EV_P_ ev_signal *w, int revents) {
    printf("收到信号 %d\n", w->signum);
    cleanup();
    ev_break(EV_A_ EVBREAK_ALL);
}

int main() {
    struct ev_signal sig_watcher;
    
    loop = ev_default_loop(0);
    resource = malloc(1024);
    
    ev_signal_init(&sig_watcher, signal_cb, SIGINT);
    ev_signal_start(loop, &sig_watcher);
    
    printf("程序运行中（Ctrl+C 退出）\n");
    ev_run(loop, 0);
    
    return 0;
}
```

**学习要点**：
- 信号处理的异步特性
- pipe 在信号传递中的作用
- 多个信号的统一处理
- 信号处理中的资源安全问题

---

### 第二周：核心机制

#### Day 8-9：事件循环核心

**任务目标**：深入理解 ev_run 的执行流程

| 时间 | 任务 | 参考资源 |
|------|----------|
| 上午 | 阅读 `ev_run` 函数实现 | `ev.c: 3700-3871` |
| 上午 | 理解事件循环的各个阶段 | `ev.c: 3700-3750` |
| 下午 | 分析 prepare/check watcher 的作用 | `ev.c: 3848-3853` |
| 下午 | 理解 pending 队列的调度机制 | `ev.c: 2021-2040` |
| 晚上 | 绘制事件循环流程图 | |

**学习要点**：
1. **事件循环的五个阶段**：
   - prepare 阶段（准备）
   - IO 等待阶段
   - 定时器处理阶段
   - check 阶段（检查）
   - 回调执行阶段

2. **pending 队列的作用**：
   - 收集待处理的事件
   - 按优先级排序
   - 批量执行回调

3. **优先级调度机制**：
   - EV_MINPRI 到 EV_MAXPRI
   - 高优先级优先执行
   - 优先级调整的时机

---

#### Day 10-11：IO 多路复用后端

**任务目标**：深入理解 epoll 后端实现

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev_epoll.c` 完整实现 | `libev-master/libev-master/ev_epoll.c` |
| 上午 | 理解 epoll 的 LT 和 ET 模式 | `ev_epoll.c: 100-200` |
| 下午 | 分析 epoll_ctl 的调用时机 | `ev_epoll.c: 200-300` |
| 下午 | 理解 reify 机制 | `ev.c: 1774` |
| 晚上 | 对比 select 和 epoll 的差异 | |

**学习要点**：
1. **epoll 后端的核心函数**：
   - `epoll_init`：初始化 epoll 实例
   - `epoll_modify`：修改事件注册
   - `epoll_poll`：等待 IO 事件

2. **reify 机制的作用**：
   - 延迟更新内核事件表
   - 避免在事件处理过程中修改共享状态
   - 提高并发安全性

3. **ET 模式的优势**：
   - 减少事件触发次数
   - 提高高并发场景下的性能
   - 需要配合非阻塞 IO 使用

---

#### Day 12-13：定时器实现原理

**任务目标**：深入理解定时器堆的实现

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中的堆操作函数 | `ev.c: 2100-2200` |
| 上午 | 理解 upheap 和 downheap 操作 | `ev.c: 2100-2150` |
| 下午 | 分析时间缓存机制 | `ev.c: 1830-1832` |
| 下午 | 理解时间精度处理 | `ev.c: 482-486` |
| 晚上 | 编写自定义定时器堆实现 | |

**学习要点**：
1. **最小堆的性质**：
   - 父节点的键值小于等于子节点
   - 完全二叉树结构
   - 高效的插入和删除操作

2. **时间缓存的优化**：
   - 减少 clock_gettime 调用次数
   - 在每次循环迭代开始时更新
   - 适合对时间精度要求不是极高的场景

3. **时间精度保护**：
   - MIN_INTERVAL 常量的作用
   - 防止浮点数精度问题导致的定时器风暴
   - 最大阻塞时间的限制

---

#### Day 14：并发与线程安全

**任务目标**：理解 libev 的线程安全机制

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中的线程相关代码 | `ev.c: 1555-1563` |
| 上午 | 理解 ev_async 的实现 | `ev.c: 4500-4600` |
| 下午 | 编写示例：跨线程事件传递 | 参考下方示例 8 |
| 下午 | 理解 fork 安全机制 | `ev.c: 3171-3207` |
| 晚上 | 总结本周学习内容，整理笔记 | |

**示例 8：跨线程事件传递**

```c
#include <ev.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

struct ev_async async_watcher;
struct ev_loop *loop;

static void async_cb(EV_P_ ev_async *w, int revents) {
    printf("主线程收到异步通知\n");
}

static void *worker_thread(void *arg) {
    sleep(2);
    printf("工作线程发送异步通知\n");
    ev_async_send(loop, &async_watcher);
    return NULL;
}

int main() {
    pthread_t tid;
    
    loop = ev_default_loop(0);
    ev_async_init(&async_watcher, async_cb);
    ev_async_start(loop, &async_watcher);
    
    pthread_create(&tid, NULL, worker_thread, NULL);
    
    printf("主线程运行中\n");
    ev_run(loop, 0);
    
    pthread_join(tid, NULL);
    return 0;
}
```

**学习要点**：
1. **ev_async 的作用**：
   - 跨线程发送事件通知
   - 线程安全的事件传递
   - 避免锁竞争

2. **fork 安全的处理**：
   - 子进程继承文件描述符
   - 重建内核事件表
   - 重新初始化信号 pipe

3. **原子操作的应用**：
   - 初始化标志的原子更新
   - 无锁同步机制
   - 性能优化考虑

---

### 第三周：高级特性

#### Day 15-16：子进程监控

**任务目标**：掌握 ev_child 的使用

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中子进程监控相关代码 | `ev.c: 4200-4300` |
| 上午 | 理解 waitpid 的调用时机 | `ev.c: 4250-4280` |
| 下午 | 编写示例：进程管理器 | 参考下方示例 9 |
| 下午 | 编写示例：守护进程实现 | 参考下方示例 10 |
| 晚上 | 分析子进程退出状态处理 | |

**示例 9：进程管理器**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

struct ev_child child_watcher;

static void child_cb(EV_P_ ev_child *w, int revents) {
    printf("子进程 %d 退出，状态: %d\n", w->rpid, WEXITSTATUS(w->rstatus));
    ev_child_stop(EV_A_ w);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    pid_t pid;
    
    pid = fork();
    if (pid == 0) {
        // 子进程
        printf("子进程启动，PID: %d\n", getpid());
        sleep(3);
        printf("子进程退出\n");
        exit(42);
    }
    
    ev_child_init(&child_watcher, child_cb, pid, 0);
    ev_child_start(loop, &child_watcher);
    
    printf("监控子进程 %d\n", pid);
    ev_run(loop, 0);
    return 0;
}
```

**示例 10：守护进程**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>

static void daemonize() {
    pid_t pid;
    
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }
    if (pid != 0) exit(0);
    
    setsid();
    
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }
    if (pid != 0) exit(0);
    
    chdir("/");
    umask(0);
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    openlog("my_daemon", LOG_PID, LOG_DAEMON);
}

static void timer_cb(EV_P_ ev_timer *w, int revents) {
    syslog(LOG_INFO, "守护进程运行中...");
}

int main() {
    daemonize();
    
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer;
    
    ev_timer_init(&timer, timer_cb, 1.0, 5.0);
    ev_timer_start(loop, &timer);
    
    syslog(LOG_INFO, "守护进程启动成功");
    ev_run(loop, 0);
    
    closelog();
    return 0;
}
```

**学习要点**：
1. **ev_child 的使用场景**：
   - 监控子进程退出
   - 自动重启崩溃的子进程
   - 收集子进程的退出状态

2. **守护进程的创建步骤**：
   - 第一次 fork 脱离终端
   - setsid 创建新会话
   - 第二次 fork 确保不是会话首进程
   - 重定向标准输入输出
   - 切换工作目录

3. **waitpid 的选项**：
   - WNOHANG 非阻塞等待
   - WUNTRACED 监控停止的进程
   - WCONTINUED 监控继续的进程

---

#### Day 17-18：文件系统监控

**任务目标**：掌握 ev_stat 和 inotify 的使用

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中文件系统监控相关代码 | `ev.c: 4500-4600` |
| 上午 | 理解 inotify 的集成 | `ev.c: 4550-4580` |
| 下午 | 编写示例：配置文件热更新 | 参考下方示例 11 |
| 下午 | 编写示例：目录变更监控 | 参考下方示例 12 |
| 晚上 | 分析轮询和 inotify 的对比 | |

**示例 11：配置文件热更新**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ev_stat stat_watcher;
char *config_file = "config.txt";

static void load_config() {
    FILE *fp = fopen(config_file, "r");
    if (!fp) return;
    
    char buf[1024];
    printf("重新加载配置:\n");
    while (fgets(buf, sizeof(buf), fp)) {
        printf("  %s", buf);
    }
    fclose(fp);
}

static void stat_cb(EV_P_ ev_stat *w, int revents) {
    printf("配置文件 %s 发生变化\n", config_file);
    load_config();
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    ev_stat_init(&stat_watcher, stat_cb, config_file, 0.);
    ev_stat_start(loop, &stat_watcher);
    
    printf("监控配置文件: %s\n", config_file);
    load_config();
    ev_run(loop, 0);
    return 0;
}
```

**示例 12：目录变更监控**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>

struct ev_io inotify_watcher;
int inotify_fd;

static void inotify_cb(EV_P_ ev_io *w, int revents) {
    char buf[4096];
    struct inotify_event *event;
    int n, i = 0;
    
    n = read(inotify_fd, buf, sizeof(buf));
    while (i < n) {
        event = (struct inotify_event*)&buf[i];
        printf("文件 %s 发生变化，mask: %x\n", 
               event->len ? event->name : "(未知)", event->mask);
        i += sizeof(struct inotify_event) + event->len;
    }
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    int wd;
    
    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
        return 1;
    }
    
    wd = inotify_add_watch(inotify_fd, ".", 
                           IN_CREATE | IN_DELETE | IN_MODIFY);
    if (wd < 0) {
        perror("inotify_add_watch");
        return 1;
    }
    
    ev_io_init(&inotify_watcher, inotify_cb, inotify_fd, EV_READ);
    ev_io_start(loop, &inotify_watcher);
    
    printf("监控当前目录（Ctrl+C 退出）\n");
    ev_run(loop, 0);
    
    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    return 0;
}
```

**学习要点**：
1. **ev_stat vs inotify**：
   - ev_stat：基于轮询，跨平台
   - inotify：基于事件，Linux 特有
   - 各有优缺点，根据场景选择

2. **inotify 的事件类型**：
   - IN_CREATE：文件创建
   - IN_DELETE：文件删除
   - IN_MODIFY：文件修改
   - IN_MOVE：文件移动

3. **配置文件热更新的实现**：
   - 监控文件变更
   - 安全地重新加载配置
   - 避免竞争条件

---

#### Day 19-20：异步事件

**任务目标**：掌握 ev_async 的使用

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中异步事件相关代码 | `ev.c: 4600-4700` |
| 上午 | 理解 ev_async_send 的实现 | `ev.c: 4650-4680` |
| 下午 | 编写示例：线程池实现 | 参考下方示例 13 |
| 下午 | 编写示例：事件驱动的计算任务 | 参考下方示例 14 |
| 晚上 | 分析异步事件的线程安全问题 | |

**示例 13：线程池**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <queue>
#include <mutex>
#include <condition_variable>

struct ev_async async_watcher;
struct ev_loop *loop;

std::queue<int> task_queue;
std::mutex queue_mutex;
std::condition_variable cv;
bool running = true;

struct TaskResult {
    int task_id;
    int result;
};

static void result_cb(EV_P_ ev_async *w, int revents) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    while (!task_queue.empty()) {
        int task_id = task_queue.front();
        task_queue.pop();
        printf("任务 %d 完成\n", task_id);
    }
}

static void *worker_thread(void *arg) {
    while (running) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock);
        
        // 模拟任务处理
        sleep(1);
        
        // 发送完成通知
        ev_async_send(loop, &async_watcher);
    }
    return NULL;
}

int main() {
    pthread_t tid;
    
    loop = ev_default_loop(0);
    ev_async_init(&async_watcher, result_cb);
    ev_async_start(loop, &async_watcher);
    
    pthread_create(&tid, NULL, worker_thread, NULL);
    
    // 提交任务
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push(1);
    }
    cv.notify_one();
    
    printf("线程池运行中（Ctrl+C 退出）\n");
    ev_run(loop, 0);
    
    running = false;
    cv.notify_all();
    pthread_join(tid, NULL);
    return 0;
}
```

**示例 14：事件驱动的计算任务**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct ComputeTask {
    ev_async async;
    int input;
    int result;
};

static void compute_async_cb(EV_P_ ev_async *w, int revents) {
    ComputeTask *task = (ComputeTask*)w;
    printf("计算结果: %d * 2 = %d\n", task->input, task->result);
    free(task);
}

static void *compute_thread(void *arg) {
    ComputeTask *task = (ComputeTask*)arg;
    
    // 模拟耗时计算
    sleep(2);
    task->result = task->input * 2;
    
    // 通知主线程
    ev_async_send(EV_DEFAULT, &task->async);
    return NULL;
}

static void submit_compute_task(int input) {
    ComputeTask *task = malloc(sizeof(ComputeTask));
    task->input = input;
    ev_async_init(&task->async, compute_async_cb);
    ev_async_start(EV_DEFAULT, &task->async);
    
    pthread_t tid;
    pthread_create(&tid, NULL, compute_thread, task);
    pthread_detach(tid);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    submit_compute_task(42);
    submit_compute_task(100);
    
    printf("计算任务提交（等待结果）\n");
    ev_run(loop, 0);
    return 0;
}
```

**学习要点**：
1. **ev_async 的特点**：
   - 跨线程安全
   - 非阻塞通知
   - 自动合并重复通知

2. **线程池的设计模式**：
   - 任务队列
   - 工作线程
   - 主线程和工作线程的通信

3. **异步计算的优势**：
   - 不阻塞事件循环
   - 充分利用多核 CPU
   - 提高响应性

---

#### Day 21：ev_once 一次性事件与 ev_embed 嵌入式事件循环

**任务目标**：掌握 ev_once 的使用，理解 ev_embed 嵌入事件循环的机制

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中 ev_once 的实现 | `ev.c: 4120-4160` |
| 上午 | 理解 ev_once 的回调机制和超时处理 | `ev.h: ev_once 相关声明` |
| 下午 | 阅读 `ev.c` 中 ev_embed 的实现 | `ev.c: 4600-4750` |
| 下午 | 理解嵌入式事件循环的适用场景 | `ev.pod: ev_embed 部分` |
| 晚上 | 编写示例，总结学习内容 | |

**示例 16：ev_once 一次性 IO 事件**

```c
#include <ev.h>
#include <stdio.h>
#include <unistd.h>

static void once_cb(struct ev_loop *loop, ev_io *w, int revents) {
    char buf[1024];
    if (revents & EV_READ) {
        int n = read(w->fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("ev_once 读取到: %s", buf);
        } else if (n == 0) {
            printf("EOF\n");
        }
    }
    printf("ev_once 回调执行完毕，watcher 自动停止\n");
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);

    printf("等待标准输入（输入一行后自动停止监控）...\n");
    ev_once(loop, STDIN_FILENO, EV_READ, 10.0, once_cb, NULL);

    ev_run(loop, 0);
    printf("事件循环退出\n");
    return 0;
}
```

**示例 17：ev_embed 嵌入另一个事件循环**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct ev_loop *outer_loop;
struct ev_loop *inner_loop;
struct ev_embed embed_watcher;
struct ev_timer inner_timer;
struct ev_timer outer_timer;

static void inner_timer_cb(EV_P_ ev_timer *w, int revents) {
    static int count = 0;
    printf("内部循环定时器触发 (count=%d)\n", ++count);
    if (count >= 5) {
        printf("内部循环任务完成，停止内部循环\n");
        ev_break(EV_A_ EVBREAK_ONE);
    }
}

static void outer_timer_cb(EV_P_ ev_timer *w, int revents) {
    static int count = 0;
    printf("外部循环定时器触发 (count=%d)\n", ++count);
    if (count >= 10) {
        printf("外部循环任务完成，退出\n");
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

int main() {
    outer_loop = ev_default_loop(0);
    inner_loop = ev_loop_new(EVFLAG_AUTO);

    if (!inner_loop) {
        fprintf(stderr, "无法创建内部事件循环\n");
        return 1;
    }

    ev_timer_init(&inner_timer, inner_timer_cb, 0.5, 1.0);
    ev_timer_start(inner_loop, &inner_timer);

    ev_embed_init(&embed_watcher, NULL, inner_loop);
    ev_embed_start(outer_loop, &embed_watcher);

    ev_timer_init(&outer_timer, outer_timer_cb, 1.0, 1.0);
    ev_timer_start(outer_loop, &outer_timer);

    printf("外部循环中嵌入了内部循环\n");
    printf("两个循环的定时器将同时运行\n");

    ev_run(outer_loop, 0);

    ev_embed_stop(outer_loop, &embed_watcher);
    ev_loop_destroy(inner_loop);

    return 0;
}
```

**示例 18：ev_embed 实现多后端共存**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    unsigned int backends = ev_supported_backends();

    printf("系统支持的后端: 0x%08x\n", backends);
    printf("  EVBACKEND_SELECT: %s\n", (backends & EVBACKEND_SELECT) ? "是" : "否");
    printf("  EVBACKEND_POLL:   %s\n", (backends & EVBACKEND_POLL)   ? "是" : "否");
    printf("  EVBACKEND_EPOLL:  %s\n", (backends & EVBACKEND_EPOLL)  ? "是" : "否");
    printf("  EVBACKEND_KQUEUE: %s\n", (backends & EVBACKEND_KQUEUE) ? "是" : "否");

    struct ev_loop *epoll_loop = NULL;
    struct ev_loop *main_loop = ev_default_loop(0);

    if (backends & EVBACKEND_EPOLL) {
        epoll_loop = ev_loop_new(EVBACKEND_EPOLL);
        if (epoll_loop) {
            printf("成功创建 epoll 后端循环\n");
        }
    }

    if (epoll_loop) {
        struct ev_embed embed;
        ev_embed_init(&embed, NULL, epoll_loop);
        ev_embed_start(main_loop, &embed);
        printf("已将 epoll 循环嵌入主循环\n");

        ev_run(main_loop, 0);

        ev_embed_stop(main_loop, &embed);
        ev_loop_destroy(epoll_loop);
    } else {
        printf("无法创建嵌入循环，使用主循环\n");
        ev_run(main_loop, 0);
    }

    return 0;
}
```

**学习要点**：

1. **ev_once 的特点**：
   - 一次性事件回调，执行后自动停止并清理 watcher
   - 支持 IO 事件和超时两种触发方式
   - 无需手动 init/start/stop，适合简单的单次操作
   - 内部使用 ev_io + ev_timer 实现，回调后自动释放资源

2. **ev_once 的典型场景**：
   - 等待一次性 IO 就绪（如读取配置文件）
   - 连接超时控制
   - 简单的延迟执行
   - 避免为一次性操作创建持久 watcher

3. **ev_once 的函数签名**：
   ```c
   void ev_once(struct ev_loop *loop, int fd, int events,
                ev_tstamp timeout, void (*cb)(...), void *arg);
   ```
   - `fd`：文件描述符，设为 -1 则仅使用超时
   - `events`：监听的事件类型（EV_READ/EV_WRITE）
   - `timeout`：超时时间，设为 0 则仅监听 IO
   - `cb`：回调函数，revents 可能为 EV_READ/EV_WRITE/EV_TIMER/EV_ERROR

4. **ev_embed 的作用**：
   - 将一个事件循环嵌入到另一个事件循环中
   - 使被嵌入循环的事件在宿主循环中处理
   - 实现不同后端的共存（如 epoll 循环嵌入 kqueue 循环）
   - 用于将第三方库的事件循环集成到自己的主循环中

5. **ev_embed 的典型场景**：
   - 集成使用独立事件循环的第三方库
   - 在同一程序中使用不同的 IO 多路复用后端
   - 将子循环的事件统一到主循环中管理
   - 实现分层的事件处理架构

6. **ev_embed 的注意事项**：
   - 被嵌入的循环不能独立运行 `ev_run`，由宿主循环驱动
   - 被嵌入循环必须是可嵌入的（支持 `ev_embed` 后端）
   - 宿主循环停止时，被嵌入循环也会停止
   - 需要手动管理被嵌入循环的生命周期（创建和销毁）

---

#### Day 22：fork 安全与状态恢复

**任务目标**：深入理解 fork 后的处理机制

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| 上午 | 阅读 `ev.c` 中 fork 相关代码 | `ev.c: 3171-3207` |
| 上午 | 理解各后端的 fork 处理 | `ev_epoll.c: 300-320` |
| 下午 | 编写示例：fork 后的事件循环恢复 | 参考下方示例 15 |
| 下午 | 分析 fork 后的资源继承问题 | |
| 晚上 | 总结本周学习内容 | |

**示例 15：fork 后的事件循环恢复**

```c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

struct ev_timer timer;
struct ev_loop *loop;

static void timer_cb(EV_P_ ev_timer *w, int revents) {
    printf("定时器触发 - PID: %d\n", getpid());
}

int main() {
    loop = ev_default_loop(0);
    
    ev_timer_init(&timer, timer_cb, 1.0, 1.0);
    ev_timer_start(loop, &timer);
    
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程需要重新初始化事件循环
        printf("子进程 PID: %d\n", getpid());
        ev_loop_fork(loop);  // 关键：fork 后的状态恢复
        ev_run(loop, 0);
        exit(0);
    }
    
    printf("父进程 PID: %d，子进程 PID: %d\n", getpid(), pid);
    ev_run(loop, 0);
    return 0;
}
```

**学习要点**：
1. **fork 后的问题**：
   - 文件描述符的继承
   - 内核事件表的状态
   - 信号处理的状态

2. **ev_loop_fork 的作用**：
   - 重建 epoll/kqueue 实例
   - 重新初始化信号 pipe
   - 恢复事件循环的正常运行

3. **fork 后的最佳实践**：
   - 在子进程中尽快调用 ev_loop_fork
   - 避免在 fork 前后共享状态
   - 注意资源的正确释放

---

### 第四周：项目实践

#### Day 23-25：实现 Echo 服务器

**任务目标**：实现一个基于 libev 的高性能 Echo 服务器

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| Day 23 | 设计服务器架构，实现基础的 TCP 监听 | 参考官方示例 `echo_server.c` |
| Day 24 | 实现客户端连接管理和数据回显 | 参考 `ev_io` 的使用 |
| Day 25 | 添加信号处理和优雅退出 | 参考 `ev_signal` 的使用 |

**功能需求**：
1. 监听指定端口
2. 接受客户端连接
3. 回显客户端发送的数据
4. 支持多个客户端同时连接
5. 优雅处理客户端断开
6. 支持通过信号优雅退出

**代码结构**：
```
echo_server/
├── main.c          # 主函数和初始化
├── server.c        # 服务器核心逻辑
├── server.h        # 头文件定义
├── client.c        # 客户端连接管理
├── client.h        # 客户端相关定义
├── Makefile        # 编译脚本
└── README.md       # 说明文档
```

**参考实现**：

```c
// server.c
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

struct ev_io accept_watcher;
struct ev_loop *loop;
int listen_fd;

static void client_cb(EV_P_ ev_io *w, int revents);

static void accept_cb(EV_P_ ev_io *w, int revents) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
    
    if (client_fd < 0) {
        perror("accept");
        return;
    }
    
    printf("新客户端连接: %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    struct ev_io *client_watcher = malloc(sizeof(struct ev_io));
    ev_io_init(client_watcher, client_cb, client_fd, EV_READ);
    ev_io_start(loop, client_watcher);
}

static void client_cb(EV_P_ ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf));
    
    if (n <= 0) {
        printf("客户端断开连接\n");
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }
    
    printf("收到数据 (%d 字节)\n", n);
    write(w->fd, buf, n);  // Echo 回显
}

int server_init(int port) {
    struct sockaddr_in addr;
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return -1;
    }
    
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return -1;
    }
    
    if (listen(listen_fd, 128) < 0) {
        perror("listen");
        close(listen_fd);
        return -1;
    }
    
    loop = ev_default_loop(0);
    ev_io_init(&accept_watcher, accept_cb, listen_fd, EV_READ);
    ev_io_start(loop, &accept_watcher);
    
    return 0;
}

int server_run() {
    printf("Echo 服务器启动，监听端口 %d\n", 8080);
    ev_run(loop, 0);
    return 0;
}

void server_cleanup() {
    ev_io_stop(loop, &accept_watcher);
    close(listen_fd);
    printf("服务器关闭\n");
}
```

---

#### Day 26-27：实现 HTTP 服务器

**任务目标**：在 Echo 服务器基础上实现一个简单的 HTTP 服务器

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| Day 26 | 实现 HTTP 请求解析 | 学习 HTTP 协议基础 |
| Day 27 | 实现静态文件服务和动态响应 | 参考 HTTP 状态码 |

**功能需求**：
1. 支持 GET 请求
2. 支持静态文件服务
3. 返回正确的 HTTP 响应头
4. 支持目录列表
5. 处理常见的 HTTP 状态码

**参考实现**：

```c
// http_server.c
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 4096
#define MAX_PATH 1024

static void http_response(EV_P_ ev_io *w, int status, const char *content_type, 
                          const char *body, int body_len) {
    char response[BUF_SIZE];
    int len = snprintf(response, BUF_SIZE,
                       "HTTP/1.1 %d %s\r\n"
                       "Content-Type: %s\r\n"
                       "Content-Length: %d\r\n"
                       "Connection: close\r\n"
                       "\r\n",
                       status, 
                       status == 200 ? "OK" : "Not Found",
                       content_type,
                       body_len);
    
    write(w->fd, response, len);
    write(w->fd, body, body_len);
}

static void http_client_cb(EV_P_ ev_io *w, int revents) {
    char buf[BUF_SIZE];
    char method[16], path[MAX_PATH], version[16];
    int n = read(w->fd, buf, sizeof(buf)-1);
    
    if (n <= 0) {
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }
    
    buf[n] = '\0';
    sscanf(buf, "%s %s %s", method, path, version);
    
    printf("收到请求: %s %s %s\n", method, path, version);
    
    if (strcmp(method, "GET") != 0) {
        http_response(EV_A_ w, 405, "text/plain", "Method Not Allowed", 17);
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }
    
    char file_path[MAX_PATH];
    snprintf(file_path, MAX_PATH, ".%s", path);
    
    struct stat st;
    if (stat(file_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        http_response(EV_A_ w, 404, "text/plain", "Not Found", 9);
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }
    
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        http_response(EV_A_ w, 500, "text/plain", "Internal Server Error", 21);
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }
    
    char *content = malloc(st.st_size);
    read(fd, content, st.st_size);
    close(fd);
    
    http_response(EV_A_ w, 200, "text/html", content, st.st_size);
    free(content);
    
    ev_io_stop(loop, w);
    close(w->fd);
    free(w);
}
```

---

#### Day 28-29：性能优化与测试

**任务目标**：对服务器进行性能优化和压力测试

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| Day 28 | 分析性能瓶颈，进行代码优化 | 使用 perf 工具 |
| Day 29 | 编写测试用例，进行压力测试 | 使用 ab 或 wrk 工具 |

**优化方向**：
1. **内存分配优化**：使用内存池减少 malloc/free 调用
2. **缓冲区优化**：减少数据拷贝
3. **非阻塞 IO**：使用 ET 模式
4. **连接复用**：支持 HTTP Keep-Alive
5. **多核利用**：使用多个事件循环

**性能测试**：
```bash
# 使用 ab 进行压力测试
ab -n 10000 -c 100 http://localhost:8080/

# 使用 wrk 进行压力测试
wrk -t4 -c100 -d30s http://localhost:8080/

# 使用 perf 分析性能
perf record -g ./http_server
perf report
```

---

#### Day 30-31：项目总结与文档编写

**任务目标**：完成项目文档，总结学习成果

| 时间 | 任务 | 参考资源 |
|------|------|----------|
| Day 30 | 编写项目说明文档 | 参考 README 模板 |
| Day 31 | 总结学习心得，整理知识体系 | 回顾学习计划 |

**文档内容**：
1. **项目概述**：项目的目标和背景
2. **功能特性**：支持的功能列表
3. **编译运行**：编译命令和运行方式
4. **API 文档**：主要函数和数据结构
5. **性能测试**：测试结果和性能指标
6. **扩展建议**：未来的改进方向

**学习总结**：
1. **掌握的知识**：
   - 事件驱动编程模型
   - IO 多路复用机制
   - libev 核心 API
   - 定时器、信号、子进程等高级特性
   - ev_once 一次性事件和 ev_embed 嵌入式事件循环

2. **遇到的挑战**：
   - 理解事件循环的执行流程
   - 处理并发和线程安全问题
   - 性能优化的实践

3. **未来的学习方向**：
   - 深入学习其他事件库（libuv, event）
   - 学习协程和异步编程
   - 研究高性能网络编程的其他技术

---

## 附录

### 常用编译命令

```bash
# 编译单个源文件
gcc -o program program.c -lev

# 带调试信息编译
gcc -g -o program program.c -lev

# 带优化编译
gcc -O2 -o program program.c -lev

# 编译多个源文件
gcc -o server server.c client.c -lev

# 使用 pkg-config
gcc -o program program.c $(pkg-config --cflags --libs libev)
```

### 调试技巧

1. **启用 libev 调试模式**：
```c
#define EV_VERIFY 3
#include <ev.h>
```

2. **检查后端支持**：
```c
printf("支持的后端: %08x\n", ev_supported_backends());
printf("推荐的后端: %08x\n", ev_recommended_backends());
```

3. **使用 GDB 调试**：
```bash
gdb ./program
(gdb) break ev_run
(gdb) run
(gdb) next
(gdb) print loop->backend
```

### 参考资料

1. **官方文档**: http://pod.tst.eu/http://cvs.schmorp.de/libev/ev.pod
2. **源码仓库**: https://github.com/enki/libev
3. **UNIX 网络编程**: 卷 1，W. Richard Stevens
4. **Linux 高性能服务器编程**: 游双
5. **libev 教程**: https://github.com/nicow/libev-tutorial

---

**学习计划版本**: 1.0  
**创建日期**: 2024年  
**适用人群**: C 语言开发者，希望学习事件驱动编程和高性能网络编程

---

祝你学习愉快！🚀
