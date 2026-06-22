# libev Fork（子进程）安全详解

## 一、为什么 fork 后需要特殊处理？

### 1.1 fork 的语义

`fork()` 创建子进程后，子进程**继承父进程的内存映像**，包括：

- 所有 watcher（ev_io、ev_timer、ev_signal 等）
- 事件循环的状态（pending 队列、定时器堆等）
- **文件描述符**（epoll fd、kqueue fd、pipe fd 等）

### 1.2 核心问题：内核状态不可继承

```
父进程                              子进程（fork 后）
  │                                   │
  epoll_fd ──► 内核 epoll 实例        epoll_fd（同一个数字）──► ？
  kqueue_fd ──► 内核 kqueue 实例      kqueue_fd（同一个数字）──► ？
  pipe[0], pipe[1]                    pipe[0], pipe[1]（共享？）
```

**问题**：

| 资源 | fork 后的状态 | 问题 |
|------|-------------|------|
| **epoll fd** | 子进程继承 fd 编号，但**不继承内核中的 epoll 实例** | epoll_wait 返回空事件或错误 |
| **kqueue fd** | 某些 BSD 内核甚至**直接关闭** kqueue fd | fd 变成无效 |
| **pipe fd** | 父子进程**共享**同一个 pipe | 信号通知混乱 |
| **信号处理** | 子进程继承信号处理函数 | 但 `signalfd` 的 fd 可能无效 |

### 1.3 类比

```
fork 就像复制了一栋房子的钥匙（fd 编号）
但房子本身（内核数据结构）并没有被复制
→ 子进程拿着钥匙去开门，发现房子已经不在了
```

---

## 二、libev 的 fork 处理机制

### 2.1 两种检测 fork 的方式

#### 方式 1：手动调用 `ev_loop_fork()`

```c
pid_t pid = fork();
if (pid == 0) {
    // 子进程
    ev_loop_fork(EV_DEFAULT);  // 必须！
    ev_run(EV_DEFAULT, 0);
}
```

#### 方式 2：自动检测 `EVFLAG_FORKCHECK`

```c
struct ev_loop *loop = ev_default_loop(EVFLAG_FORKCHECK);
// libev 会在每次循环迭代中调用 getpid() 检测是否 fork
```

### 2.2 `ev_loop_fork` 的实现

```c
void ev_loop_fork(EV_P) {
    postfork = 1;  // 只是设置标志位！
}
```

**注意**：`ev_loop_fork()` 只是设置 `postfork = 1`，真正的处理在 `ev_run()` 的循环迭代中。

### 2.3 `postfork` 标志的处理流程

```
ev_run() 主循环
    │
    ├── 检测 fork（两种方式）
    │   ├── EVFLAG_FORKCHECK: getpid() != curpid → postfork = 1
    │   └── 手动调用: ev_loop_fork() → postfork = 1
    │
    ├── postfork == 1 ?
    │   ├── 触发 ev_fork watcher 回调
    │   │   queue_events(forks, forkcnt, EV_FORK)
    │   │   EV_INVOKE_PENDING
    │   │
    │   └── 执行 loop_fork() 重建内核状态
    │       ├── backend_fork()  → 重建 epoll/kqueue fd
    │       ├── pipe 重建       → 关闭旧 pipe，创建新 pipe
    │       └── postfork = 0    → 标记处理完成
    │
    └── fd_reify()  → 重新注册所有 fd 的监听事件
```

---

## 三、`loop_fork` 详解 — 重建内核状态

### 3.1 完整代码

```c
inline_size void loop_fork(EV_P) {
    // 1. 重建 backend（epoll/kqueue fd）
#if EV_USE_EPOLL
    if (backend == EVBACKEND_EPOLL) epoll_fork(EV_A);
#endif
#if EV_USE_KQUEUE
    if (backend == EVBACKEND_KQUEUE) kqueue_fork(EV_A);
#endif
#if EV_USE_PORT
    if (backend == EVBACKEND_PORT) port_fork(EV_A);
#endif

    // 2. 重建 inotify
#if EV_USE_INOTIFY
    infy_fork(EV_A);
#endif

    // 3. 重建 pipe
#if EV_SIGNAL_ENABLE || EV_ASYNC_ENABLE
    if (ev_is_active(&pipe_w) && postfork != 2) {
        ev_ref(EV_A);
        ev_io_stop(EV_A_, &pipe_w);

        if (evpipe[0] >= 0)
            EV_WIN32_CLOSE_FD(evpipe[0]);

        evpipe_init(EV_A);              // 创建新 pipe
        ev_feed_event(EV_A_, &pipe_w, EV_CUSTOM);  // 确保处理残留事件
    }
#endif

    postfork = 0;  // 标记处理完成
}
```

### 3.2 epoll_fork — 重建 epoll 实例

```c
void epoll_fork(EV_P) {
    close(backend_fd);                    // 关闭父进程的 epoll fd

    while ((backend_fd = epoll_create(256)) < 0)  // 创建新的 epoll 实例
        ev_syserr("(libev) epoll_create");

    fcntl(backend_fd, F_SETFD, FD_CLOEXEC);  // 设置 close-on-exec

    fd_rearm_all(EV_A);                  // 重新注册所有 fd
}
```

**为什么必须重建？**

```
父进程的 epoll fd 指向内核中的 epoll 实例 A
fork 后，子进程的 epoll fd 编号相同
但内核中的 epoll 实例 A 仍属于父进程
子进程的 epoll_wait() 会返回空事件

解决方案：关闭旧 fd，创建新的 epoll 实例 B
然后通过 fd_rearm_all() 重新注册所有监听
```

### 3.3 kqueue_fork — 重建 kqueue 实例

```c
void kqueue_fork(EV_P) {
    pid_t newpid = getpid();

    // BSD 内核的 bug：fork 后可能直接关闭 kqueue fd
    // 通过比较 pid 判断 fd 是否还有效
    if (newpid == kqueue_fd_pid)
        close(backend_fd);

    kqueue_fd_pid = newpid;
    while ((backend_fd = kqueue()) < 0)
        ev_syserr("(libev) kqueue");

    fcntl(backend_fd, F_SETFD, FD_CLOEXEC);

    fd_rearm_all(EV_A);
}
```

**BSD 内核的坑**：

```
某些 BSD 内核在 fork 后会直接关闭 kqueue fd
但这个行为没有文档记录！

libev 的解决方案：
  - 记录创建 kqueue 时的 pid（kqueue_fd_pid）
  - fork 后比较 pid
  - 如果 pid 相同 → fd 还有效，关闭它
  - 如果 pid 不同 → fd 已被内核关闭，不需要再 close
  - 然后创建新的 kqueue 实例
```

### 3.4 pipe 重建

```
父进程                              子进程
  pipe[0] ◄──── pipe ────► pipe[1]    pipe[0] ◄──── 同一个 pipe ────► pipe[1]
  │                                         │
  ev_io 监听 pipe[0]                        ev_io 监听 pipe[0]
  │                                         │
  如果父子进程共享同一个 pipe：               │
  - 父进程读走数据 → 子进程读不到              │
  - 子进程写数据 → 父进程也被唤醒              │
  - 信号通知混乱                              │

解决方案：
  关闭旧 pipe → 创建新 pipe → 重新注册 pipe_w
```

---

## 四、`EVFLAG_FORKCHECK` — 自动检测 fork

### 4.1 工作原理

```c
// ev_run 主循环中
if (expect_false(curpid))
    if (expect_false(getpid() != curpid)) {  // pid 变了 → fork 了
        curpid = getpid();
        postfork = 1;
    }
```

### 4.2 性能影响

| 方面 | 影响 |
|------|------|
| **每次迭代** | 调用一次 `getpid()` |
| **Linux** | `getpid()` 是 5 条指令，无系统调用，极快 |
| **其他系统** | 可能需要系统调用，稍慢 |
| **适用场景** | 不确定是否 fork、使用第三方库可能 fork |

### 4.3 对比两种方式

| 方式 | 优点 | 缺点 |
|------|------|------|
| `ev_loop_fork()` 手动调用 | 零开销 | 需要记得在 fork 后调用 |
| `EVFLAG_FORKCHECK` 自动检测 | 无需手动处理 | 每次迭代多一次 `getpid()` |

### 4.4 推荐：`pthread_atfork` + `ev_loop_fork`

```c
static void post_fork_child(void) {
    ev_loop_fork(EV_DEFAULT);
}

// 注册 fork 处理器
pthread_atfork(0, 0, post_fork_child);
```

**优点**：零运行时开销 + 自动调用。

---

## 五、ev_fork Watcher — fork 事件通知

### 5.1 用途

在 fork 发生后，子进程中执行自定义的清理/重建逻辑。

### 5.2 使用示例

```c
struct ev_fork fork_w;

void fork_cb(EV_P_ ev_fork *w, int revents) {
    printf("fork 发生！子进程 pid = %d\n", getpid());
    // 在这里做子进程特有的清理工作
    // 比如：关闭继承的数据库连接、重新初始化线程等
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    ev_fork_init(&fork_w, fork_cb);
    ev_fork_start(loop, &fork_w);

    pid_t pid = fork();
    if (pid == 0) {
        ev_loop_fork(EV_A);  // 触发 fork_cb
        ev_run(loop, 0);
    } else {
        ev_run(loop, 0);
    }
}
```

### 5.3 回调执行时机

```
ev_run() 检测到 postfork == 1
    │
    ├── 1. 触发 ev_fork 回调  ← 用户自定义处理
    │
    ├── 2. 执行 loop_fork()   ← libev 内部重建
    │
    └── 3. fd_reify()         ← 重新注册 fd
```

**关键**：ev_fork 回调在 loop_fork() **之前**执行，用户可以在回调中做额外清理。

### 5.4 ev_fork 只在子进程中触发

```
父进程                              子进程
  │                                   │
  fork()                              │
  │                                   │
  ev_run() 继续运行                    ev_run() 检测到 postfork
  （不触发 ev_fork 回调）               触发 ev_fork 回调 ✓
                                       执行 loop_fork()
```

---

## 六、不同场景的 fork 处理策略

### 6.1 场景 1：fork + exec（最常见）

```c
pid_t pid = fork();
if (pid == 0) {
    // 子进程：直接 exec，不需要事件循环
    execvp("/bin/ls", args);
}
// 父进程：继续使用事件循环，无需任何处理
```

**不需要** `ev_loop_fork()`，因为子进程没有使用事件循环。

### 6.2 场景 2：子进程继续使用事件循环

```c
pid_t pid = fork();
if (pid == 0) {
    // 子进程
    ev_loop_fork(EV_DEFAULT);  // 必须！
    ev_run(EV_DEFAULT, 0);
} else {
    // 父进程
    ev_run(EV_DEFAULT, 0);
}
```

### 6.3 场景 3：父子进程都想使用事件循环

```c
pid_t pid = fork();
if (pid == 0) {
    // 子进程：销毁旧 loop，创建新 loop
    ev_loop_destroy(EV_DEFAULT);
    struct ev_loop *loop = ev_default_loop(0);
    // 重新注册需要的 watcher
    ev_run(loop, 0);
} else {
    // 父进程：继续使用原来的 loop
    ev_run(EV_DEFAULT, 0);
}
```

**为什么子进程要销毁重建？**

```
fork 后子进程继承了父进程的所有 watcher
但子进程可能不需要所有 watcher
如果直接 ev_loop_fork + ev_run，所有 watcher 都会生效

销毁重建：
  - 更干净（没有不需要的 watcher）
  - 避免 copy-on-write（不触碰父进程的内存页）
  - 但需要手动重新注册需要的 watcher
```

### 6.4 场景 4：多线程程序中 fork

```c
// 注册 pthread_atfork 处理器
static void post_fork_child(void) {
    ev_loop_fork(EV_DEFAULT);
}

pthread_atfork(0, 0, post_fork_child);

// 任何地方 fork 都安全
pid_t pid = fork();
```

**多线程 fork 的特殊问题**：

```
多线程程序 fork 后，子进程只有调用 fork 的线程存活
其他线程的锁可能处于锁定状态 → 死锁风险

解决方案：
  1. fork 前获取所有锁
  2. fork 后在子进程中释放所有锁
  3. 或者使用 pthread_atfork 注册准备/清理函数
```

---

## 七、epoll 的 fork 陷阱

### 7.1 epoll 的已知 Bug

libev 文档中明确提到 epoll 的 fork 问题：

> Epoll is also notoriously buggy - embedding epoll fds should work, but of course doesn't.

| Bug | 说明 |
|-----|------|
| **fork 后 epoll 无效** | 子进程的 epoll fd 不指向有效的 epoll 实例 |
| **错误报告事件** | epoll 可能报告**完全不同 fd** 的事件（尤其是 SMP 系统） |
| **已关闭 fd 的事件** | epoll 可能报告已关闭 fd 的事件，无法从集合中移除 |

### 7.2 libev 的应对措施

```c
// epoll_init 中设置 FD_CLOEXEC
fcntl(backend_fd, F_SETFD, FD_CLOEXEC);

// epoll_fork 中完全重建
close(backend_fd);              // 关闭旧 fd
backend_fd = epoll_create(256); // 创建新实例
fd_rearm_all(EV_A);            // 重新注册所有 fd
```

### 7.3 代际计数器（generation counter）

libev 使用代际计数器过滤 epoll 的虚假事件：

```
每次 fd 重新注册时，递增代际计数器
收到事件时，比较事件中的代际与当前代际
如果不匹配 → 虚假事件，忽略
```

---

## 八、完整 fork 处理流程图

```
fork() 调用
    │
    ├── 父进程：继续运行，无需特殊处理
    │
    └── 子进程：
        │
        ├── 方式 1：手动调用 ev_loop_fork()
        │   ev_loop_fork(EV_DEFAULT) → postfork = 1
        │
        ├── 方式 2：EVFLAG_FORKCHECK 自动检测
        │   ev_run() 中 getpid() != curpid → postfork = 1
        │
        └── 方式 3：pthread_atfork 自动调用
            post_fork_child() → ev_loop_fork() → postfork = 1

子进程进入 ev_run()：
    │
    ├── 检测 postfork == 1
    │   │
    │   ├── 触发 ev_fork watcher 回调
    │   │   （用户自定义的清理逻辑）
    │   │
    │   └── 执行 loop_fork()
    │       ├── epoll_fork() / kqueue_fork()
    │       │   ├── close(旧 backend_fd)
    │       │   ├── 创建新 backend_fd
    │       │   └── fd_rearm_all() 重新注册所有 fd
    │       │
    │       ├── infy_fork() 重建 inotify
    │       │
    │       ├── 重建 pipe
    │       │   ├── ev_io_stop(&pipe_w)
    │       │   ├── close(旧 pipe[0])
    │       │   ├── evpipe_init() 创建新 pipe
    │       │   └── ev_feed_event(&pipe_w, EV_CUSTOM)
    │       │
    │       └── postfork = 0
    │
    └── fd_reify() 重新注册 fd 事件
```

---

## 九、最佳实践总结

### 9.1 通用规则

| 规则 | 说明 |
|------|------|
| **子进程必须调用 `ev_loop_fork()`** | 在继续使用事件循环之前 |
| **fork 后忽略 SIGPIPE** | pipe 在 fork 后可能异常 |
| **不要在 ev_prepare 回调中调用 `ev_loop_fork()`** | 会导致未定义行为 |
| **优先使用 `pthread_atfork`** | 零运行时开销，自动调用 |

### 9.2 场景速查

| 场景 | 处理方式 |
|------|---------|
| fork + exec | 不需要 `ev_loop_fork()` |
| 子进程使用事件循环 | `ev_loop_fork()` |
| 父子都用事件循环 | 子进程销毁重建 loop |
| 多线程程序 | `pthread_atfork` + `ev_loop_fork()` |
| 不确定是否 fork | `EVFLAG_FORKCHECK` |

### 9.3 常见错误

```c
// ❌ 错误：fork 后没有调用 ev_loop_fork
pid_t pid = fork();
if (pid == 0) {
    ev_run(loop, 0);  // epoll/kqueue fd 无效！
}

// ❌ 错误：在 ev_prepare 回调中调用 ev_loop_fork
void prepare_cb(EV_P_ ev_prepare *w, int revents) {
    ev_loop_fork(EV_A);  // 未定义行为！
}

// ✅ 正确：fork 后立即调用 ev_loop_fork
pid_t pid = fork();
if (pid == 0) {
    ev_loop_fork(EV_DEFAULT);
    ev_run(EV_DEFAULT, 0);
}
```

---

## 十、API 速查

| API | 说明 | 线程安全 |
|-----|------|---------|
| `ev_loop_fork(loop)` | 设置 postfork 标志，下次 ev_run 时重建 | ✅ |
| `EVFLAG_FORKCHECK` | 创建 loop 时的标志，自动检测 fork | — |
| `ev_fork_init(w, cb)` | 初始化 fork watcher | — |
| `ev_fork_start(loop, w)` | 注册 fork watcher | — |
| `ev_fork_stop(loop, w)` | 注销 fork watcher | — |

---

## 参考资料

- `ev.c:3165-3225` — `loop_fork()` 实现
- `ev.c:3408-3411` — `ev_loop_fork()` 实现
- `ev.c:3724-3730` — `EVFLAG_FORKCHECK` 检测逻辑
- `ev.c:3733-3740` — ev_fork watcher 触发
- `ev_epoll.c:272-285` — `epoll_fork()` 实现
- `ev_kqueue.c:187-214` — `kqueue_fork()` 实现
- `ev.c:5008-5040` — `ev_fork_start/stop` 实现
- `ev.pod:3262-3316` — ev_fork 官方文档
- `ev.pod:685-722` — ev_loop_fork 官方文档
