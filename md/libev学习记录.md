# libev学习记录

## 概要

libev的核心思想就是事件循环，将异步转为同步。

核心结构体：事件循环ev\_loop。

有一系列watcher去封装不同的事件：

- ev\_io：I/O事件
- ev\_signal：signal
- ev\_timer：相对定时器
- ev\_periodic：绝对定时器
- ev\_child：SIGCHILD
- ev\_fork：fork
- ev\_idle：loop空转
- ev\_async：跨线程异步通知
- ev\_stat：文件、目录发生变化

<br />

还有一些非事件的watcher：

- ev\_prepare：每次loop最先执行的watcher
- ev\_check：每次loop最后执行的watcher

<br />

事件循环函数ev\_run：

```C
ev_run(loop, flags)
│
├── 1. EV_INVOKE_PENDING          ← 先执行上一轮遗留的 pending 回调
│
├── do {
│     │
│     ├── 2. fork 检测             ← 检查是否在子进程中（pid 变化）
│     │   └── 若是：设置 postfork 标志
│     │
│     ├── 3. EV_FORK 处理          ← 若 postfork，执行所有 fork watcher 回调
│     │
│     ├── 4. EV_PREPARE 处理       ← 执行所有 prepare watcher 回调
│     │
│     ├── 5. loop_done 检查        ← 若 ev_break 被调用，跳出循环
│     │
│     ├── 6. loop_fork()           ← 若 postfork，重建内核事件表（epoll/kqueue fd）
│     │
│     ├── 7. fd_reify()            ← 将 fd 变更同步到内核（epoll_ctl 等）
│     │
│     ├── 8. 计算阻塞时间 waittime
│     │   ├── 取定时器堆顶的最早超时时间
│     │   ├── 取周期定时器堆顶的最早超时时间
│     │   ├── 取三者最小值：waittime、MAX_BLOCKTIME、backend_mintime
│     │   └── 若有 io_blocktime，先 sleep 再 poll
│     │
│     ├── 9. backend_poll(waittime) ← 核心：调用 IO 多路复用 API 阻塞等待
│     │   ├── epoll_wait / kevent / select / poll
│     │   └── 就绪的 fd 事件被加入 pending 队列
│     │
│     ├── 10. pipe 事件处理         ← 若有 signal/async 事件，加入 pending
│     │
│     ├── 11. time_update()         ← 更新当前时间
│     │
│     ├── 12. timers_reify()        ← 处理超时的 ev_timer，回调加入 pending
│     │
│     ├── 13. periodics_reify()     ← 处理超时的 ev_periodic，回调加入 pending
│     │
│     ├── 14. idle_reify()          ← 若无其他 pending 事件，执行 idle watcher
│     │
│     ├── 15. EV_CHECK 处理         ← 执行所有 check watcher 回调
│     │
│     └── 16. EV_INVOKE_PENDING     ← 执行本轮所有 pending 回调
│
├── } while (activecnt && !loop_done && !(flags & ONCE|NOWAIT))
│
└── 返回 activecnt（剩余活跃 watcher 数）
```

<br />

## 代码结构

### 核心源代码文件

ev.c&#x20;

**类型**: C 源代码
**用途**: libev 的核心实现
**内容**:

- 事件循环的核心逻辑
- 所有 Watcher 类型的实现
- 事件调度和处理机制
- 平台抽象层
- 主要 API 函数的实现（约 5200 行代码）

---

ev.h&#x20;

**类型**: C 头文件
**用途**: libev 的公共 API 接口
**内容**:

- 所有 Watcher 结构体定义
- 公共 API 函数声明
- 宏定义和常量
- 事件类型标志（EV\_READ, EV\_WRITE 等）
- 后端类型定义

---

ev\_vars.h

**类型**: C 头文件
**用途**: 事件循环的变量定义
**内容**:

- `struct ev_loop` 的完整定义
- 事件循环的内部变量
- 后端特定的数据结构
- 文件描述符管理结构

**作用**: 定义事件循环内部使用的数据结构和变量

---

ev\_wrap.h

**类型**: C 头文件
**用途**: 编译时的包装宏
**内容**:

- 条件编译包装
- 不同编译器适配
- 跨平台兼容性宏

**作用**: 提供编译时的抽象层，处理不同编译器和平台的差异

---

ev++.h

**类型**: C++ 头文件
**用途**: libev 的 C++ 接口
**内容**:

- libev API 的 C++ 封装
- 类和对象风格的接口
- 支持 C++ 的回调（成员函数）
- 不增加额外的内存和运行时开销

---

event.c&#x20;

**类型**: C 源代码
**用途**: libevent 兼容层实现
**内容**:

- 将 libevent API 映射到 libev
- 提供向后兼容的接口
- 实现 libevent 的核心函数

**目的**: 让使用 libevent 的代码可以无缝切换到 libev

---

event.h

**类型**: C 头文件
**用途**: libevent 兼容层头文件
**内容**:

- libevent API 的函数声明
- 宏定义和数据结构
- 与 libevent 的兼容性接口

---

### 平台特定后端文件

ev\_epoll.c&#x20;

**类型**: C 源代码
**用途**: Linux epoll 后端实现
**平台**: Linux 系统
**内容**:

- `epoll_create`, `epoll_ctl`, `epoll_wait` 的封装
- epoll 特定的事件处理逻辑
- 高性能 I/O 多路复用实现

---

ev\_kqueue.c&#x20;

**类型**: C 源代码
**用途**: BSD/macOS kqueue 后端实现
**平台**: BSD, macOS, FreeBSD 等
**内容**:

- `kqueue`, `kevent` 系统调用的封装
- kqueue 特定的事件处理逻辑

---

ev\_select.c

**类型**: C 源代码
**用途**: POSIX select 后端实现
**平台**: 跨平台（POSIX 标准）
**内容**:

- `select()` 系统调用的封装
- 兼容性最强的后端，但性能较低

---

ev\_poll.c&#x20;

**类型**: C 源代码
**用途**: POSIX poll 后端实现
**平台**: Linux, Unix 等
**内容**:

- `poll()` 系统调用的封装
- 比 select 更高效，支持更多文件描述符

---

ev\_port.c&#x20;

**类型**: C 源代码
**用途**: Solaris Event Ports 后端实现
**平台**: Solaris 系统
**内容**:

- Solaris 特有的 event port 机制
- `port_create`, `port_associate` 等封装

---

ev\_win32.c&#x20;

**类型**: C 源代码
**用途**: Windows IOCP 后端实现
**平台**: Windows 系统
**内容**:

- Windows I/O Completion Port 封装
- Windows 特有的 I/O 多路复用机

<br />

## 代码风格和代码组织

### 宏编程：X-Macro 模式

libev 大量使用 **X-Macro 模式**，通过宏实现代码生成，避免重复定义：

**ev\_vars.h + ev\_wrap.h 的配合**：

```c
// ev_vars.h 中用 VARx 宏声明变量
#define VARx(type,name) VAR(name, type name)
VARx(ev_tstamp, mn_now)
VARx(int, timercnt)
VARx(ANHE *, timers)

// ev.c 中两次 include ev_vars.h，第一次展开为结构体成员
struct ev_loop {
  #define VAR(name,decl) decl;
  #include "ev_vars.h"    // 展开为：ev_tstamp mn_now; int timercnt; ANHE *timers;
  #undef VAR
};

// ev_wrap.h 中为每个变量生成访问宏
#define mn_now    ((loop)->mn_now)
#define timercnt  ((loop)->timercnt)
#define timers    ((loop)->timers)
```

**效果**：在 `ev.c` 中直接写 `timercnt`，编译器展开为 `((loop)->timercnt)`，既支持多 loop 模式，又保持代码简洁。

**好处**：

- 新增变量只需在 `ev_vars.h` 加一行
- 自动同步到 `ev_wrap.h`（由 `update_ev_wrap` 脚本生成）
- 单 loop 模式下 `EV_P` 为 `void`，宏展开为全局变量，零开销

### 条件编译：特性开关体系

libev 通过位掩码 `EV_FEATURES` 控制编译特性，实现可裁剪：

```
特性宏                位    功能说明              影响范围
EV_FEATURE_CODE       1    代码相关特性          内联优化、分支预测
EV_FEATURE_DATA       2    数据相关特性          四叉堆、堆缓存
EV_FEATURE_CONFIG     4    配置相关特性          多循环、优先级系统
EV_FEATURE_API        8    API 相关特性          高级 API、统计信息
EV_FEATURE_WATCHERS  16    Watcher 特性          各类事件监听器
EV_FEATURE_BACKENDS  32    后端特性              多种 IO 多路复用后端
EV_FEATURE_OS        64    操作系统特性          OS 系统调用、平台特性
```

**使用方式**：每个 Watcher 类型都有对应的使能宏

```c
#ifndef EV_PERIODIC_ENABLE
# define EV_PERIODIC_ENABLE EV_FEATURE_WATCHERS  // 默认跟随 EV_FEATURE_WATCHERS
#endif
```

**后端选择**同理：

```c
#ifndef EV_USE_EPOLL
# if __linux && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 4))
#  define EV_USE_EPOLL EV_FEATURE_BACKENDS       // Linux + glibc >= 2.4 才启用
# else
#  define EV_USE_EPOLL 0
# endif
#endif
```

**效果**：`-DEV_FEATURES=0x7c` 就能关闭 CODE 和 DATA 特性，减小二进制体积。

### 多循环支持：EV\_P / EV\_A 宏体系

```c
// 多循环模式（EV_MULTIPLICITY = EV_FEATURE_CONFIG）
#define EV_P   struct ev_loop *loop          // 函数声明参数
#define EV_P_  EV_P,                         // 函数声明参数（后面还有参数）
#define EV_A   loop                          // 函数调用参数
#define EV_A_  EV_A,                         // 函数调用参数（后面还有参数）

// 单循环模式（EV_MULTIPLICITY = 0）
#define EV_P   void
#define EV_P_
#define EV_A
#define EV_A_
```

**使用示例**：

```c
// 声明
void ev_io_start (EV_P_ ev_io *w);   // 多循环：void ev_io_start(struct ev_loop *loop, ev_io *w)
                                      // 单循环：void ev_io_start(ev_io *w)

// 调用
ev_io_start (EV_A_ &w);              // 多循环：ev_io_start(loop, &w)
                                      // 单循环：ev_io_start(&w)
```

### Watcher 继承体系：宏模拟面向对象

libev 用宏模拟了 C 的"继承"：

```
ev_watcher          ← 基类（所有 watcher 的最小公共集）
  ├── active        // 是否正在监听
  ├── pending       // 是否在 pending 队列
  ├── priority      // 优先级
  ├── data          // 用户自定义数据
  └── cb            // 回调函数

ev_watcher_list     ← 继承 ev_watcher + next 指针（用于链表）
  └── next

ev_watcher_time     ← 继承 ev_watcher + at 时间戳（用于定时器）
  └── at
```

**宏展开过程**：

```c
#define EV_WATCHER(type)            \
  int active;                       \
  int pending;                      \
  EV_DECL_PRIORITY                  \
  EV_COMMON                         \
  EV_CB_DECLARE(type)

#define EV_WATCHER_LIST(type)       \
  EV_WATCHER(type)                  \
  struct ev_watcher_list *next;

#define EV_WATCHER_TIME(type)       \
  EV_WATCHER(type)                  \
  ev_tstamp at;

// 具体 watcher 继承
typedef struct ev_io {
  EV_WATCHER_LIST(ev_io)   // 继承：active, pending, priority, data, cb, next
  int fd;                   // 自有：监听的文件描述符
  int events;               // 自有：监听的事件类型
} ev_io;

typedef struct ev_timer {
  EV_WATCHER_TIME(ev_timer) // 继承：active, pending, priority, data, cb, at
  ev_tstamp repeat;         // 自有：重复间隔
} ev_timer;
```

**三种基类的使用场景**：


| 基类              | 额外字段 | 使用者                                                             |
| ----------------- | -------- | ------------------------------------------------------------------ |
| `EV_WATCHER`      | 无       | ev\_idle, ev\_prepare, ev\_check, ev\_fork, ev\_cleanup, ev\_async |
| `EV_WATCHER_LIST` | next     | ev\_io, ev\_signal, ev\_child, ev\_stat（需要链表管理）            |
| `EV_WATCHER_TIME` | at       | ev\_timer, ev\_periodic（需要超时时间）                            |

### 事件类型：位掩码设计

```c
enum {
  EV_READ     = 0x01,        // IO 读就绪
  EV_WRITE    = 0x02,        // IO 写就绪
  EV_TIMER    = 0x00000100,  // 定时器超时
  EV_PERIODIC = 0x00000200,  // 周期定时器
  EV_SIGNAL   = 0x00000400,  // 信号到达
  EV_CHILD    = 0x00000800,  // 子进程状态变化
  EV_STAT     = 0x00001000,  // 文件状态变化
  EV_IDLE     = 0x00002000,  // 事件循环空闲
  EV_PREPARE  = 0x00004000,  // 即将进入 IO 等待
  EV_CHECK    = 0x00008000,  // 刚完成 IO 等待
  EV_EMBED    = 0x00010000,  // 嵌入循环需要扫描
  EV_FORK     = 0x00020000,  // fork 后子进程恢复
  EV_CLEANUP  = 0x00040000,  // 事件循环销毁
  EV_ASYNC    = 0x00080000,  // 跨线程异步信号
  EV_CUSTOM   = 0x01000000,  // 用户自定义事件
  EV_ERROR    = 0x80000000   // 错误发生
};
```

**设计特点**：

- IO 事件（READ/WRITE）在低 8 位，可直接传给 `select`/`poll`
- 各 watcher 类型在不同位段，可按位或组合
- `EV_ERROR` 在最高位，作为错误标志

### 优先级系统

```c
#ifndef EV_MINPRI
# define EV_MINPRI (EV_FEATURE_CONFIG ? -2 : 0)
#endif
#ifndef EV_MAXPRI
# define EV_MAXPRI (EV_FEATURE_CONFIG ? +2 : 0)
#endif

#define NUMPRI (EV_MAXPRI - EV_MINPRI + 1)  // 优先级数量：5（-2,-1,0,+1,+2）
```

**pending 队列按优先级组织**：

```c
VAR (pendings, ANPENDING *pendings [NUMPRI])   // 每个优先级一个 pending 数组
VAR (pendingcnt, int pendingcnt [NUMPRI])       // 每个优先级的待处理数量
VARx(int, pendingpri)                           // 当前最高待处理优先级
```

**调度规则**：`ev_invoke_pending` 从高优先级到低优先级依次执行，高优先级回调先执行。

### 内存管理：动态数组 + 倍增扩容

```c
// 倍增扩容策略
inline_size int
array_nextsize (int elem, int cur, int cnt)
{
  int ncur = cur + 1;
  do
    ncur <<= 1;           // 容量翻倍
  while (cnt > ncur);

  // 大数组对齐到 MALLOC_ROUND（4096）
  if (elem * ncur > MALLOC_ROUND - sizeof(void *) * 4)
    ncur = (ncur * elem + MALLOC_ROUND - 1) & ~(MALLOC_ROUND - 1);
  return ncur;
}

// 通用数组扩容宏
#define array_needsize(type,base,cur,cnt,init)  \
  if (expect_false((cnt) > (cur)))              \
    {                                            \
      int ocur_ = (cur);                         \
      (base) = (type *)array_realloc(...);       \
      init((base) + ocur_, (cur) - ocur_);       \
    }
```

**特点**：

- 小数组：倍增扩容（2, 4, 8, 16, ...）
- 大数组：对齐到 4096 字节，减少内存碎片
- 自定义 `init` 回调初始化新分配的内存

### 分支预测优化

```c
#define expect_false(cond) ecb_expect_false(cond)   // 预测条件为假的概率高
#define expect_true(cond)  ecb_expect_true(cond)    // 预测条件为真的概率高
```

**使用场景**：

```c
if (expect_false(ev_is_active(w)))   // 大多数情况下 watcher 未激活，冷路径
  return;

if (expect_true(have_monotonic))     // 大多数系统支持单调时钟，热路径
  { ... }
```

### 函数属性注解

```c
ecb_cold      // 冷函数（很少调用），编译器优化代码布局
noinline      // 禁止内联，避免代码膨胀
inline_speed  // 强制内联（性能关键路径）
ecb_const     // 纯函数（无副作用，返回值只依赖参数）
EV_THROW      // noexcept（C++）或 throw()（C++03）
```

**使用示例**：

```c
unsigned int ecb_cold                    // 冷函数：初始化相关，很少调用
ev_supported_backends(void) EV_THROW;

void noinline                           // 不内联：错误处理等大函数
ev_invoke_pending(EV_P);

inline_speed void                       // 强制内联：热路径上的小函数
ev_start(EV_P_ W w, int active);
```

### 后端抽象：函数指针多态

libev 通过函数指针实现后端多态，避免大量 `if-else`：

```c
// ev_loop 中的函数指针成员
VARx(void (*backend_modify)(EV_P_ int fd, int oev, int nev), backend_modify)
VARx(void (*backend_poll)(EV_P_ ev_tstamp waittime), backend_poll)

// 各后端在 init 时设置函数指针
// ev_epoll.c
static int epoll_init(EV_P_ int flags) {
  backend_modify = epoll_modify;   // 设置修改函数
  backend_poll   = epoll_poll;     // 设置轮询函数
  return EVBACKEND_EPOLL;
}

// ev_kqueue.c
static int kqueue_init(EV_P_ int flags) {
  backend_modify = kqueue_modify;
  backend_poll   = kqueue_poll;
  return EVBACKEND_KQUEUE;
}

// ev.c 中统一调用
fd_reify(EV_P) {
  backend_modify(EV_A_ fd, oev, nev);  // 不关心具体后端
}

ev_run(EV_P_ int flags) {
  backend_poll(EV_A_ waittime);         // 不关心具体后端
}
```

**好处**：新增后端只需实现 `xxx_init`、`xxx_modify`、`xxx_poll`，无需修改核心逻辑。

### 后端文件的条件包含

```c
// ev.c 中按条件包含后端实现
#if EV_USE_IOCP
# include "ev_iocp.c"
#endif
#if EV_USE_PORT
# include "ev_port.c"
#endif
#if EV_USE_KQUEUE
# include "ev_kqueue.c"
#endif
#if EV_USE_EPOLL
# include "ev_epoll.c"
#endif
#if EV_USE_POLL
# include "ev_poll.c"
#endif
#if EV_USE_SELECT
# include "ev_select.c"
#endif
```

**注意**：不是链接 `.o` 文件，而是直接 `#include` 源文件。这样后端代码可以直接访问 `ev.c` 中的 static 函数和宏，无需暴露内部接口。

### Watcher 生命周期：init → start → (active) → stop

所有 watcher 遵循统一的生命周期模式：

```c
// 1. 定义 watcher 变量
ev_io w;

// 2. 初始化（设置回调和参数，不启动）
ev_io_init(&w, io_cb, fd, EV_READ);

// 3. 启动（注册到事件循环）
ev_io_start(loop, &w);

// 4. 事件循环运行中，回调被自动调用
// io_cb(loop, &w, revents) { ... }

// 5. 停止（从事件循环注销）
ev_io_stop(loop, &w);
```

**内部实现**：

```c
// ev_start：设置 active 标志 + 增加 activecnt
inline_speed void
ev_start(EV_P_ W w, int active) {
  pri_adjust(EV_A_ w);   // 调整优先级到合法范围
  w->active = active;     // active = 数组索引 + 1（0 表示未激活）
  ev_ref(EV_A);           // ++activecnt，阻止 ev_run 退出
}

// ev_stop：清除 active 标志 + 减少 activecnt
inline_size void
ev_stop(EV_P_ W w) {
  ev_unref(EV_A);         // --activecnt
  w->active = 0;
}
```

### Pipe 机制：信号和异步事件的桥梁

libev 使用 pipe 将信号/异步事件统一到 IO 事件流中：

```
信号到达 → ev_sighandler() → write(pipe[1]) → pipe[0] 可读
                                                  ↓
异步通知 → ev_async_send() → write(pipe[1]) → ev_run 中 backend_poll 检测到 pipe[0] 可读
                                                  ↓
                                            pipecb() 读 pipe[0]
                                                  ↓
                                            遍历 signal/async watcher，加入 pending
```

**关键变量**：

```c
VARx(ev_io, pipe_w)               // 监听 pipe[0] 的 IO watcher
VARx(int [2], evpipe)             // pipe 的读写端 fd
VARx(EV_ATOMIC_T, pipe_write_skipped)  // 写 pipe 失败标记
VARx(EV_ATOMIC_T, pipe_write_wanted)   // 需要写 pipe 标记
```

### ev\_loop 结构体组成

```c
struct ev_loop {
  ev_tstamp ev_rt_now;           // 实时时间（单独定义，不在 ev_vars.h 中）

  // === 时间管理 ===
  // mn_now, now_floor, rtmn_diff

  // === 后端 ===
  // backend, backend_fd, backend_mintime, backend_modify, backend_poll

  // === fd 管理 ===
  // anfds[], anfdmax, fdchanges[], fdchangecnt, fdchangemax

  // === 定时器 ===
  // timers[], timercnt, timermax        (ev_timer 最小堆)
  // periodics[], periodiccnt, periodicmax (ev_periodic 最小堆)

  // === pending 队列 ===
  // pendings[NUMPRI][], pendingcnt[NUMPRI], pendingmax[NUMPRI], pendingpri

  // === 各类 watcher 数组 ===
  // prepares[], checks[], idles[], forks[], cleanups[], asyncs[]

  // === 信号管理 ===
  // signals[EV_NSIG-1], sig_pending, sigfd, sigfd_set, sigfd_w

  // === 子进程管理 ===
  // childs[EV_PID_HASHSIZE], childev

  // === pipe 机制 ===
  // evpipe[2], pipe_w, pipe_write_skipped, pipe_write_wanted

  // === 循环控制 ===
  // activecnt, loop_done, loop_count, loop_depth, postfork, curpid

  // === 后端私有数据 ===
  // epoll_events, epoll_eventmax, ...     (epoll)
  // kqueue_changes, kqueue_events, ...    (kqueue)
  // polls, pollidxs, ...                  (poll)
  // vec_ri, vec_wi, vec_ro, vec_wo, ...   (select)
};
```

## watcher

### ev_io

#### 结构体定义

```c
typedef struct ev_io {
  EV_WATCHER_LIST(ev_io)    // 继承：active, pending, priority, data, cb, next
  int fd;                    // 监听的文件描述符（ro：启动后不可修改）
  int events;                // 监听的事件类型（ro：启动后不可修改）
} ev_io;
```

**字段说明**：


| 字段     | 类型     | 权限    | 说明                                                         |
| -------- | -------- | ------- | ------------------------------------------------------------ |
| active   | int      | private | 是否正在监听（0=未激活，非0=数组索引+1）                     |
| pending  | int      | private | 是否在 pending 队列等待调度（0=不在，非0=pending 数组索引）  |
| priority | int      | private | 优先级（-2\~+2，值越大越先执行）                             |
| data     | void\*   | rw      | 用户自定义数据                                               |
| cb       | 函数指针 | private | 回调函数`void cb(EV_P_ ev_io *w, int revents)`               |
| next     | 指针     | private | 链表节点（同一 fd 的多个 watcher 组成链表）                  |
| fd       | int      | ro      | 监听的文件描述符                                             |
| events   | int      | ro      | 监听的事件类型（EV\_READ / EV\_WRITE / EV\_READ\|EV\_WRITE） |

**为什么用 EV\_WATCHER\_LIST？** 因为同一个 fd 可以被多个 ev\_io watcher 监听（比如一个监听读，一个监听写），它们通过 `next` 指针组成链表挂在 ANFD 上。

#### 相关函数

```c
// 初始化（宏，不启动）
ev_io_init(ev, cb, fd, events)    // = ev_init + ev_io_set
ev_io_set(ev, fd_, events_)       // 设置 fd 和 events（会加上 EV__IOFDSET 标记）

// 启动 / 停止
void ev_io_start(EV_P_ ev_io *w)  // 注册到事件循环
void ev_io_stop(EV_P_ ev_io *w)   // 从事件循环注销

// 手动投递事件（不经过 backend_poll）
void ev_feed_fd_event(EV_P_ int fd, int revents)

// 状态查询
ev_is_active(w)                   // watcher 是否已启动
ev_is_pending(w)                  // watcher 是否在 pending 队列
ev_cb(w)                          // 获取回调函数
```

**ev\_io\_init 展开过程**：

```c
ev_io_init(&w, io_cb, sockfd, EV_READ);

// 展开为：
do {
  // ev_init：清零 active/pending，设置 priority=0，设置 cb
  ((ev_watcher*)&w)->active  = 0;
  ((ev_watcher*)&w)->pending = 0;
  ev_set_priority(&w, 0);
  ev_set_cb(&w, io_cb);

  // ev_io_set：设置 fd 和 events
  w.fd = sockfd;
  w.events = EV_READ | EV__IOFDSET;  // EV__IOFDSET 是内部标记，表示 fd 新设置，需要注册到内核
} while (0)
```

#### 核心数据结构

**ANFD（每个 fd 一个）**：

```c
typedef struct {
  WL head;              // 监听该 fd 的 watcher 链表头
  unsigned char events; // 该 fd 所有 watcher 监听事件的并集
  unsigned char reify;  // 需要重新同步到内核的标记（EV_ANFD_REIFY / EV__IOFDSET）
  unsigned char emask;  // epoll 专用：实际注册到内核的事件掩码
} ANFD;

VARx(ANFD *, anfds)     // anfds 数组，以 fd 为索引
VARx(int, anfdmax)      // anfds 数组容量
```

**fdchanges 数组**：

```c
VARx(int *, fdchanges)  // 需要重新同步到内核的 fd 列表
VARx(int, fdchangecnt)  // 当前需要同步的 fd 数量
VARx(int, fdchangemax)  // fdchanges 数组容量
```

**ANPENDING（pending 队列元素）**：

```c
typedef struct {
  W w;           // watcher 指针
  int events;    // 待处理的事件类型
} ANPENDING;

VAR (pendings, ANPENDING *pendings[NUMPRI])  // 按优先级分桶的 pending 数组
VAR (pendingcnt, int pendingcnt[NUMPRI])      // 每个优先级的 pending 数量
```

#### ev\_io\_start 流程

```
ev_io_start(loop, &w)
│
├── 1. 防重入检查
│     if (ev_is_active(w)) return;   // 已启动则直接返回
│
├── 2. 参数校验
│     assert(fd >= 0);
│     assert(events 合法);
│
├── 3. ev_start(loop, w, 1)
│     ├── pri_adjust：调整优先级到合法范围
│     ├── w->active = 1
│     └── ++activecnt              // 阻止 ev_run 退出
│
├── 4. 扩容 anfds 数组
│     array_needsize(ANFD, anfds, anfdmax, fd+1, ...)
│     // 确保 anfds 数组大小 >= fd+1
│
├── 5. 加入 watcher 链表
│     wlist_add(&anfds[fd].head, &w)
│     // 头插法：w->next = anfds[fd].head; anfds[fd].head = w;
│
└── 6. 标记 fd 需要同步
      fd_change(loop, fd, w->events & EV__IOFDSET | EV_ANFD_REIFY)
      ├── anfds[fd].reify |= flags
      └── if (reify 之前为 0)
            fdchanges[fdchangecnt++] = fd   // 加入待同步列表
```

#### ev\_io\_stop 流程

```
ev_io_stop(loop, &w)
│
├── 1. 清除 pending 状态
│     clear_pending(loop, w)
│     // 如果 w 在 pending 队列中，将其替换为无效 watcher
│
├── 2. 防重入检查
│     if (!ev_is_active(w)) return;  // 未启动则直接返回
│
├── 3. 从 watcher 链表移除
│     wlist_del(&anfds[w.fd].head, &w)
│     // 遍历链表，找到 w 并移除
│
├── 4. ev_stop(loop, w)
│     ├── --activecnt              // 允许 ev_run 退出
│     └── w->active = 0
│
└── 5. 标记 fd 需要同步
      fd_change(loop, w.fd, EV_ANFD_REIFY)
      // 即使 watcher 被移除，fd 上的其他 watcher 可能还在
      // 需要重新计算 anfds[fd].events 并同步到内核
```

#### fd\_reify：将 fd 变更同步到内核

`ev_io_start` / `ev_io_stop` 只是修改了 anfds 链表和 fdchanges 数组，**并没有立即调用 epoll\_ctl / kevent**。真正的同步发生在 `ev_run` 每次迭代的 `fd_reify()` 中：

```
fd_reify(loop)
│
├── 遍历 fdchanges[] 数组
│     for (i = 0; i < fdchangecnt; i++)
│       fd = fdchanges[i]
│       anfd = anfds[fd]
│
├── 重新计算该 fd 的 events 并集
│     anfd->events = 0
│     for (w = anfd->head; w; w = w->next)
│       anfd->events |= w->events    // 合并所有 watcher 的事件
│
├── 比较新旧 events
│     if (o_events != anfd->events)
│       o_reify = EV__IOFDSET        // 事件变了，需要重新注册
│
└── 调用 backend_modify 同步到内核
      if (o_reify & EV__IOFDSET)
        backend_modify(loop, fd, o_events, anfd->events)
        // epoll: epoll_ctl(EPOLL_CTL_MOD, ...)
        // kqueue: EV_SET(kevent, ...)
        // select: 修改 fd_set
```

**为什么要延迟同步？** 因为一次 `ev_run` 迭代中可能多次 start/stop 同一个 fd，延迟到 `fd_reify` 统一处理可以合并多次修改为一次系统调用。

#### 事件触发流程

```
ev_run 每次迭代
│
├── fd_reify()                    ← 将 fd 变更同步到内核
│
├── backend_poll(waittime)        ← 阻塞等待 IO 事件
│   │
│   ├── epoll_wait()              ← 返回就绪的 fd 列表
│   │
│   └── 对每个就绪 fd：
│       fd_event(loop, fd, revents)
│         │
│         ├── 检查 anfds[fd].reify
│         │   if (reify) return;  ← fd 正在被修改，跳过
│         │
│         └── fd_event_nocheck(loop, fd, revents)
│             │
│             └── 遍历 anfds[fd].head 链表
│                 for (w = anfd->head; w; w = w->next)
│                   ev = w->events & revents   ← 取交集
│                   if (ev)
│                     ev_feed_event(loop, w, ev)
│                       │
│                       ├── 如果 w 已在 pending 队列
│                       │   pendings[pri][w->pending-1].events |= ev  ← 合并事件
│                       │
│                       └── 如果 w 不在 pending 队列
│                           w->pending = ++pendingcnt[pri]
│                           pendings[pri][w->pending-1] = {w, ev}
│
├── ... (timers, idle, check 等)
│
└── ev_invoke_pending()           ← 执行 pending 队列中的回调
    │
    └── 从高优先级到低优先级遍历
        for (pri = NUMPRI-1; pri >= 0; pri--)
          while (pendingcnt[pri])
            p = pendings[pri] + --pendingcnt[pri]
            p->w->pending = 0
            p->w->cb(loop, p->w, p->events)   ← 用户回调在这里执行
```

#### 完整生命周期示例

```c
#include <ev.h>
#include <stdio.h>
#include <unistd.h>

ev_io stdin_w;

static void stdin_readable_cb(EV_P_ ev_io *w, int revents) {
    char buf[1024];
    if (revents & EV_READ) {
        int n = read(w->fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("读取到: %s", buf);
        } else if (n == 0) {
            printf("EOF, 停止监听\n");
            ev_io_stop(EV_A_ w);  // 停止 watcher
        }
    } else if (revents & EV_ERROR) {
        printf("错误发生\n");
        ev_io_stop(EV_A_ w);
    }
}

int main() {
    struct ev_loop *loop = EV_DEFAULT;

    // 1. 初始化：设置 fd、events、callback
    ev_io_init(&stdin_w, stdin_readable_cb, STDIN_FILENO, EV_READ);

    // 2. 启动：注册到事件循环
    ev_io_start(loop, &stdin_w);

    // 3. 运行事件循环
    ev_run(loop, 0);

    return 0;
}
```

#### 关键设计要点

**1. 一个 fd 多个 watcher**

```c
// 同一个 fd 可以注册多个 watcher
ev_io_init(&w_read, read_cb, fd, EV_READ);
ev_io_init(&w_write, write_cb, fd, EV_WRITE);
ev_io_start(loop, &w_read);
ev_io_start(loop, &w_write);

// 它们挂在 anfds[fd] 的同一个链表上
// anfds[fd].head -> w_write -> w_read -> NULL
// anfds[fd].events = EV_READ | EV_WRITE（并集）

// fd 可读时：只有 w_read 的回调被调用
// fd 可写时：只有 w_write 的回调被调用
```

**2. 事件合并**

```c
// 如果 watcher 已经在 pending 队列中（还没来得及执行回调）
// 再次触发时，events 会被合并
pendings[pri][w->pending-1].events |= ev;  // 合并而非替换
```

**3. fd 变更的延迟同步**

```
ev_io_start → fd_change → 记录到 fdchanges[]
ev_io_stop  → fd_change → 记录到 fdchanges[]
                          ↓
ev_run 每次迭代 → fd_reify → 统一调用 backend_modify
```

好处：避免多次 start/stop 同一 fd 时的重复系统调用。

**4. reify 标记的作用**

```c
// backend_poll 返回事件时
fd_event(loop, fd, revents) {
  if (anfds[fd].reify)
    return;  // 跳过！因为 fd 正在被修改，事件可能已过期
}
```

防止在 fd 修改过程中触发已过期的事件回调。

**5. EV\_\_IOFDSET 内部标记**

```c
#define ev_io_set(ev, fd_, events_)  do { (ev)->fd = (fd_); (ev)->events = (events_) | EV__IOFDSET; } while (0)
```

`EV__IOFDSET = 0x80`，是内部标记，表示 fd 是新设置的。在 `ev_io_start` 中：

```c
fd_change(EV_A_ fd, w->events & EV__IOFDSET | EV_ANFD_REIFY);
w->events &= ~EV__IOFDSET;  // 用完即清
```

如果 `w->events` 包含 `EV__IOFDSET`，说明 fd 是新设置的，`fd_change` 的 flags 会带上 `EV__IOFDSET`，在 `fd_reify` 中会强制调用 `backend_modify`（即使 events 并集没变）。这是因为 epoll 等后端可能在 fork 后丢失了 fd 注册，需要强制重新注册。

### ev_signal

#### 结构体定义

```c
typedef struct ev_signal {
  EV_WATCHER_LIST(ev_signal)  // 继承：active, pending, priority, data, cb, next
  int signum;                  // 监听的信号号（ro：启动后不可修改）
} ev_signal;
```

**字段说明**：


| 字段     | 类型     | 权限    | 说明                                               |
| -------- | -------- | ------- | -------------------------------------------------- |
| active   | int      | private | 是否正在监听（0=未激活，非0=已激活）               |
| pending  | int      | private | 是否在 pending 队列等待调度                        |
| priority | int      | private | 优先级（ev_signal 不支持优先级，始终为 0）         |
| data     | void*    | rw      | 用户自定义数据                                     |
| cb       | 函数指针 | private | 回调函数`void cb(EV_P_ ev_signal *w, int revents)` |
| next     | 指针     | private | 链表节点（同一 signum 的多个 watcher 组成链表）    |
| signum   | int      | ro      | 监听的信号号（如 SIGINT、SIGTERM）                 |

**为什么用 EV_WATCHER_LIST？** 因为同一个信号可以被多个 ev_signal watcher 监听，它们通过 `next` 指针组成链表挂在 ANSIG 上。

#### 相关函数

```c
// 初始化（宏，不启动）
ev_signal_init(ev, cb, signum)       // = ev_init + ev_signal_set
ev_signal_set(ev, signum_)           // 设置 signum

// 启动 / 停止
void ev_signal_start(EV_P_ ev_signal *w)  // 注册到事件循环
void ev_signal_stop(EV_P_ ev_signal *w)   // 从事件循环注销

// 手动投递信号事件
void ev_feed_signal(int signum)            // 线程安全：设置 pending + 写 pipe
void ev_feed_signal_event(EV_P_ int signum) // 非线程安全：直接加入 pending 队列

// 状态查询
ev_is_active(w)                            // watcher 是否已启动
ev_is_pending(w)                           // watcher 是否在 pending 队列
```

#### 核心数据结构

**ANSIG（每个信号号一个）**：

```c
typedef struct {
  EV_ATOMIC_T pending;   // 信号是否已发生（原子变量，线程安全）
#if EV_MULTIPLICITY
  EV_P;                  // 绑定的 loop 指针
#endif
  WL head;               // 监听该信号的 watcher 链表头
} ANSIG;

static ANSIG signals [EV_NSIG - 1];   // 全局数组，索引 = signum - 1
```

**为什么是 `EV_NSIG - 1`？** 因为信号号从 1 开始（没有信号 0），数组索引 `i` 对应信号号 `i + 1`。

**信号相关变量**：

```c
VARx(EV_ATOMIC_T, sig_pending)        // 是否有信号需要处理
VARx(int, sigfd)                       // signalfd 的 fd（Linux 特有，-2=未初始化，-1=不可用，>=0=可用）
VARx(sigset_t, sigfd_set)             // signalfd 监听的信号集
VARx(ev_io, sigfd_w)                  // 监听 signalfd 的 IO watcher
```

**Pipe 相关变量**：

```c
VARx(int [2], evpipe)                 // pipe 的读写端 fd
VARx(ev_io, pipe_w)                   // 监听 pipe[0] 的 IO watcher
VARx(EV_ATOMIC_T, pipe_write_skipped) // 写 pipe 被跳过标记
VARx(EV_ATOMIC_T, pipe_write_wanted)  // 需要写 pipe 标记
```

#### 两种信号处理模式

libev 支持两种信号处理模式，由 `EV_USE_SIGNALFD` 宏控制：

```
模式一：signalfd（Linux 特有，推荐）
┌──────────────────────────────────────────────────┐
│  信号到达 → 内核将信号写入 signalfd               │
│           → backend_poll 检测到 sigfd_w 可读      │
│           → sigfdcb() 读取 signalfd               │
│           → ev_feed_signal_event() 加入 pending   │
└──────────────────────────────────────────────────┘

模式二：signal handler + pipe（通用，跨平台）
┌──────────────────────────────────────────────────┐
│  信号到达 → ev_sighandler() 被内核调用             │
│           → ev_feed_signal() 设置 pending 标记     │
│           → evpipe_write() 写 pipe 唤醒 loop      │
│           → backend_poll 检测到 pipe_w 可读        │
│           → pipecb() 读 pipe，遍历 signals[]       │
│           → ev_feed_signal_event() 加入 pending    │
└──────────────────────────────────────────────────┘
```

**signalfd 模式的优势**：信号在内核层面转为 fd 事件，完全在事件循环线程处理，无需 pipe 中转，无需担心信号处理函数的限制。

#### ev_signal_start 流程

```
ev_signal_start(loop, &w)
│
├── 1. 防重入检查
│     if (ev_is_active(w)) return;
│
├── 2. 参数校验
│     assert(signum > 0 && signum < EV_NSIG);
│     assert(同一信号不能绑定到两个不同的 loop);
│
├── 3. 绑定 loop
│     signals[signum-1].loop = loop;   // 多循环模式下
│
├── 4. signalfd 初始化（仅首次，sigfd == -2 时）
│     ├── signalfd(-1, ..., SFD_NONBLOCK | SFD_CLOEXEC)  // 创建 signalfd
│     ├── ev_io_init(&sigfd_w, sigfdcb, sigfd, EV_READ)  // 注册 sigfd_w
│     ├── ev_io_start(loop, &sigfd_w)                     // 启动 sigfd_w
│     └── ev_unref(loop)                                  // sigfd_w 不阻止 loop 退出
│
├── 5. signalfd 模式（sigfd >= 0）
│     ├── sigaddset(&sigfd_set, signum)      // 添加信号到监听集
│     ├── sigprocmask(SIG_BLOCK, ...)         // 阻塞该信号（由 signalfd 接管）
│     └── signalfd(sigfd, &sigfd_set, 0)     // 更新 signalfd 监听
│
├── 6. ev_start(loop, w, 1)
│     ├── w->active = 1
│     └── ++activecnt
│
├── 7. 加入 watcher 链表
│     wlist_add(&signals[signum-1].head, &w)
│
└── 8. 首个 watcher 时注册信号处理（非 signalfd 模式）
      if (!w->next)  // 该信号的首个 watcher
        ├── evpipe_init(loop)              // 初始化 pipe（仅首次）
        ├── sigaction(signum, ev_sighandler, ...)  // 注册信号处理函数
        │   ├── sa.sa_handler = ev_sighandler
        │   ├── sigfillset(&sa.sa_mask)   // 处理期间阻塞所有信号
        │   └── sa.sa_flags = SA_RESTART  // 被中断的系统调用自动重启
        └── if (EVFLAG_NOSIGMASK)
              sigprocmask(SIG_UNBLOCK, signum)  // 解除信号阻塞
```

#### ev_signal_stop 流程

```
ev_signal_stop(loop, &w)
│
├── 1. 清除 pending 状态
│     clear_pending(loop, w)
│
├── 2. 防重入检查
│     if (!ev_is_active(w)) return;
│
├── 3. 从 watcher 链表移除
│     wlist_del(&signals[signum-1].head, &w)
│
├── 4. ev_stop(loop, w)
│     ├── --activecnt
│     └── w->active = 0
│
└── 5. 最后一个 watcher 时恢复信号默认行为
      if (!signals[signum-1].head)  // 该信号已无 watcher
        ├── signals[signum-1].loop = 0   // 解绑 loop
        ├── signalfd 模式：
        │   ├── sigdelset(&sigfd_set, signum)
        │   ├── signalfd(sigfd, &sigfd_set, 0)  // 更新 signalfd
        │   └── sigprocmask(SIG_UNBLOCK, signum) // 解除信号阻塞
        └── 非 signalfd 模式：
            signal(signum, SIG_DFL)              // 恢复默认行为
```

#### 信号触发流程（pipe 模式）

```
信号到达（如用户按 Ctrl+C → SIGINT）
│
├── 内核调用 ev_sighandler(signum)
│     │
│     └── ev_feed_signal(signum)
│           ├── signals[signum-1].pending = 1    // 标记信号已发生
│           └── evpipe_write(loop, &sig_pending)
│                 ├── if (sig_pending) return;    // 已标记，避免重复
│                 ├── sig_pending = 1
│                 ├── pipe_write_skipped = 1
│                 └── if (pipe_write_wanted)      // loop 正在阻塞等待
│                       ├── pipe_write_skipped = 0
│                       └── write(evpipe[1], ...)  // 写 pipe 唤醒 loop
│
├── ev_run 中的 backend_poll 被 pipe 可读事件唤醒
│     │
│     └── pipe_w 回调被触发 → pipecb()
│           ├── read(evpipe[0], ...)              // 读 pipe，清空缓冲
│           ├── pipe_write_skipped = 0
│           └── if (sig_pending)
│                 ├── sig_pending = 0
│                 └── 遍历 signals[] 数组
│                     for (i = EV_NSIG-1; i--; )
│                       if (signals[i].pending)
│                         ev_feed_signal_event(loop, i+1)
│                           ├── signals[i].pending = 0
│                           └── 遍历 signals[i].head 链表
│                               for (w = head; w; w = w->next)
│                                 ev_feed_event(loop, w, EV_SIGNAL)
│                                   → 加入 pending 队列
│
└── ev_invoke_pending()
      └── 执行用户回调 w->cb(loop, w, EV_SIGNAL)
```

#### 信号触发流程（signalfd 模式）

```
信号到达（如用户按 Ctrl+C → SIGINT）
│
├── 内核将信号写入 signalfd（因为该信号已被 SIG_BLOCK 阻塞）
│
├── ev_run 中的 backend_poll 检测到 sigfd_w 可读
│     │
│     └── sigfd_w 回调被触发 → sigfdcb()
│           ├── read(sigfd, si, ...)              // 读取 signalfd_siginfo
│           └── 对每个 signalfd_siginfo：
│               ev_feed_signal_event(loop, sip->ssi_signo)
│                 ├── signals[signum-1].pending = 0
│                 └── 遍历 signals[signum-1].head 链表
│                     for (w = head; w; w = w->next)
│                       ev_feed_event(loop, w, EV_SIGNAL)
│                         → 加入 pending 队列
│
└── ev_invoke_pending()
      └── 执行用户回调 w->cb(loop, w, EV_SIGNAL)
```

#### evpipe_init：pipe 的惰性初始化

pipe 不是在 `ev_loop_new` 时创建，而是在首次需要时（第一个 signal 或 async watcher 启动时）才创建：

```
evpipe_init(loop)
│
├── if (ev_is_active(&pipe_w)) return;   // 已初始化，跳过
│
├── 创建通信通道（优先 eventfd，回退 pipe）
│   ├── eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)   // Linux: eventfd 更高效
│   └── pipe(fds)                                  // 通用: pipe 兼容性好
│
├── 保存 fd
│   ├── evpipe[0] = fds[0]   // 读端
│   └── evpipe[1] = fds[1]   // 写端
│
├── fd_intern()  // 设置 O_NONBLOCK + FD_CLOEXEC
│
└── 注册 pipe_w 监听读端
    ├── ev_io_set(&pipe_w, evpipe[0], EV_READ)
    ├── ev_io_start(loop, &pipe_w)
    └── ev_unref(loop)  // pipe_w 不阻止 loop 退出
```

**eventfd vs pipe**：


| 特性     | eventfd                  | pipe                 |
| -------- | ------------------------ | -------------------- |
| fd 数量  | 1 个                     | 2 个（读写端）       |
| 缓冲区   | 内核计数器，无缓冲区限制 | 内核缓冲区，可能满   |
| 性能     | 更快（无数据拷贝）       | 稍慢（需要拷贝数据） |
| 可移植性 | Linux 2.6.22+            | POSIX 通用           |

#### 完整生命周期示例

```c
#include <ev.h>
#include <stdio.h>
#include <signal.h>

ev_signal sigint_w;

static void sigint_cb(EV_P_ ev_signal *w, int revents) {
    printf("收到 SIGINT (Ctrl+C)，退出事件循环\n");
    ev_break(EV_A_ EVBREAK_ALL);
}

int main() {
    struct ev_loop *loop = EV_DEFAULT;

    // 1. 初始化：设置 signum 和 callback
    ev_signal_init(&sigint_w, sigint_cb, SIGINT);

    // 2. 启动：注册到事件循环
    ev_signal_start(loop, &sigint_w);

    // 3. 运行事件循环
    printf("等待 SIGINT，按 Ctrl+C 退出\n");
    ev_run(loop, 0);

    return 0;
}
```

#### 关键设计要点

**1. 信号到 IO 事件的统一**

```
信号（异步事件）→ pipe/signalfd → IO 事件（同步事件）
```

信号是异步的，信号处理函数中能做的事情非常有限（不能调用非异步安全函数）。libev 通过 pipe/signalfd 将信号转为 IO 事件，在事件循环的正常流程中处理，避免了异步安全问题。

**2. 同一信号不能绑定多个 loop**

```c
#if EV_MULTIPLICITY
  assert(("libev: a signal must not be attached to two different loops",
          !signals[signum-1].loop || signals[signum-1].loop == loop));
#endif
```

因为 `signals[]` 是全局数组，每个信号号只能绑定一个 loop。这是信号本身的特性决定的——信号是进程级别的，不是线程级别的。

**3. evpipe_write 的原子性保证**

```c
evpipe_write(loop, &sig_pending) {
  if (*flag) return;          // 原子检查：信号已标记，跳过
  *flag = 1;                  // 原子设置：标记信号发生
  pipe_write_skipped = 1;     // 原子设置：标记写被跳过
  if (pipe_write_wanted) {    // 原子检查：loop 是否在等待
    pipe_write_skipped = 0;
    write(evpipe[1], ...);    // 写 pipe 唤醒 loop
  }
}
```

所有标志位都是 `EV_ATOMIC_T`（原子类型），确保信号处理函数和事件循环线程之间的安全通信。

**4. pipe_write_skipped 的作用**

```
场景：信号在 backend_poll 之外到达
  ├── loop 不在阻塞等待 → pipe_write_wanted = 0
  ├── evpipe_write 不写 pipe → pipe_write_skipped = 1
  └── 下次 ev_run 迭代开始时检查：
      if (pipe_write_skipped) → 不进入阻塞等待，直接处理
```

确保即使 pipe 没有被写入，信号也不会丢失。

**5. sigfd_w 和 pipe_w 不阻止 loop 退出**

```c
ev_io_start(loop, &sigfd_w);
ev_unref(loop);   // --activecnt，不阻止 loop 退出

ev_io_start(loop, &pipe_w);
ev_unref(loop);   // --activecnt，不阻止 loop 退出
```

这两个内部 watcher 是辅助性的，不应影响 `ev_run` 的退出条件（`activecnt == 0`）。

**6. SA_RESTART 标志**

```c
sa.sa_flags = SA_RESTART;
```

信号到达时，被中断的系统调用（如 `read`、`write`）会自动重启，而不是返回 `EINTR`。这简化了 libev 的错误处理逻辑。

**7. 信号处理函数中的信号屏蔽**

```c
sigfillset(&sa.sa_mask);  // 在信号处理函数执行期间阻塞所有其他信号
```

防止信号处理函数被另一个信号中断，确保 `ev_feed_signal` 的原子性。

### ev_timer

#### 结构体定义

```c
typedef struct ev_timer {
  EV_WATCHER_TIME(ev_timer)   // 继承：active, pending, priority, data, cb, at
  ev_tstamp repeat;            // rw：重复间隔（0=不重复）
} ev_timer;
```

**字段说明**：


| 字段     | 类型      | 权限    | 说明                                                     |
| -------- | --------- | ------- | -------------------------------------------------------- |
| active   | int       | private | 是否正在监听（0=未激活，非0=在 timers 堆中的索引+HEAP0） |
| pending  | int       | private | 是否在 pending 队列等待调度                              |
| priority | int       | rw      | 优先级（-2~+2）                                          |
| data     | void\*    | rw      | 用户自定义数据                                           |
| cb       | 函数指针  | rw      | 回调函数`void cb(EV_P_ ev_timer *w, int revents)`        |
| at       | ev_tstamp | private | 绝对超时时间（mn_now + after），堆排序的依据             |
| repeat   | ev_tstamp | rw      | 重复间隔。0=一次性定时器，>0=重复定时器                  |

**为什么用 EV_WATCHER_TIME？** 因为 ev_timer 需要记录超时时间 `at`，用于最小堆排序。与 ev_io 不同，ev_timer 不需要链表（同一时间点不会"共享"一个 timer），所以不需要 `next` 指针。

**at 与 repeat 的关系**：

```
ev_timer_init(&w, cb, after, repeat)
  → ev_timer_set: at = after, repeat = repeat
  → ev_timer_start: at += mn_now   （将相对时间转为绝对时间）

at 是绝对超时时间 = mn_now + after
repeat 是重复间隔（相对时间）
```

#### 相关函数

```c
// 初始化（宏，不启动）
ev_timer_init(ev, cb, after, repeat)    // = ev_init + ev_timer_set
ev_timer_set(ev, after, repeat)         // 设置 at=after, repeat=repeat

// 启动 / 停止
void ev_timer_start(EV_P_ ev_timer *w)  // 加入 timers 最小堆
void ev_timer_stop(EV_P_ ev_timer *w)   // 从 timers 最小堆移除

// 重新调度
void ev_timer_again(EV_P_ ev_timer *w)  // 重启/重新调度定时器

// 状态查询
ev_tstamp ev_timer_remaining(EV_P_ ev_timer *w)  // 返回剩余时间
ev_is_active(w)          // watcher 是否已启动
ev_is_pending(w)         // watcher 是否在 pending 队列
```

#### 核心数据结构

**ANHE（最小堆节点）**：

```c
#if EV_HEAP_CACHE_AT
  // 缓存 at 的版本（默认，性能更好）
  typedef struct {
    ev_tstamp at;    // 缓存的超时时间，避免间接访问
    WT w;            // watcher 指针
  } ANHE;

  #define ANHE_w(he)        (he).w              // 访问 watcher
  #define ANHE_at(he)       (he).at             // 访问缓存的 at
  #define ANHE_at_cache(he) (he).at = (he).w->at // 从 watcher 同步 at 到缓存
#else
  // 不缓存 at 的版本（省内存）
  typedef WT ANHE;

  #define ANHE_w(he)        (he)        // 直接就是 watcher 指针
  #define ANHE_at(he)       (he)->at    // 需要间接访问
  #define ANHE_at_cache(he)             // 无需同步
#endif
```

**为什么缓存 at？** 最小堆的核心操作（upheap/downheap）需要频繁比较 `at` 值。缓存后，比较操作只需访问 `ANHE.at`（连续内存），无需通过指针间接访问 `watcher->at`，对 CPU 缓存更友好。代价是每个堆节点多占 8 字节（一个 ev_tstamp）。

**timers 堆变量**：

```c
VARx(ANHE *, timers)    // 定时器堆数组（按 at 排序的最小堆）
VARx(int, timermax)     // timers 数组最大容量
VARx(int, timercnt)     // timers 数组当前数量
```

#### 最小堆实现

libev 的 timers 使用**最小堆**管理，堆顶（HEAP0）始终是最早超时的 timer。支持两种堆实现：

**二叉堆（EV_USE_4HEAP=0）**：

```c
#define HEAP0 1                    // 堆起始下标 = 1
#define HPARENT(k) ((k) >> 1)     // 父节点 = k/2
#define UPHEAP_DONE(p,k) (!(p))   // p==0 终止
```

**四叉堆（EV_USE_4HEAP=1，默认）**：

```c
#define DHEAP 4                              // 分支数：4叉堆
#define HEAP0 (DHEAP - 1)                    // 堆起始下标 = 3
#define HPARENT(k) ((((k) - HEAP0 - 1) / DHEAP) + HEAP0)  // 父节点
#define UPHEAP_DONE(p,k) ((p) == (k))       // p==k 终止
```

**为什么用四叉堆？** 四叉堆比二叉堆有更好的 CPU 缓存局部性。虽然比较次数略多，但内存访问更集中。在 50000+ watcher 场景下性能提升约 5%。

**三种堆操作**：


| 操作       | 函数                     | 场景                            | 复杂度   |
| ---------- | ------------------------ | ------------------------------- | -------- |
| 上浮       | `upheap(heap, k)`        | 新元素插入、at 减小             | O(log N) |
| 下沉       | `downheap(heap, N, k)`   | 元素删除、at 增大               | O(log N) |
| 自适应调整 | `adjustheap(heap, N, k)` | at 值变化后，自动选择上浮或下沉 | O(log N) |

```c
// adjustheap：根据 at 值变化方向自动选择
inline_size void
adjustheap(ANHE *heap, int N, int k) {
  if (k > HEAP0 && ANHE_at(heap[k]) <= ANHE_at(heap[HPARENT(k)]))
    upheap(heap, k);    // 比父节点小 → 上浮
  else
    downheap(heap, N, k); // 比父节点大 → 下沉
}
```

#### ev_timer_start 流程

```c
void ev_timer_start(EV_P_ ev_timer *w) {
  if (ev_is_active(w))    // 已激活则直接返回
    return;

  ev_at(w) += mn_now;     // 相对时间 → 绝对时间：at = at + mn_now

  ++timercnt;
  ev_start(EV_A_ (W)w, timercnt + HEAP0 - 1);  // 设置 active = 堆索引

  array_needsize(ANHE, timers, timermax, ev_active(w) + 1, EMPTY2);  // 扩容
  ANHE_w(timers[ev_active(w)]) = (WT)w;        // 放入堆数组
  ANHE_at_cache(timers[ev_active(w)]);          // 同步 at 缓存
  upheap(timers, ev_active(w));                 // 上浮到正确位置
}
```

**流程图**：

```
ev_timer_start(loop, &w)
│
├── 1. 检查是否已激活 → 已激活则返回
│
├── 2. at += mn_now（相对时间 → 绝对时间）
│     例：at=5.0, mn_now=100.0 → at=105.0
│
├── 3. ++timercnt, ev_start（设置 active 标志 + 增加 activecnt）
│
├── 4. 扩容 timers 数组（如果需要）
│
├── 5. timers[active] = w（放入堆末尾）
│
└── 6. upheap（上浮到正确位置，维护最小堆性质）
```

#### ev_timer_stop 流程

```c
void ev_timer_stop(EV_P_ ev_timer *w) {
  clear_pending(EV_A_ (W)w);   // 清除 pending 状态
  if (!ev_is_active(w))        // 未激活则返回
    return;

  int active = ev_active(w);   // 获取在堆中的索引
  --timercnt;

  if (active < timercnt + HEAP0) {
    // 不是最后一个元素：用末尾元素填补空位，然后调整堆
    timers[active] = timers[timercnt + HEAP0];
    adjustheap(timers, timercnt, active);  // 自适应调整
  }
  // 如果是最后一个元素，直接 --timercnt 即可

  ev_at(w) -= mn_now;          // 绝对时间 → 相对时间（恢复初始值）
  ev_stop(EV_A_ (W)w);         // 清除 active 标志 + 减少 activecnt
}
```

**流程图**：

```
ev_timer_stop(loop, &w)
│
├── 1. clear_pending：清除 pending 状态
│
├── 2. 检查是否已激活 → 未激活则返回
│
├── 3. 获取堆索引 active = ev_active(w)
│
├── 4. --timercnt
│
├── 5. 从堆中移除：
│     ├── 若非末尾元素：用末尾元素填补 → adjustheap 自适应调整
│     └── 若是末尾元素：直接移除（无需调整）
│
├── 6. at -= mn_now（绝对时间 → 相对时间，恢复初始值）
│
└── 7. ev_stop：清除 active 标志 + 减少 activecnt
```

**注意**：`ev_at(w) -= mn_now` 使得 stop 后 watcher 的 `at` 恢复为初始的相对时间，这样下次 `ev_timer_start` 时 `at += mn_now` 才能得到正确的绝对超时时间。

#### ev_timer_again 流程

`ev_timer_again` 是一个"智能重启"函数，根据 watcher 当前状态和 repeat 值采取不同行为：

```c
void ev_timer_again(EV_P_ ev_timer *w) {
  clear_pending(EV_A_ (W)w);   // 清除 pending 状态

  if (ev_is_active(w)) {       // 已激活
    if (w->repeat) {
      ev_at(w) = mn_now + w->repeat;             // 重新计算超时时间
      ANHE_at_cache(timers[ev_active(w)]);        // 同步缓存
      adjustheap(timers, timercnt, ev_active(w)); // 调整堆
    } else {
      ev_timer_stop(EV_A_ w);  // 非重复 → 直接停止
    }
  } else if (w->repeat) {      // 未激活但有 repeat
    ev_at(w) = w->repeat;      // 设置 at = repeat（相对时间）
    ev_timer_start(EV_A_ w);   // 重新启动（start 会 at += mn_now）
  }
}
```

**行为矩阵**：


| 状态               | repeat > 0                                     | repeat == 0               |
| ------------------ | ---------------------------------------------- | ------------------------- |
| 已激活（active）   | 重新计算超时时间`at = mn_now + repeat`，调整堆 | 停止定时器`ev_timer_stop` |
| 未激活（inactive） | 设置`at = repeat`，然后 `ev_timer_start`       | 什么都不做                |

**典型用法**：

```c
// 用 ev_timer_again 实现重复定时器
ev_timer_init(&w, timeout_cb, 0., 5.);  // after=0, repeat=5
ev_timer_again(loop, &w);                // 首次启动：at=5, start后 at=mn_now+5

// 在回调中再次调用 ev_timer_again
static void timeout_cb(EV_P_ ev_timer *w, int revents) {
  // 处理超时逻辑...
  ev_timer_again(EV_A_ w);  // 重新调度：at = mn_now + 5
}
```

#### ev_timer_remaining

```c
ev_tstamp ev_timer_remaining(EV_P_ ev_timer *w) {
  return ev_at(w) - (ev_is_active(w) ? mn_now : 0.);
}
```

- 已激活：返回 `at - mn_now`（剩余时间）
- 未激活：返回 `at`（初始设置的相对时间）

#### timers_reify：处理超时的 timer

`timers_reify` 在 `ev_run` 的每次迭代中被调用，检查并处理所有已超时的 timer：

```c
void timers_reify(EV_P) {
  if (timercnt && ANHE_at(timers[HEAP0]) < mn_now) {  // 堆顶 timer 已超时
    do {
      ev_timer *w = (ev_timer *)ANHE_w(timers[HEAP0]);

      if (w->repeat) {
        // 重复定时器：重新计算超时时间，调整堆
        ev_at(w) += w->repeat;
        if (ev_at(w) < mn_now)        // 防止累积延迟
          ev_at(w) = mn_now;
        ANHE_at_cache(timers[HEAP0]);
        downheap(timers, timercnt, HEAP0);  // 下沉到正确位置
      } else {
        // 一次性定时器：停止
        ev_timer_stop(EV_A_ w);
      }

      feed_reverse(EV_A_ (W)w);       // 加入 rfeeds 临时数组
    } while (timercnt && ANHE_at(timers[HEAP0]) < mn_now);

    feed_reverse_done(EV_A_ EV_TIMER); // 批量加入 pending 队列
  }
}
```

**流程图**：

```
timers_reify(loop)
│
├── 检查：堆顶 timer 的 at < mn_now？
│   └── 否 → 返回（无超时 timer）
│
└── 是 → 循环处理所有超时 timer：
    │
    ├── 取出堆顶 timer
    │
    ├── repeat > 0？
    │   ├── 是：at += repeat（若 at < mn_now 则 at = mn_now），downheap
    │   └── 否：ev_timer_stop
    │
    ├── feed_reverse：加入 rfeeds 临时数组
    │
    └── 继续检查新的堆顶是否也超时
        │
        └── 全部处理完毕 → feed_reverse_done：批量加入 pending 队列
```

**关键设计**：

**1. 防止累积延迟**：`if (ev_at(w) < mn_now) ev_at(w) = mn_now;`

```
场景：repeat=1s，但某次回调处理耗时 3s
正常计算：at += 1 → at 仍 < mn_now → 下次迭代立即再触发
修正后：at = mn_now → 从"现在"开始重新计时，避免连续触发
```

**2. feed_reverse 批量处理**：

```c
// 先收集所有超时 timer 到 rfeeds 数组
feed_reverse(EV_A_ (W)w);

// 再统一加入 pending 队列
feed_reverse_done(EV_A_ EV_TIMER);
```

为什么不直接 `ev_feed_event`？因为如果直接加入 pending，回调中可能修改堆（start/stop 其他 timer），导致堆结构被破坏。先收集到临时数组，再统一投递，保证堆操作的完整性。

**3. 重复 timer 不 stop**：重复 timer 在超时后不会 stop，而是调整 `at` 后下沉，继续留在堆中。

#### 在 ev_run 中的角色

ev_timer 在 ev_run 的两个阶段发挥作用：

**阶段1：计算阻塞时间 waittime**

```c
// ev_run 中计算 backend_poll 的阻塞时间
waittime = MAX_BLOCKTIME;

if (timercnt) {
  ev_tstamp to = ANHE_at(timers[HEAP0]) - mn_now;  // 堆顶 timer 的剩余时间
  if (waittime > to) waittime = to;                  // 取最小值
}

// 还要考虑 periodic timer、timeout_blocktime、backend_mintime
// 最终 waittime = min(堆顶timer剩余, MAX_BLOCKTIME, ...)
```

**阶段2：处理超时 timer**

```c
// ev_run 每次迭代中
timers_reify(EV_A);  // 检查并处理超时的 ev_timer
```

**完整时间线**：

```
ev_run 每次迭代：
│
├── 1. 计算阻塞时间：取堆顶 timer 剩余时间作为 waittime 上限
│
├── 2. backend_poll(waittime)：阻塞等待 IO 事件
│     └── 最多阻塞到最早 timer 超时
│
├── 3. time_update()：更新 mn_now
│
├── 4. timers_reify()：处理所有超时 timer
│     ├── 重复 timer：调整 at，继续留在堆中
│     └── 一次性 timer：stop
│
└── 5. ev_invoke_pending()：执行 timer 回调
```

#### 完整生命周期示例

```c
#include <ev.h>
#include <stdio.h>

ev_timer timeout_w;

static void timeout_cb(EV_P_ ev_timer *w, int revents) {
    if (revents & EV_TIMER) {
        printf("Timer fired!\n");

        // 一次性定时器：自动 stop，无需手动处理
        // 重复定时器：自动重新调度，无需手动处理
    }
}

int main() {
    struct ev_loop *loop = EV_DEFAULT;

    // 1. 一次性定时器：2秒后触发
    ev_timer_init(&timeout_w, timeout_cb, 2.0, 0.);

    // 2. 重复定时器：1秒后首次触发，之后每3秒触发一次
    // ev_timer_init(&timeout_w, timeout_cb, 1.0, 3.0);

    // 3. 启动
    ev_timer_start(loop, &timeout_w);

    // 4. 查询剩余时间
    printf("Remaining: %.2f seconds\n", ev_timer_remaining(loop, &timeout_w));

    // 5. 运行事件循环
    ev_run(loop, 0);

    return 0;
}
```

#### 关键设计要点

**1. ev_timer vs ev_periodic**


| 特性               | ev_timer                    | ev_periodic                        |
| ------------------ | --------------------------- | ---------------------------------- |
| 时钟源             | 单调时钟（monotonic clock） | 实时时钟（wall clock / UTC）       |
| 时间类型           | 相对时间（after + repeat）  | 绝对时间（offset + interval）      |
| 受系统时间调整影响 | 否                          | 是                                 |
| 重复模式           | 固定间隔 repeat             | offset/interval/reschedule_cb 三种 |
| 典型用途           | 超时、心跳、重试            | 每天凌晨3点执行、每小时整点执行    |

**2. 单调时钟的意义**

ev_timer 基于 monotonic clock（`mn_now`），不受系统时间调整影响：

```
场景：设置 10 秒超时，第 5 秒时用户将系统时间调慢 1 小时

ev_timer（monotonic）：5 秒后正常触发 ✅
ev_periodic（realtime）：1 小时 5 秒后才触发 ❌
```

**3. at 的双重含义**

```
stop 状态：at = 相对时间（用户设置的 after 值）
start 状态：at = 绝对时间（mn_now + after）

ev_timer_start: at += mn_now  （相对 → 绝对）
ev_timer_stop:  at -= mn_now  （绝对 → 相对）
```

这种设计使得同一个 watcher 可以反复 start/stop，而不会累积时间偏差。

**4. timers_reschedule：时间跳变处理**

当检测到 monotonic 时钟与 realtime 时钟的偏差发生变化时（如 NTP 校正导致系统时间跳变），libev 会调整所有 timer 的超时时间：

```c
static void timers_reschedule(EV_P_ ev_tstamp adjust) {
  for (i = 0; i < timercnt; ++i) {
    ANHE *he = timers + i + HEAP0;
    ANHE_w(*he)->at += adjust;      // 调整每个 timer 的 at
    ANHE_at_cache(*he);             // 同步缓存
  }
}
```

**5. ev_timer_again 的典型应用：ev_stat**

ev_stat 内部使用 ev_timer 实现周期性文件检测：

```c
// ev_stat_start 中
ev_timer_init(&w->timer, stat_timer_cb, 0., w->interval > 0. ? w->interval : EV_STAT_INTERVAL);
ev_timer_again(EV_A_ &w->timer);  // 用 again 启动

// stat_timer_cb 中
static void stat_timer_cb(EV_P_ ev_timer *w, int revents) {
  ev_stat *w_ = (ev_stat *)((char *)w - offsetof(ev_stat, timer));
  // 检查文件状态...
  ev_timer_again(EV_A_ &w_->timer);  // 重新调度
}
```

注意：ev_stat 使用 `ev_unref` 抵消 `ev_timer_again` 带来的引用计数增加，使内部定时器不会阻止事件循环退出。

### ev_periodic

#### 结构体定义

```c
typedef struct ev_periodic {
  EV_WATCHER_TIME(ev_periodic)   // 继承：active, pending, priority, data, cb, at
  ev_tstamp offset;               // rw：首次触发偏移（绝对时间点）
  ev_tstamp interval;             // rw：重复间隔（0=不重复）
  ev_tstamp (*reschedule_cb)(struct ev_periodic *w, ev_tstamp now) EV_THROW; // rw：自定义调度回调
} ev_periodic;
```

**字段说明**：


| 字段          | 类型      | 权限    | 说明                                                        |
| ------------- | --------- | ------- | ----------------------------------------------------------- |
| active        | int       | private | 是否正在监听（0=未激活，非0=在 periodics 堆中的索引+HEAP0） |
| pending       | int       | private | 是否在 pending 队列等待调度                                 |
| priority      | int       | rw      | 优先级（-2~+2）                                             |
| data          | void\*    | rw      | 用户自定义数据                                              |
| cb            | 函数指针  | rw      | 回调函数`void cb(EV_P_ ev_periodic *w, int revents)`        |
| at            | ev_tstamp | private | 下一次超时的绝对时间（ev_rt_now 基准），堆排序的依据        |
| offset        | ev_tstamp | rw      | 绝对时间点。模式1的直接触发时间，模式2的基准偏移            |
| interval      | ev_tstamp | rw      | 重复间隔。0=一次性定时器，>0=固定间隔重复                   |
| reschedule_cb | 函数指针  | rw      | 模式3：每次触发后调用此回调，由回调决定下一次的触发时间     |

**三种调度模式**：

```
模式1：绝对时间（offset + interval=0 + reschedule_cb=NULL）
  ev_periodic_set(&w, at, 0, 0)
  行为：只在 at 时间点触发一次，触发后 stop

模式2：offset/interval（reschedule_cb=NULL）
  ev_periodic_set(&w, offset, interval, 0)
  行为：在 offset 之后，每 interval 秒触发一次
  例：offset=0, interval=3600 → 每小时整点触发

模式3：reschedule_cb（interval=0）
  ev_periodic_set(&w, 0, 0, cb)
  行为：每次触发后调用 reschedule_cb，由回调返回下一次触发时间
  例：每天凌晨3点执行（需考虑夏令时）
```

**为什么用 EV_WATCHER_TIME？** 和 ev_timer 一样，ev_periodic 需要记录超时时间 `at`，用于最小堆排序。

**at 与 offset/interval 的关系**：

```
at 是下一次超时的绝对时间（堆排序依据），在不同模式下计算方式不同：
模式1：at = offset                                    （启动时直接设置）
模式2：at = offset + interval * ceil((ev_rt_now - offset) / interval)  （启动时计算）
模式3：at = reschedule_cb(w, ev_rt_now)               （启动时调用回调计算）
```

#### 相关函数

```c
// 初始化（宏，不启动）
ev_periodic_init(ev, cb, offset, interval, reschedule_cb)  // = ev_init + ev_periodic_set
ev_periodic_set(ev, offset_, interval_, reschedule_cb_)     // 设置三种模式参数

// 启动 / 停止
void ev_periodic_start(EV_P_ ev_periodic *w)  // 加入 periodics 最小堆
void ev_periodic_stop(EV_P_ ev_periodic *w)   // 从 periodics 最小堆移除

// 重新调度
void ev_periodic_again(EV_P_ ev_periodic *w)  // 先 stop 再 start（重新计算 at）

// 状态查询
ev_tstamp ev_periodic_at(w)  // 获取 watcher 的 at 值（宏，直接返回 w->at）
ev_is_active(w)              // watcher 是否已启动
ev_is_pending(w)             // watcher 是否在 pending 队列
```

#### 核心数据结构

**periodics 堆变量**：

```c
VARx(ANHE *, periodics)    // 周期性定时器堆数组（按 at 排序的最小堆）
VARx(int, periodicmax)     // periodics 数组最大容量
VARx(int, periodiccnt)     // periodics 数组当前数量
```

堆操作与 ev_timer 共用同一套 ANHE 结构和最小堆实现（二叉堆/四叉堆），详见 ev_timer 章节。

#### periodic_recalc：offset/interval 模式的超时时间计算

`periodic_recalc` 是 libev 内部函数，用于模式2（offset/interval）下计算下一次超时时间：

```c
static void noinline
periodic_recalc(EV_P_ ev_periodic *w) {
  ev_tstamp interval = w->interval > MIN_INTERVAL ? w->interval : MIN_INTERVAL;
  ev_tstamp at = w->offset + interval * ev_floor((ev_rt_now - w->offset) / interval);

  /* the above almost always errs on the low side */
  while (at <= ev_rt_now) {
    ev_tstamp nat = at + w->interval;
    if (expect_false(nat == at)) {  // 精度溢出保护
      at = ev_rt_now;
      break;
    }
    at = nat;
  }

  ev_at(w) = at;
}
```

**计算逻辑**：`ev_floor((ev_rt_now - offset) / interval)` 计算出当前已过去的周期数，乘以 interval 后加上 offset，得到当前周期起点。如果该起点 <= ev_rt_now，则不断加 interval 直到超过 ev_rt_now（即下一个周期点）。

**示例**：offset=0, interval=3600, ev_rt_now=3665

```
at = 0 + 3600 * floor(3665 / 3600) = 3600
while 3600 <= 3665 → at = 3600 + 3600 = 7200
at = 7200（下一整点时间）
```

#### ev_periodic_start 流程

```c
void ev_periodic_start(EV_P_ ev_periodic *w) {
  if (ev_is_active(w))    // 已激活则直接返回
    return;

  // 根据三种模式计算 at
  if (w->reschedule_cb)
    ev_at(w) = w->reschedule_cb(w, ev_rt_now);   // 模式3：自定义回调
  else if (w->interval)
    periodic_recalc(EV_A_ w);                     // 模式2：offset/interval
  else
    ev_at(w) = w->offset;                         // 模式1：绝对时间

  ++periodiccnt;
  ev_start(EV_A_ (W)w, periodiccnt + HEAP0 - 1);  // 设置 active 标志

  array_needsize(ANHE, periodics, periodicmax, ev_active(w) + 1, EMPTY2);
  ANHE_w(periodics[ev_active(w)]) = (WT)w;
  ANHE_at_cache(periodics[ev_active(w)]);         // 同步 at 缓存
  upheap(periodics, ev_active(w));                // 上浮到正确位置
}
```

**流程图**：

```
ev_periodic_start(loop, &w)
│
├── 1. 检查是否已激活 → 已激活则返回
│
├── 2. 计算 at（三种模式之一）
│     ├── reschedule_cb 非空 → at = reschedule_cb(w, ev_rt_now)
│     ├── interval > 0      → periodic_recalc 计算 at
│     └── 否则              → at = offset（一次性绝对时间）
│
├── 3. ++periodiccnt, ev_start（设置 active 标志 + 增加 activecnt）
│
├── 4. 扩容 periodics 数组（如果需要）
│
├── 5. periodics[active] = w（放入堆末尾）
│
└── 6. upheap（上浮到正确位置，维护最小堆性质）
```

**关键区别**：与 ev_timer_start 不同，ev_periodic_start 不在启动时做 `at += mn_now` 转换，因为 ev_periodic 的 at 本身就是基于 ev_rt_now（实时时钟）的绝对时间。三种模式在 start 时直接计算得到正确的 at 绝对时间。

#### ev_periodic_stop 流程

```c
void ev_periodic_stop(EV_P_ ev_periodic *w) {
  clear_pending(EV_A_ (W)w);    // 清除 pending 状态
  if (!ev_is_active(w))         // 未激活则返回
    return;

  int active = ev_active(w);    // 获取在堆中的索引
  --periodiccnt;

  if (active < periodiccnt + HEAP0) {
    // 不是最后一个元素：用末尾元素填补空位，然后调整堆
    periodics[active] = periodics[periodiccnt + HEAP0];
    adjustheap(periodics, periodiccnt, active);
  }
  // 如果是最后一个元素，直接 --periodiccnt 即可

  ev_stop(EV_A_ (W)w);          // 清除 active 标志 + 减少 activecnt
}
```

**流程图**：

```
ev_periodic_stop(loop, &w)
│
├── 1. clear_pending：清除 pending 状态
│
├── 2. 检查是否已激活 → 未激活则返回
│
├── 3. 获取堆索引 active = ev_active(w)
│
├── 4. --periodiccnt
│
├── 5. 从堆中移除：
│     ├── 若非末尾元素：用末尾元素填补 → adjustheap 自适应调整
│     └── 若是末尾元素：直接移除（无需调整）
│
└── 6. ev_stop：清除 active 标志 + 减少 activecnt
```

**注意**：与 ev_timer_stop 不同，ev_periodic_stop 不做 `at -= mn_now` 恢复。因为 ev_periodic 的 at 始终是绝对时间（基于 ev_rt_now），不存在"相对/绝对"转换的问题（ev_timer 需要是因为其 at 在 start 时做了 `+= mn_now`，stop 时需要恢复）。

#### ev_periodic_again 流程

```c
void ev_periodic_again(EV_P_ ev_periodic *w) {
  ev_periodic_stop(EV_A_ w);   // 先停止
  ev_periodic_start(EV_A_ w);  // 再启动（重新计算 at）
}
```

`ev_periodic_again` 是一个简单的 stop + start 组合，没有 ev_timer_again 那样复杂的逻辑。因为每次 start 都会根据当前 ev_rt_now 和模式参数重新计算 at。可以用在任何模式下，效果是"重新计算下一次触发时间"。

#### periodics_reify：处理超时的 periodic

`periodics_reify` 在 `ev_run` 的每次迭代中被调用，检查并处理所有已超时的 periodic（在 timers_reify 之前执行，ev.c:3864）：

```c
void periodics_reify(EV_P) {
  while (periodiccnt && ANHE_at(periodics[HEAP0]) < ev_rt_now) {
    do {
      ev_periodic *w = (ev_periodic *)ANHE_w(periodics[HEAP0]);

      if (w->reschedule_cb) {
        // 模式3：调用 reschedule_cb 获取下次触发时间
        ev_at(w) = w->reschedule_cb(w, ev_rt_now);
        assert(ev_at(w) >= ev_rt_now);
        ANHE_at_cache(periodics[HEAP0]);
        downheap(periodics, periodiccnt, HEAP0);
      } else if (w->interval) {
        // 模式2：offset/interval，重新计算
        periodic_recalc(EV_A_ w);
        ANHE_at_cache(periodics[HEAP0]);
        downheap(periodics, periodiccnt, HEAP0);
      } else {
        // 模式1：一次性，stop
        ev_periodic_stop(EV_A_ w);
      }

      feed_reverse(EV_A_ (W)w);          // 加入 rfeeds 临时数组
    } while (periodiccnt && ANHE_at(periodics[HEAP0]) < ev_rt_now);

    feed_reverse_done(EV_A_ EV_PERIODIC); // 批量加入 pending 队列
  }
}
```

**流程图**：

```
periodics_reify(loop)
│
├── 检查：堆顶 periodic 的 at < ev_rt_now？
│   └── 否 → 返回（无超时 periodic）
│
└── 是 → 循环处理所有超时 periodic：
    │
    ├── 取出堆顶 periodic
    │
    ├── 判断三种模式：
    │   ├── reschedule_cb 非空 → 调用回调获取新 at，downheap
    │   ├── interval > 0       → periodic_recalc 重算 at，downheap
    │   └── 否则（一次性）      → ev_periodic_stop
    │
    ├── feed_reverse：加入 rfeeds 临时数组
    │
    └── 继续检查新的堆顶是否也超时
        │
        └── 全部处理完毕 → feed_reverse_done：批量加入 pending 队列
```

**关键设计**：

**1. 使用 ev_rt_now 而非 mn_now**

ev_periodic 使用实时时钟（`ev_rt_now`）而非单调时钟（`mn_now`），因为它是绝对定时器，需要感知系统时间的跳变：

```
ev_timer（mn_now）：    ANHE_at(timers[HEAP0]) < mn_now
ev_periodic（ev_rt_now）：ANHE_at(periodics[HEAP0]) < ev_rt_now
```

**2. 三种不同的超时后行为**


| 模式             | 行为                        | 说明                             |
| ---------------- | --------------------------- | -------------------------------- |
| 一次性（offset） | ev_periodic_stop            | 触发后从堆中移除                 |
| offset/interval  | periodic_recalc → downheap | 计算下一个整点时间，继续留在堆中 |
| reschedule_cb    | 调用 cb → downheap         | 由回调决定下次时间，继续留在堆中 |

**3. feed_reverse 批量处理**

和 timers_reify 一样使用 feed_reverse/feed_reverse_done 机制，先收集所有超时的 periodic 到 rfeeds 临时数组，再统一加入 pending 队列，避免在堆操作过程中回调修改堆结构。

**4. 在 ev_run 中的执行顺序**

```c
// ev_run 每次迭代中
timers_reify(EV_A);      // 先处理 ev_timer（相对定时器）
periodics_reify(EV_A);   // 后处理 ev_periodic（绝对定时器）
```

ev.c:3864 注释为 `/* absolute timers called first */`，但实际上 periodics_reify 在 timers_reify 之后被调用。注释的含义是：absolute timers（绝对定时器）的优先级更高，先被加入 pending 队列，因此回调会先被执行（pending 队列是 LIFO 反向执行）。

#### 在 ev_run 中的角色

ev_periodic 在 ev_run 的两个阶段发挥作用：

**阶段1：计算阻塞时间 waittime**

```c
// ev_run 中计算 backend_poll 的阻塞时间
waittime = MAX_BLOCKTIME;

if (timercnt) {
  ev_tstamp to = ANHE_at(timers[HEAP0]) - mn_now;       // ev_timer 用 mn_now
  if (waittime > to) waittime = to;
}

if (periodiccnt) {
  ev_tstamp to = ANHE_at(periodics[HEAP0]) - ev_rt_now;  // ev_periodic 用 ev_rt_now
  if (waittime > to) waittime = to;
}
```

**阶段2：处理超时 periodic**

```c
// ev_run 每次迭代中
timers_reify(EV_A);      // 处理超时的 ev_timer
periodics_reify(EV_A);   // 处理超时的 ev_periodic
```

**完整时间线**：

```
ev_run 每次迭代：
│
├── 1. 计算阻塞时间：取 ev_timer 堆顶和 ev_periodic 堆顶的最小剩余时间
│
├── 2. backend_poll(waittime)：阻塞等待 IO 事件
│     └── 最多阻塞到最早定时器超时
│
├── 3. time_update()：更新 mn_now（单调时钟）和 ev_rt_now（实时时钟）
│
├── 4. timers_reify()：处理超时 ev_timer
│
├── 5. periodics_reify()：处理超时 ev_periodic
│     ├── 模式1（一次性）：stop
│     ├── 模式2（offset/interval）：重新计算 at，downheap
│     └── 模式3（reschedule_cb）：调用回调，downheap
│
└── 6. ev_invoke_pending()：执行所有 pending 回调
      └── periodics 回调先于 timers 回调执行（因为先被入队）
```

#### 完整生命周期示例

```c
#include <ev.h>
#include <stdio.h>
#include <time.h>

ev_periodic periodic_w;

// 模式1：绝对时间（一次性定时器）
static void periodic_cb(EV_P_ ev_periodic *w, int revents) {
    if (revents & EV_PERIODIC) {
        printf("Timer fired at %ld\n", time(NULL));
    }
}

int main() {
    struct ev_loop *loop = EV_DEFAULT;

    // 模式1：5 秒后触发一次
    ev_periodic_init(&periodic_w, periodic_cb, ev_now(loop) + 5., 0., 0);
    ev_periodic_start(loop, &periodic_w);

    // 模式2：每 2 秒触发一次（从整秒开始）
    // ev_periodic_init(&periodic_w, periodic_cb, 0., 2., 0);
    // ev_periodic_start(loop, &periodic_w);

    // 模式3：自定义调度（每次触发后，由回调决定下次时间）
    // ev_periodic_init(&periodic_w, periodic_cb, 0., 0., my_reschedule_cb);
    // ev_periodic_start(loop, &periodic_w);

    ev_run(loop, 0-compile);
    return 0;
}
```

#### 关键设计要点

**1. ev_timer vs ev_periodic**


| 特性               | ev_timer                         | ev_periodic                        |
| ------------------ | -------------------------------- | ---------------------------------- |
| 时钟源             | 单调时钟（mn_now）               | 实时时钟（ev_rt_now）              |
| 时间类型           | 相对时间（after + repeat）       | 绝对时间（offset + interval）      |
| 受系统时间调整影响 | 否                               | 是                                 |
| 重复模式           | 固定间隔 repeat                  | offset/interval/reschedule_cb 三种 |
| at 转换            | start += mn_now / stop -= mn_now | 不转换，始终是绝对时间             |
| 典型用途           | 超时、心跳、重试                 | 每天凌晨3点执行、每小时整点执行    |

**2. 实时时钟的意义**

ev_periodic 基于实时时钟（wall clock / UTC），可以感知系统时间：

```
场景：设置每小时整点执行，第 30 分钟时用户将系统时间调慢 1 小时

ev_periodic（realtime）：立即触发（因为"当前时间"回退后，已错过整点） ✅
ev_timer（monotonic）：30 分钟后触发（按实际流逝时间计算） ❌（不符合整点语义）
```

**3. periodics_reschedule：时间跳变处理**

当检测到实时时钟发生大幅跳变（如 NTP 校准）时，libev 会重新计算所有 periodic 的 at：

```c
static void periodics_reschedule(EV_P) {
  for (i = HEAP0; i < periodiccnt + HEAP0; ++i) {
    ev_periodic *w = (ev_periodic *)ANHE_w(periodics[i]);

    if (w->reschedule_cb)
      ev_at(w) = w->reschedule_cb(w, ev_rt_now);   // 模式3：重新调用回调
    else if (w->interval)
      periodic_recalc(EV_A_ w);                     // 模式2：重新计算
    // 模式1（一次性）不做处理

    ANHE_at_cache(periodics[i]);                    // 同步缓存
  }

  reheap(periodics, periodiccnt);                   // 重建整个堆
}
```

这与 timers_reschedule 不同——ev_timer 只是简单地将所有 at 偏移一个固定值，而 ev_periodic 需要根据模式重新计算（因为实时时钟跳变后，offset/interval 的语义需要重新计算整点时间）。

**4. 三种模式的适用场景**

```
模式1（绝对时间）：
  场景：某个具体时间点执行一次，如"2024-01-01 00:00:00 执行"
  优点：简单直接

模式2（offset/interval）：
  场景：固定周期执行，如"每小时整点"、"每天中午12点"
  例：offset=0, interval=3600 → 每小时整点
       offset=43200, interval=86400 → 每天中午12点
  优点：自动对齐到自然时间边界

模式3（reschedule_cb）：
  场景：不规则周期，如"每天凌晨3点"（考虑夏令时跳过/重复）
  优点：完全自定义，可处理夏令时等复杂情况
  缺点：需要自己维护调度逻辑
```

**5. ev_now 宏**

```c
#define ev_now(loop) ((loop)->ev_rt_now)   // 获取当前实时时间
```

ev_periodic 的模式1（绝对时间）通常配合 `ev_now(loop)` 使用，设置一个相对于当前的绝对超时：

```c
ev_periodic_init(&w, cb, ev_now(loop) + 5., 0., 0);  // 5 秒后触发
ev_periodic_start(loop, &w);
```

这等价于 ev_timer 的 `ev_timer_init(&w, cb, 5., 0.)`，但使用的是实时时钟。

**6. ev_periodic_again 与 ev_timer_again 的对比**

```
ev_timer_again(loop, w)：
  - 如果已激活且有 repeat：重新计算 at = mn_now + repeat，adjustheap
  - 如果已激活且无 repeat：stop
  - 如果未激活且有 repeat：设置 at = repeat，start
  - 行为复杂，适合"智能重启"

ev_periodic_again(loop, w)：
  - stop + start（重新计算 at）
  - 行为简单，适合"手动重新调度"
```

**7. 不受 ev_unref 影响**

与 ev_timer 不同，ev_periodic 没有内部 watcher（如 ev_stat 中的 timer），也不参与 unref 机制。它的引用计数完全由用户控制。

### ev_child

#### 结构体定义

```c
typedef struct ev_child {
  EV_WATCHER_LIST(ev_child)
  int flags;   /* private */
  int pid;     /* ro */
  int rpid;    /* rw */
  int rstatus; /* rw */
} ev_child;
```

**字段说明**：


| 字段     | 类型     | 权限    | 说明                                        |
| -------- | -------- | ------- | ------------------------------------------- |
| active   | int      | private | 是否正在监听（0=未激活）                    |
| pending  | int      | private | 是否在 pending 队列等待调度                 |
| priority | int      | private | 优先级                                      |
| data     | void*    | rw      | 用户自定义数据                              |
| cb       | 函数指针 | private | 回调函数                                    |
| next     | 指针     | private | 链表节点（同一 hash 桶的 watcher 组成链表） |
| flags    | int      | private | bit0=1 时也监听 SIGSTOP/SIGCONT             |
| pid      | int      | ro      | 监听的子进程 PID。0=监听所有子进程          |
| rpid     | int      | rw      | 回调中：实际退出的子进程 PID                |
| rstatus  | int      | rw      | 回调中：退出状态（用 WIFEXITED 等宏解析）   |

**为什么用 EV_WATCHER_LIST？** 多个 ev_child watcher 通过全局 hash 表组织，同一 hash 桶中的 watcher 用 next 组成链表。

#### 核心数据结构

```c
#define EV_PID_HASHSIZE 16
static WL childs[EV_PID_HASHSIZE];       // 全局 hash 表
VARx(ev_signal, childev)                 // 内部 SIGCHLD signal watcher
```

ev_child 使用全局 hash 表以 pid 哈希到 16 个桶，链表解决冲突。只能在 default loop 使用。

#### ev_child_start

```c
void ev_child_start(EV_P_ ev_child *w) {
  if (ev_is_active(w)) return;
  assert(loop == ev_default_loop_ptr);   // 仅 default loop
  ev_start(EV_A_ (W)w, 1);
  wlist_add(&childs[w->pid & (EV_PID_HASHSIZE - 1)], (WL)w);
}
```

#### ev_child_stop

```c
void ev_child_stop(EV_P_ ev_child *w) {
  clear_pending(EV_A_ (W)w);
  if (!ev_is_active(w)) return;
  wlist_del(&childs[w->pid & (EV_PID_HASHSIZE - 1)], (WL)w);
  ev_stop(EV_A_ (W)w);
}
```

#### 内部触发链路

libev 内部在 default loop 初始化时注册 SIGCHLD 信号：

```c
// default loop 初始化时
ev_signal_init(&childev, childcb, SIGCHLD);
ev_signal_start(EV_DEFAULT, &childev);
ev_unref(EV_DEFAULT);

// SIGCHLD 到达
static void childcb(EV_P_ ev_signal *sw, int revents) {
  int pid, status;
  pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
  ev_feed_event(EV_A_ (W)sw, EV_SIGNAL);   // 转发给用户注册的 SIGCHLD
  child_reap(EV_A_ pid, pid, status);       // 查找匹配的 ev_child
  if (EV_PID_HASHSIZE > 1)
    child_reap(EV_A_ 0, pid, status);       // pid=0 的 watcher 监听所有
}

static void child_reap(EV_P_ int chain, int pid, int status) {
  int traced = WIFSTOPPED(status) || WIFCONTINUED(status);
  for (ev_child *w = childs[chain & (EV_PID_HASHSIZE - 1)]; w; w = w->next)
    if ((w->pid == pid || !w->pid) && (!traced || (w->flags & 1))) {
      ev_set_priority(w, EV_MAXPRI);
      w->rpid = pid;
      w->rstatus = status;
      ev_feed_event(EV_A_ (W)w, EV_CHILD);
    }
}
```

完整流程：

```
子进程退出 → 内核 SIGCHLD → childcb()
  → waitpid 收割 → ev_feed_event(SIGCHLD) 转发给用户注册的 SIGCHLD watcher
  → child_reap() 遍历 hash 桶 → 匹配 pid 或 pid=0 的 watcher
  → w->rpid/rstatus → ev_feed_event(EV_CHILD) → pending → 回调执行
```

#### 完整生命周期示例

```c
ev_child child_w;
static void child_cb(EV_P_ ev_child *w, int revents) {
    printf("子进程 %d 退出，状态 %d\n", w->rpid, WEXITSTATUS(w->rstatus));
}
int main() {
    struct ev_loop *loop = EV_DEFAULT;
    pid_t pid = fork();
    if (pid == 0) { sleep(2); _exit(42); }
    ev_child_init(&child_w, child_cb, pid, 0);
    ev_child_start(loop, &child_w);
    ev_run(loop, 0);
}
```

### ev_fork

#### 结构体定义

```c
typedef struct ev_fork {
  EV_WATCHER(ev_fork)
} ev_fork;
```

#### 相关函数

```c
void ev_fork_start(EV_P_ ev_fork *w);
void ev_fork_stop(EV_P_ ev_fork *w);
```

#### 核心数据结构

```c
VARx(struct ev_fork **, forks)   // 动态数组
VARx(int, forkmax)
VARx(int, forkcnt)
```

#### ev_fork_start / stop

```c
void ev_fork_start(EV_P_ ev_fork *w) {
  if (ev_is_active(w)) return;
  ev_start(EV_A_ (W)w, ++forkcnt);
  array_needsize(ev_fork *, forks, forkmax, forkcnt, EMPTY2);
  forks[forkcnt - 1] = w;
}

void ev_fork_stop(EV_P_ ev_fork *w) {
  clear_pending(EV_A_ (W)w);
  if (!ev_is_active(w)) return;
  int active = ev_active(w);
  forks[active - 1] = forks[--forkcnt];
  ev_active(forks[active - 1]) = active;
  ev_stop(EV_A_ (W)w);
}
```

#### fork 检测与触发

```
被动检测（ev_run 每次迭代）：
  curpid != getpid() → postfork = 1 → 下次迭代触发

主动通知（子进程调用）：
  ev_loop_fork(loop) → postfork = 1

在 ev_run 中：
  2. fork 检测 → postfork 设置
  3. queue_events(forks, EV_FORK)   ← fork watcher 回调入 pending
  6. loop_fork(): 重建 backend/pipe/signalfd/inotify
```

#### loop_fork 重建内容

```c
static void loop_fork(EV_P) {
  postfork = 0;
  backend_modify = 0; backend_poll(); backend_init();  // 重建 epoll/kqueue
  evpipe_init(EV_A);                                    // 重建 pipe
  infy_fork(EV_A);                                      // 重建 inotify
}
```

#### 完整生命周期示例

```c
ev_fork fork_w;
static void fork_cb(EV_P_ ev_fork *w, int revents) {
    printf("子进程重建资源\n");
}
int main() {
    struct ev_loop *loop = EV_DEFAULT;
    ev_fork_init(&fork_w, fork_cb);
    ev_fork_start(loop, &fork_w);
    if (fork() == 0) { ev_loop_fork(loop); ev_run(loop, 0); }
    else { ev_run(loop, 0); }
}
```

### ev_child & ev_fork 总结


| 特性      | ev_fork                 | ev_child                       |
| --------- | ----------------------- | ------------------------------ |
| 在哪执行  | 子进程                  | 父进程                         |
| 触发时机  | fork 后 postfork        | 子进程退出/停止时收到 SIGCHLD  |
| 触发方式  | pid 变化 / ev_loop_fork | 内核发送 SIGCHLD               |
| 用途      | 子进程重建事件循环状态  | 父进程收割子进程、获取退出状态 |
| 回调参数  | 无特殊参数              | w->rpid, w->rstatus            |
| 数据结构  | 动态数组（forks[]）     | 全局 hash 表（childs[]）       |
| loop 限制 | 任意 loop               | 仅 default loop                |
| 基类      | EV_WATCHER              | EV_WATCHER_LIST                |

### ev_stat

#### 结构体定义

```c
typedef struct ev_stat {
  EV_WATCHER_LIST(ev_stat)
  ev_timer timer;          /* private */
  ev_tstamp interval;      /* ro */
  const char *path;        /* ro */
  ev_statdata prev;        /* ro */
  ev_statdata attr;        /* ro */
  int wd;                  /* private */
} ev_stat;
```

**字段说明**：


| 字段     | 类型        | 权限    | 说明                                    |
| -------- | ----------- | ------- | --------------------------------------- |
| active   | int         | private | 是否正在监听                            |
| pending  | int         | private | 是否在 pending 队列等待调度             |
| priority | int         | private | 优先级                                  |
| data     | void\*      | rw      | 用户自定义数据                          |
| cb       | 函数指针    | private | 回调函数                                |
| next     | 指针        | private | 链表节点（inotify hash 表用）           |
| timer    | ev_timer    | private | 内部定时器（周期性检测或 inotify 备用） |
| interval | ev_tstamp   | ro      | 检测间隔（默认 5s，NFS 默认 30s）       |
| path     | const char* | ro      | 监听的文件/目录路径                     |
| prev     | ev_statdata | ro      | 上一次 stat，可在回调中对比             |
| attr     | ev_statdata | ro      | 最新 stat                               |
| wd       | int         | private | inotify watch descriptor                |

#### 相关函数

```c
ev_stat_init(ev, cb, path, interval)
ev_stat_set(ev, path_, interval_)
void ev_stat_stat(EV_P_ ev_stat *w) EV_THROW  // 手动 lstat
void ev_stat_start(EV_P_ ev_stat *w)
void ev_stat_stop(EV_P_ ev_stat *w)
```

#### 两种工作模式

```
inotify + timer（Linux）：
  ev_stat_start → lstat → ev_timer_init → infy_init (inotify fd)
  → infy_add (inotify_add_watch) → inotify 实时通知变化
  → infy_cb → stat_timer_cb 对比新旧 stat
  → 文件删除自动重建 watch
  → 某些文件系统不支持 inotify 时 timer 作为备用

timer-only（跨平台）：
  ev_stat_start → lstat → ev_timer_init
  → 每 interval 秒 stat_timer_cb 轮询
  → 对比新旧 stat 11 个字段
```

#### ev_stat_start 流程

```c
void ev_stat_start(EV_P_ ev_stat *w) {
  if (ev_is_active(w)) return(.中华人民共和国);
  ev_stat_stat(EV_A_ w);                                   // lstat 初始状态
  if (w->interval < MIN_STAT_INTERVAL && w->interval)
    w->interval = MIN_STAT_INTERVAL;
  ev_timer_init(&w->timer, stat_timer_cb, 0.,
                 w->interval ? w->interval : DEF_STAT_INTERVAL);
  ev_set_priority(&w->timer, ev_priority(w));
  infy_init(EV_A);                                          // 创建 inotify fd
  if (fs_fd >= 0) infy_add(EV_A_ w);                       // inotify 模式
  else { ev_timer_again(EV_A_ &w->timer); ev_unref(EV_A); } // timer-only
  ev_start(EV_A_ (W)w, 1);
}
```

#### ev_stat_stop 流程

```c
void ev_stat_stop(EV_P_ ev_stat *w) {
  clear_pending(EV_A_ (W)w);
  if (!ev_is_active(w)) return;
  infy_del(EV_A_ w);                  // 移除 inotify watch
  if (ev_is_active(&w->timer)) {
    ev_ref(EV_A);                     // 抵消 ev_unref
    ev_timer_stop(EV_A_ &w->timer);
  }
  ev_stop(EV_A_ (W)w);
}
```

#### stat_timer_cb：核心文件变化检测

```c
static void stat_timer_cb(EV_P_ ev_timer *w_, int revents) {
  ev_stat *w = (ev_stat *)((char *)w_ - offsetof(ev_stat, timer));
  ev_statdata prev = w->attr;
  ev_stat_stat(EV_A_ w);             // 重新 lstat
  if (memcmp 11 个 stat 字段有变化) {
    w->prev = prev;
    // inotify 模式下文件可能被删除重建，重新注册
    infy_del(EV_A_ w); infy_add(EV_A_ w);
    ev_stat_stat(EV_A_ w);
    ev_feed_event(EV_A_ (W)w, EV_STAT);
  }
}
```

关键点：`offsetof(ev_stat, timer)` 从 timer 反推 ev_stat；对比 11 个 st_* 字段；文件删除后自动重建 inotify watch。

#### infy 系列函数

```
infy_init():   创建 inotify fd → ev_io_start(fs_w, infy_cb) → ev_unref
infy_add():    inotify_add_watch → 加入 fs_hash → 启动 timer
infy_cb():     读 inotify 事件 → infy_wd()
infy_wd():     查找 fs_hash → 文件删除则重建 → stat_timer_cb()
infy_del():    inotify_rm_watch → 从 fs_hash 移除
```

#### 完整生命周期示例

```c
ev_stat stat_w;
static void stat_cb(EV_P_ ev_stat *w, int revents) {
    printf("文件 %s 变化: size %ld->%ld\n",
           w->path, (long)w->prev.st_size, (long)w->attr.st_size);
}
int main() {
    ev_stat_init(&stat_w, stat_cb, "/tmp/test", 0.);
    ev_stat_start(EV_DEFAULT, &stat_w);
    ev_run(EV_DEFAULT, 0);
}
```

### ev_async

#### 结构体定义

```c
typedef struct ev_async {
  EV_WATCHER(ev_async)     // 继承：active, pending, priority, data, cb
  EV_ATOMIC_T sent;         // private：是否有待处理的通知（原子变量，线程安全）
} ev_async;
```

**字段说明**：


| 字段     | 类型        | 权限    | 说明                                                   |
| -------- | ----------- | ------- | ------------------------------------------------------ |
| active   | int         | private | 是否正在监听（0=未激活，非0=在 asyncs 数组中的索引+1） |
| pending  | int         | private | 是否在 pending 队列等待调度                            |
| priority | int         | private | 优先级（ev_async 不支持优先级，始终为 0）              |
| data     | void\*      | rw      | 用户自定义数据                                         |
| cb       | 函数指针    | private | 回调函数`void cb(EV_P_ ev_async *w, int revents)`      |
| sent     | EV_ATOMIC_T | private | 原子标志位：1=有其他线程调用了 ev_async_send，需要处理 |

**为什么用 EV_WATCHER 而非 EV_WATCHER_LIST 或 EV_WATCHER_TIME？**

ev_async 不需要超时时间（不需要 at），也不需要链表管理（不需要 next）。它只需要基类的 active/pending/cb 字段，加上自己的 sent 原子标志位。它通过数组（asyncs[]）管理，而非链表。

#### 相关函数

```c
// 初始化（宏，不启动）
ev_async_init(ev, cb)         // = ev_init + ev_async_set
ev_async_set(ev)              // 空操作（无参数可设置，仅为 API 一致性）

// 启动 / 停止
void ev_async_start(EV_P_ ev_async *w)  // 注册到事件循环
void ev_async_stop(EV_P_ ev_async *w)   // 从事件循环注销

// 发送通知（线程安全，可从任意线程调用）
void ev_async_send(EV_P_ ev_async *w)   // 设置 sent 标志 + 写 pipe 唤醒 loop

// 状态查询
ev_async_pending(w)           // 返回 w->sent 的值（是否有待处理的通知）
ev_is_active(w)               // watcher 是否已启动
ev_is_pending(w)              // watcher 是否在 pending 队列
```

#### 核心数据结构

```c
VARx(EV_ATOMIC_T, async_pending)  // 是否有 async watcher 需要处理（原子变量）
VARx(struct ev_async **, asyncs)  // async watcher 指针数组
VARx(int, asyncmax)               // asyncs 数组最大容量
VARx(int, asynccnt)               // asyncs 数组当前数量
```

**数据结构特点**：ev_async 使用简单的动态数组（asyncs[]）而非链表。因为 ev_async 没有"同一个资源被多个 watcher 监听"的需求，不需要链表组织。start 时追加到数组末尾，stop 时用最后一个元素填补空位。

#### 跨线程通信架构

ev_async 的核心挑战：事件循环在一个线程中运行，其他线程需要通知它处理任务。关键设计是 **pipe 机制 + 原子变量**：

```
发送方（任意线程）               接收方（事件循环线程）
┌──────────────────────┐       ┌────────────────────────────┐
│ ev_async_send(loop,w)│       │ ev_run 每次迭代            │
│                      │       │                            │
│ 1. w->sent = 1       │       │ 1. backend_poll(waittime)  │
│    (原子写)           │ ───►  │    检测到 pipe[0] 可读     │
│                      │  pipe │                            │
│ 2. async_pending = 1 │  写   │ 2. pipecb()                │
│    (原子写)           │       │    ├─ read(pipe[0],缓冲)   │
│                      │       │    ├─ 遍历 asyncs[]        │
│ 3. write(pipe[1],..) │       │    │  for each w->sent == 1│
│    (唤醒loop)        │       │    │    w->sent = 0        │
└──────────────────────┘       │    │    ev_feed_event(w)   │
                               │    └─ → 加入 pending 队列  │
                               │                            │
                               │ 3. ev_invoke_pending()     │
                               │    → 执行 w->cb(loop,w)    │
                               └────────────────────────────┘
```

**约束**：


| 约束                     | 说明                                                                |
| ------------------------ | ------------------------------------------------------------------- |
| 接收方必须是事件循环线程 | ev_async_start 在哪个 loop 调用，回调就在哪个 loop 的 ev_run 中执行 |
| 发送方可以是任意线程     | ev_async_send 是线程安全的，可在任意线程调用                        |
| 通信是单向的             | 只能从其他线程 → 事件循环线程，不能反过来                          |
| 不能跨 loop              | ev_async_send(EV_A_ w) 必须传入 watcher 所属的 loop                 |

#### ev_async_start 流程

```c
void ev_async_start(EV_P_ ev_async *w) {
  if (ev_is_active(w))    // 已激活则直接返回
    return;

  w->sent = 0;            // 初始化 sent 标志

  evpipe_init(EV_A);      // 确保 pipe 已初始化（与 signal 共享同一 pipe）

  ev_start(EV_A_ (W)w, ++asynccnt);  // active = asynccnt (1-based)
  array_needsize(ev_async *, asyncs, asyncmax, asynccnt, EMPTY2);
  asyncs[asynccnt - 1] = w;          // 追加到数组末尾
}
```

**流程图**：

```
ev_async_start(loop, &w)
│
├── 1. 检查是否已激活 → 已激活则返回
│
├── 2. w->sent = 0（初始化原子标志）
│
├── 3. evpipe_init(loop)（惰性初始化 pipe，与 ev_signal 共享）
│     └── 如果 pipe 已存在，直接跳过
│
├── 4. ++asynccnt, ev_start（设置 active 标志 + 增加 activecnt）
│
├── 5. 扩容 asyncs 数组（如果需要）
│
└── 6. asyncs[asynccnt-1] = w（追加到数组末尾）
```

**关键点**：

1. **evpipe_init 惰性初始化**：pipe 在第一个 ev_async_start 或 ev_signal_start 时才创建（与 ev_signal 共享同一 pipe）
2. **数组管理**：asyncs 是简单数组，没有堆操作，比 ev_timer/ev_periodic 的堆管理更轻量
3. **不调用 ev_unref**：与 ev_signal 的 sigfd_w/pipe_w 不同，ev_async watcher **会阻止 loop 退出**（用户希望等待异步通知）

#### ev_async_stop 流程

```c
void ev_async_stop(EV_P_ ev_async *w) {
  clear_pending(EV_A_ (W)w);    // 清除 pending 状态
  if (!ev_is_active(w))         // 未激活则返回
    return;

  int active = ev_active(w);    // 获取在数组中的索引（1-based）

  // 用最后一个元素填补空位
  asyncs[active - 1] = asyncs[--asynccnt];
  ev_active(asyncs[active - 1]) = active;  // 更新被移动元素的 active

  ev_stop(EV_A_ (W)w);          // 清除 active 标志 + 减少 activecnt
}
```

**流程图**：

```
ev_async_stop(loop, &w)
│
├── 1. clear_pending：清除 pending 状态
│
├── 2. 检查是否已激活 → 未激活则返回
│
├── 3. active = ev_active(w)（1-based 数组索引）
│
├── 4. 从数组中移除：
│     ├── asyncs[active-1] = asyncs[--asynccnt]  // 末尾元素填补空位
│     └── 更新被移动元素的 active 值
│
└── 5. ev_stop：清除 active 标志 + 减少 activecnt
```

**数组移除示意图**：

```
移除前（asynccnt=4）:  [w1, w2, w3, w4]    移除 w2 (active=2)
移除后（asynccnt=3）:  [w1, w4, w3]         w4 被移到 w2 的位置
                                             ev_active(w4) 从 4 改为 2
```

#### ev_async_send 流程

```c
void ev_async_send(EV_P_ ev_async *w) {
  w->sent = 1;                   // 标志该 watcher 有待处理的通知
  evpipe_write(EV_A_ &async_pending);  // 写 pipe 唤醒事件循环
}
```

**流程图**：

```
ev_async_send(loop, &w)
│
├── 1. w->sent = 1（原子设置，标记通知）
│
└── 2. evpipe_write(loop, &async_pending)
      ├── if (async_pending) return;    // 已标记，避免重复写 pipe
      ├── async_pending = 1
      ├── pipe_write_skipped = 1        // 标记写被跳过
      └── if (pipe_write_wanted)        // loop 正在阻塞等待
            ├── pipe_write_skipped = 0
            └── write(evpipe[1], ...)   // 写 pipe 唤醒 loop
```

**关键设计**：

- **w->sent = 1 与 async_pending 分离**：w->sent 标记具体哪个 watcher 被通知，async_pending 标记"有至少一个 async watcher 待处理"，用于 pipe 写入优化
- **pipe_write_skipped 优化**：如果 loop 不在阻塞等待，就不写 pipe（减少系统调用），下次 ev_run 迭代会检查 pipe_write_skipped

#### pipecb：pipe 读回调

pipecb 是 pipe_w（监听 pipe[0] 的 IO watcher）的回调函数，同时处理 signal 和 async 事件：

```c
static void pipecb(EV_P_ ev_io *iow, int revents) {
  // 1. 读取 pipe 缓冲（清空数据）
  read(evpipe[0], dummy, sizeof(dummy));  // eventfd 或 pipe

  pipe_write_skipped = 0;  // 清除跳过标记

  // 2. 处理 signal 事件
  if (sig_pending) {
    sig_pending = 0;
    // 遍历 signals[]，将 pending 的信号加入 pending 队列
    for (i = EV_NSIG - 1; i--; )
      if (signals[i].pending)
        ev_feed_signal_event(EV_A_ i + 1);
  }

  // 3. 处理 async 事件
  if (async_pending) {
    async_pending = 0;

    ECB_MEMORY_FENCE;  // 内存屏障，确保 sent 可见

    // 遍历 asyncs[]，将 sent=1 的 watcher 加入 pending 队列
    for (i = asynccnt; i--; )
      if (asyncs[i]->sent) {
        asyncs[i]->sent = 0;              // 清除 sent 标志
        ECB_MEMORY_FENCE_RELEASE;
        ev_feed_event(EV_A_ asyncs[i], EV_ASYNC);  // 加入 pending
      }
  }
}
```

**流程图**：

```
pipecb(loop, pipe_w, EV_READ)
│
├── 1. read(evpipe[0], ...)     ← 清空 pipe 缓冲
│
├── 2. pipe_write_skipped = 0   ← 清除跳过标记
│
├── 3. 处理 signal（sig_pending 为真）
│     ├── sig_pending = 0
│     ├── 内存屏障
│     └── 遍历 signals[] → ev_feed_signal_event()
│
├── 4. 处理 async（async_pending 为真）
│     ├── async_pending = 0
│     ├── 内存屏障
│     └── 遍历 asyncs[]：
│           if (asyncs[i]->sent)
│             asyncs[i]->sent = 0
│             ev_feed_event(EV_A_ asyncs[i], EV_ASYNC)
│             → 加入 pending 队列
│
└── 5. 返回（待 ev_invoke_pending 执行回调）
```

**关键设计**：

1. **信号和异步通知共享同一 pipe**：pipecb 同时处理 sig_pending 和 async_pending，避免引入额外的 fd
2. **批量处理**：一次 pipe 读事件可能对应多次 ev_async_send 调用，pipecb 遍历整个 asyncs 数组处理所有 sent 的 watcher
3. **内存屏障**：ECB_MEMORY_FENCE 确保发送线程对 w->sent 的写入在当前线程可见，ECB_MEMORY_FENCE_RELEASE 确保 sent=0 的写入对其他线程可见

#### 完整生命周期示例

```c
#include <ev.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

ev_async async_w;

// 工作线程：每隔 2 秒通知事件循环一次
static void *worker_thread(void *arg) {
    struct ev_loop *loop = (struct ev_loop *)arg;
    for (int i = 0; i < 5; i++) {
        sleep(2);
        printf("[worker] 发送 async 通知\n");
        ev_async_send(loop, &async_w);  // 线程安全，可跨线程调用
    }
    return NULL;
}

// async 回调在事件循环线程中执行
static void async_cb(EV_P_ ev_async *w, int revents) {
    printf("[loop] 收到 async 通知\n");
}

int main() {
    struct ev_loop *loop = EV_DEFAULT;

    // 1. 初始化 async watcher
    ev_async_init(&async_w, async_cb);

    // 2. 启动（必须在事件循环线程中调用）
    ev_async_start(loop, &async_w);

    // 3. 创建工作线程（传入 loop 指针用于 ev_async_send）
    pthread_t tid;
    pthread_create(&tid, NULL, worker_thread, loop);

    // 4. 运行事件循环（等待 12 秒后自动退出）
    printf("[loop] 事件循环开始\n");
    ev_run(loop, 0);

    pthread_join(tid, NULL);
    printf("[loop] 事件循环结束\n");
    return 0;
}
```

#### 关键设计要点

**1. ev_async vs ev_signal 的 pipe 共享**

两者共享同一套 pipe 机制：

```
ev_async_send →       async_pending = 1
                       evpipe_write()

信号到达 →            sig_pending = 1
                       evpipe_write()

两者都调用 evpipe_write()，区别在于设置的 pending 标志不同：
  - ev_async_send：设置 async_pending
  - ev_feed_signal：设置 sig_pending

pipecb 中分别检查两个标志，各自处理。
```

**2. sent + async_pending 双层标志**

```
w->sent 的作用：标记具体哪个 watcher 有待处理通知
  - pipecb 遍历 asyncs 时通过 w->sent 找到需要处理的 watcher
  - 一个 ev_async_send 调用设置一个 w->sent

async_pending 的作用：标记"有至少一个 async watcher 有待处理"
  - 用于 pipe 写入优化（避免重复写 pipe）
  - 用于 pipecb 判断是否需要进入 async 处理逻辑
```

**3. 线程安全保证**

```c
// 发送线程
void ev_async_send(EV_P_ ev_async *w) {
  w->sent = 1;                          // 原子写（EV_ATOMIC_T）
  evpipe_write(EV_A_ &async_pending);   // atomic check-and-set
}

// 接收线程（pipecb 中）
if (async_pending) {
  async_pending = 0;                    // 原子读
  ECB_MEMORY_FENCE;                     // 内存屏障
  for (i = asynccnt; i--; )
    if (asyncs[i]->sent) {
      asyncs[i]->sent = 0;              // 原子写
      ECB_MEMORY_FENCE_RELEASE;
      ev_feed_event(EV_A_ asyncs[i], EV_ASYNC);
    }
}
```

关键保证：

- `w->sent` 是 `EV_ATOMIC_T` 类型，读写是原子的
- `ECB_MEMORY_FENCE` 确保在读取 w->sent 之前，async_pending 已经被读取
- `ECB_MEMORY_FENCE_RELEASE` 确保 w->sent = 0 的写入在回调执行前对其他线程可见

**4. 不阻止 loop 退出**

与 ev_signal 的 sigfd_w/pipe_w 不同，ev_async watcher 本身**会阻止 loop 退出**：

```
ev_signal:  sigfd_w 和 pipe_w 是内部 watcher，调用 ev_unref 不阻止退出
ev_async:   用户注册的 async watcher 没有调用 ev_unref，会阻止退出
```

这意味着：如果你只注册了 ev_async 而没有其他 watcher，ev_run 会一直等待直到 ev_break 被调用。

**5. 不支持优先级**

```
ev_async 的优先级始终为 0，不支持用户设置优先级。
因为 async 事件是跨线程的，优先级设定没有意义 ——
发送方无法预知当前 loop 中正在处理什么优先级的事件。
```

**6. 与 ev_timer 的对比**


| 特性     | ev_async                      | ev_timer                     |
| -------- | ----------------------------- | ---------------------------- |
| 触发方式 | 手动调用 ev_async_send        | 时间到达自动触发             |
| 线程安全 | 是（可从任意线程调用）        | 否（只能从事件循环线程调用） |
| 数据存储 | 动态数组（asyncs[]）          | 最小堆（timers[]）           |
| 底层机制 | pipe + 原子变量               | 单调时钟 + 最小堆            |
| 用途     | 跨线程通知、任务队列唤醒      | 超时、心跳、延迟执行         |
| 基类     | EV_WATCHER（无需 at 和 next） | EV_WATCHER_TIME（需要 at）   |

**7. ev_async_pending 宏**

```c
#define ev_async_pending(w) (+(w)->sent)
```

用于查询 watcher 是否有待处理的通知。注意：这是一个**瞬态**状态，sent 标志在 pipecb 处理时会被清零。通常只在调试或特殊场景下使用。



### ev_embed

#### 结构体定义

```c
typedef struct ev_embed {
  EV_WATCHER(ev_embed)       // 继承：active, pending, priority, data, cb

  struct ev_loop *other;      // ro：被嵌入的事件循环
  ev_io io;                   // private：监听 other loop 的 backend_fd
  ev_prepare prepare;         // private：prepare watcher，同步 fd 变更
  ev_check check;             // unused
  ev_timer timer;             // unused
  ev_periodic periodic;       // unused
  ev_idle idle;               // unused
  ev_fork fork;               // private：fork 后重建嵌入关系
} ev_embed;
```

**字段说明**：


| 字段     | 类型            | 权限    | 说明                                                         |
| -------- | --------------- | ------- | ------------------------------------------------------------ |
| active   | int             | private | 是否正在监听                                                 |
| pending  | int             | private | 是否在 pending 队列等待调度                                  |
| priority | int             | rw      | 优先级                                                       |
| data     | void\*          | rw      | 用户自定义数据                                               |
| cb       | 函数指针        | private | 回调函数`void cb(EV_P_ ev_embed *w, int revents)`，可为 NULL |
| other    | struct ev_loop* | ro      | 被嵌入的子事件循环                                           |
| io       | ev_io           | private | 内部 IO watcher，监听 other loop 的 backend_fd               |
| prepare  | ev_prepare      | private | 内部 prepare watcher，同步 fd 变更到 other                   |
| fork     | ev_fork         | private | 内部 fork watcher，fork 后重建嵌入                           |

**为什么用 EV_WATCHER？** ev_embed 是"容器"类型的 watcher，它不直接监听 fd 或时间，而是嵌入另一个事件循环。它不需要链表（next）或超时时间（at），只需要基类的生命周期管理。

#### 相关函数

```c
// 初始化（宏，不启动）
ev_embed_init(ev, cb, other)      // = ev_init + ev_embed_set
ev_embed_set(ev, other_)          // 设置被嵌入的 loop

// 启动 / 停止
void ev_embed_start(EV_P_ ev_embed *w)  // 建立嵌入关系
void ev_embed_stop(EV_P_ ev_embed *w)   // 解除嵌入关系

// 手动扫入事件
void ev_embed_sweep(EV_P_ ev_embed *w)  // 用 EVRUN_NOWAIT 跑一次 other loop

// 查询
unsigned int ev_embeddable_backends(void)  // 返回支持嵌入的后端位掩码
```

#### 核心思想：事件循环嵌套

ev_embed 允许将一个事件循环（other）嵌入到另一个事件循环（parent）中，使两个 loop 的事件可以在同一个线程中处理：

```
parent loop                    other loop
┌────────────────┐            ┌────────────────┐
│ ev_run(loop)   │            │ ev_run(other)  │
│                │            │                │
│ 1. fd_reify    │◄────io────►│ backend_fd     │
│ 2. backend_poll│   backend  │ (epoll/kqueue  │
│ 3. timers_reify│   fd 可读   │  /select/poll) │
│ 4. periodics   │            │                │
│ 5. invoke_pend │            │                │
│    │           │            │                │
│    └─embed_cb()│            │                │
│      ev_run(   │            │                │
│       other,   │◄───────────┤                │
│       NOWAIT)  │  扫入事件   │                │
└────────────────┘            └────────────────┘
```

**应用场景**：

1. **拆分事件处理**：将不同模块的事件分离到不同的 loop 中，通过 ev_embed 在单线程中统一调度
2. **第三方库封装**：某个库内部创建了自己的 loop，通过 ev_embed 嵌入到主 loop 中
3. **分层事件处理**：高优先级事件在主 loop，低优先级在子 loop

**约束**：


| 约束                      | 说明                                                                            |
| ------------------------- | ------------------------------------------------------------------------------- |
| 嵌入的 loop 必须可嵌入    | other loop 的 backend 必须在 ev_embeddable_backends 返回的集合中（select/poll） |
| 单线程使用                | ev_embed 不跨线程，所有操作在事件循环线程执行                                   |
| 深度嵌套需谨慎            | 理论上可以多层嵌套，但每层都会增加开销                                          |
| other loop 不能有独立线程 | other loop 不能在自己的线程中独立运行 ev_run                                    |

#### ev_embeddable_backends：可嵌入的后端

```c
unsigned int ev_embeddable_backends(void) {
  return EVBACKEND_SELECT
#if EV_USE_POLL
       | EVBACKEND_POLL
#endif
       ;
}
```

只有 **select** 和 **poll** 是可嵌入的后端。epoll、kqueue、port 等高性能后端**不支持嵌入**。原因：高性能后端（如 epoll）的 backend_fd（epoll fd）本身不能通过另一个 epoll 实例来监听，而 select/poll 的 backend_fd 则是普通 pipe/fd，可以被套娃监听。

#### ev_embed_start 流程

```c
void ev_embed_start(EV_P_ ev_embed *w) {
  if (ev_is_active(w)) return;

  // 断言：被嵌入的 loop 的 backend 必须可嵌入
  assert(other->backend & ev_embeddable_backends());

  // 1. 监听 other loop 的 backend_fd
  //    用于唤醒 parent loop（other loop 有事件要处理时）
  {
    EV_P = w->other;
    if (backend_fd >= 0) {
      ev_io_init(&w->io, embed_io_cb, backend_fd, EV_READ);
      ev_set_priority(&w->io, ev_priority(w));
      ev_io_start(EV_A_ &w->io);       // 在 other loop 中启动 io watcher
    }
  }

  // 2. 在 parent loop 中注册 prepare watcher
  //    每次 parent loop 迭代前，检查 other loop 是否有 fd 变更
  ev_prepare_init(&w->prepare, embed_prepare_cb);
  ev_set_priority(&w->prepare, EV_MINPRI);
  ev_prepare_start(EV_A_ &w->prepare);

  // 3. 在 parent loop 中注册 fork watcher
  //    fork 后重建嵌入关系
  ev_fork_init(&w->fork, embed_fork_cb);
  ev_fork_start(EV_A_ &w->fork);

  ev_start(EV_A_ (W)w, 1);
}
```

**流程图**：

```
ev_embed_start(parent_loop, &w)
│
├── 1. 防重入检查 → 已激活则返回
│
├── 2. 断言：w->other 的 backend 可嵌入（select/poll）
│
├── 3. 在 other loop 中启动 IO watcher
│     ├── ev_io_init(&w->io, embed_io_cb, other->backend_fd, EV_READ)
│     └── ev_io_start(other, &w->io)
│
├── 4. 在 parent loop 中启动 prepare watcher
│     ├── ev_prepare_init(&w->prepare, embed_prepare_cb)
│     └── ev_prepare_start(parent, &w->prepare)
│
├── 5. 在 parent loop 中启动 fork watcher
│     ├── ev_fork_init(&w->fork, embed_fork_cb)
│     └── ev_fork_start(parent, &w->fork)
│
└── 6. ev_start：设置 active 标志 + 增加 activecnt
```

#### ev_embed_stop 流程

```c
void ev_embed_stop(EV_P_ ev_embed *w) {
  clear_pending(EV_A_ (W)w);
  if (!ev_is_active(w)) return;

  if (w->io.fd >= 0)
    ev_io_stop(EV_A_ &w->io);       // 停止 IO watcher
  ev_prepare_stop(EV_A_ &w->prepare);  // 停止 prepare watcher
  ev_fork_stop(EV_A_ &w->fork);        // 停止 fork watcher

  ev_stop(EV_A_ (W)w);
}
```

#### 三个内部回调的工作机制

ev_embed 通过三个内部 watcher 协作实现循环嵌套：

**1. embed_io_cb：other loop 有事件待处理**

```c
static void embed_io_cb(EV_P_ ev_io *io, int revents) {
  ev_embed *w = (ev_embed *)(((char *)io) - offsetof(ev_embed, io));

  if (ev_cb(w))
    ev_feed_event(EV_A_ (W)w, EV_EMBED);  // 通知用户回调
  else
    ev_run(w->other, EVRUN_NOWAIT);        // 自行扫入 other 的事件
}
```

当 `other->backend_fd` 可读时（即 other loop 有待处理的 IO 事件），此回调被触发。有两种处理策略：


| 策略     | 回调 cb | 行为                                                                      |
| -------- | ------- | ------------------------------------------------------------------------- |
| 主动扫入 | NULL    | 自动调用`ev_run(other, EVRUN_NOWAIT)` 处理 other 的事件                   |
| 通知用户 | 非空    | 通过`ev_feed_event` 通知用户回调，由用户在回调中决定何时 `ev_embed_sweep` |

**2. embed_prepare_cb：同步 fd 变更到 other loop**

```c
static void embed_prepare_cb(EV_P_ ev_prepare *prepare, int revents) {
  ev_embed *w = (ev_embed *)(((char *)prepare) - offsetof(ev_embed, prepare));

  {
    EV_P = w->other;
    while (fdchangecnt) {
      fd_reify(EV_A);                 // 将 fd 变更同步到 other loop 的内核事件表
      ev_run(EV_A_ EVRUN_NOWAIT);     // 处理可能产生的事件
    }
  }
}
```

此回调在 parent loop 每次迭代的 prepare 阶段执行。它切换到 other loop 的上下文，处理 other loop 中挂起的 fd 变更（fd_reify），并立即以 NOWAIT 模式运行 other loop，确保在 parent loop 进入阻塞等待之前，所有待处理的 IO 变更已同步到内核。

**3. embed_fork_cb：fork 后重建**

```c
static void embed_fork_cb(EV_P_ ev_fork *fork_w, int revents) {
  ev_embed *w = (ev_embed *)(((char *)fork_w) - offsetof(ev_embed, fork));

  ev_embed_stop(EV_A_ w);          // 先解除嵌入关系

  {
    EV_P = w->other;
    ev_loop_fork(EV_A);             // 重建 other loop 的内核状态
    ev_run(EV_A_ EVRUN_NOWAIT);    // 初始化
  }

  ev_embed_start(EV_A_ w);         // 重新建立嵌入关系
}
```

fork 后，子进程中的 epoll/kqueue fd 会失效。此回调先 stop 再 start，触发 other loop 的内核状态重建（loop_fork），然后重新建立嵌入关系。

#### ev_embed_sweep：手动扫入

```c
void ev_embed_sweep(EV_P_ ev_embed *w) {
  ev_run(w->other, EVRUN_NOWAIT);  // 以非阻塞模式运行一次 other loop
}
```

当用户设置了回调（而非 NULL）时，可以在回调中调用 `ev_embed_sweep` 手动处理 other loop 的待处理事件。

#### 在 ev_run 中的角色

```
ev_run(parent) 每次迭代：
│
├── 1. EV_PREPARE 处理
│     └── embed_prepare_cb 被调用
│           ├── 切换到 other loop 上下文
│           ├── fd_reify(other loop)      ← 同步 fd 变更
│           └── ev_run(other, NOWAIT)     ← 处理待处理事件
│
├── 2. backend_poll(waittime)
│     └── 检测 other 的 backend_fd 可读
│
├── 3. EV_CHECK 处理
│
├── 4. EV_INVOKE_PENDING
│     └── embed_io_cb 被调用（如果 backend_fd 可读）
│           ├── 回调为 NULL：ev_run(other, NOWAIT) 自动扫入
│           └── 回调非空：ev_feed_event(EMBED) → 用户回调
│
└── 5. 继续下一次迭代
```

**事件流**：

```
other loop 有事件就绪
  → other->backend_fd 可读
  → parent 的 backend_poll 返回
  → embed_io_cb 被调用
  → ev_run(other, NOWAIT) 或 用户回调
  → other loop 的事件被处理
  → other loop 中 watcher 的回调在 parent loop 的上下文中执行
```

#### 完整生命周期示例

```c
#include <ev.h>
#include <stdio.h>
#include <unistd.h>

ev_io stdin_w;
ev_embed embed_w;

static void stdin_cb(EV_P_ ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("[other] 读取到: %s", buf);
    }
}

// embed 回调非空，手动扫入
static void embed_cb(EV_P_ ev_embed *w, int revents) {
    printf("[parent] embed sweep\n");
    ev_embed_sweep(EV_A_ w);  // 手动处理 other loop 的事件
}

int main() {
    // 创建两个独立的事件循环
    struct ev_loop *parent = EV_DEFAULT;
    struct ev_loop *other = ev_loop_new(ev_embeddable_backends());

    // 在 other loop 中注册 IO watcher
    ev_io_init(&stdin_w, stdin_cb, STDIN_FILENO, EV_READ);
    ev_io_start(other, &stdin_w);

    // 将 other loop 嵌入到 parent loop
    ev_embed_init(&embed_w, embed_cb, other);
    ev_embed_start(parent, &embed_w);

    // 运行 parent loop
    ev_run(parent, 0);

    ev_loop_destroy(other);
    return 0;
}
```

#### 关键设计要点

**1. 只有 select/poll 可嵌入**

```c
unsigned int ev_embeddable_backends(void) {
  return EVBACKEND_SELECT | EVBACKEND_POLL;
}
```

epoll 的 backend_fd 是 `epoll_create` 返回的 fd，它本身不能被其他多路复用机制监听。而 select/poll 的 backend_fd 是一个 pipe 的读端（用于 self-pipe trick 唤醒），可以被正常监听。

**2. 嵌套循环 vs 多线程**


| 特性       | ev_embed（单线程嵌套）     | 多线程独立 loop      |
| ---------- | -------------------------- | -------------------- |
| 并发性     | 单线程，无竞态             | 多线程，需同步       |
| 事件优先级 | 主 loop 决定调度顺序       | 各线程独立调度       |
| 数据共享   | 直接共享数据，无需锁       | 需要互斥锁/原子操作  |
| 适用场景   | 拆分事件逻辑、封装第三方库 | CPU 密集型、多核并行 |

**3. prepare + io 双回调设计**

```
prepare 阶段：确保 other loop 的 IO 变更已同步
  → 避免 parent 进入阻塞等待后，other loop 才注册新 fd
  → 保证 other loop 的事件不会无限期延迟

io 阶段：处理 other loop 就绪的事件
  → backend_fd 可读表明 other loop 有事件待处理
  → 调用 ev_run(other, NOWAIT) 逐个处理
```

**4. offsetof 技法获取外层 struct**

```c
ev_embed *w = (ev_embed *)(((char *)io) - offsetof(ev_embed, io));
```

与 ev_stat 中从内部 timer 反推 ev_stat 一样，ev_embed 也使用 offsetof 从内部 io watcher 反推出外层的 ev_embed 结构体指针。

**5. 子 loop 的优先级管理**

```c
ev_set_priority(&w->io, ev_priority(w));   // IO watcher 继承 embed 的优先级
ev_set_priority(&w->prepare, EV_MINPRI);    // prepare watcher 设置为最低优先级
```

io watcher 继承用户设置的 embed 优先级，控制 other loop 事件在 parent loop 中的优先程度。prepare watcher 固定为最低优先级，确保其他 prepare watcher 先执行。

## 总结

libev 是一个精巧高效的 C 语言事件循环库，核心设计围绕"将异步事件转化为同步回调"这一思想展开。以下是 libev 的核心要点总结：

### 架构设计

```
libev 整体架构
┌─────────────────────────────────────────────────────────┐
│                    用户层 API                            │
│  ev_init / ev_xxx_init / ev_xxx_start / ev_xxx_stop     │
├─────────────────────────────────────────────────────────┤
│                   Watcher 层                             │
│  ev_io  ev_timer  ev_periodic  ev_signal  ev_child      │
│  ev_stat  ev_idle  ev_prepare  ev_check  ev_fork        │
│  ev_async  ev_cleanup  ev_embed                         │
├─────────────────────────────────────────────────────────┤
│                   核心调度引擎                            │
│  ev_run → fd_reify → backend_poll → time_update         │
│         → timers_reify → periodics_reify                │
│         → idle_reify → ev_invoke_pending                │
├─────────────────────────────────────────────────────────┤
│                   后端抽象层                              │
│  backend_modify / backend_poll (函数指针多态)             │
├─────────────────────────────────────────────────────────┤
│  epoll  │  kqueue  │  select  │  poll  │  port  │ iocp  │
└─────────────────────────────────────────────────────────┘
```

### 核心设计哲学


| 设计理念               | 体现                                                                                     |
| ---------------------- | ---------------------------------------------------------------------------------------- |
| **零拷贝、零内存开销** | C++ 包装层（ev++.h）不增加额外内存和运行时开销                                           |
| **可裁剪**             | EV_FEATURES 位掩码控制特性开关，编译时按需编译                                           |
| **单线程事件循环**     | 所有回调在事件循环线程执行，无需锁                                                       |
| **异步转同步**         | signal、async等异步事件通过 pipe/eventfd/signalfd 转为 IO 事件，统一在 ev_run 中同步处理 |
| **批量延迟同步**       | fd_reify 将多次 fd 变更延迟到 backend_poll 前统一处理，减少系统调用                      |
| **函数指针多态**       | 后端抽象通过函数指针实现，新增后端只需实现 init/modify/poll                              |

### 事件循环（ev_run）完整流程

```
ev_run 一次完整迭代
├── 1. 执行上一轮遗留的 pending 回调
├── 2. fork 检测（pid 变化检测）
├── 3. 执行 fork/fork 回调（子进程重建）
├── 4. 执行 prepare 回调（最先执行的用户回调时机）
├── 5. 检测 ev_break，判断是否退出
├── 6. loop_fork（重建后端内核状态）
├── 7. fd_reify（将 fd 变更同步到内核，批量系统调用）
├── 8. 计算阻塞时间（取 timer/periodic 堆顶超时时间）
├── 9. backend_poll（核心阻塞，等待 IO 就绪或超时）
├── 10. pipe 事件处理（signal/async 中转）
├── 11. time_update（更新当前时间）
├── 12. timers_reify（处理超时的 ev_timer）
├── 13. periodics_reify（处理超时的 ev_periodic）
├── 14. idle_reify（无其他事件时执行 idle）
├── 15. 执行 check 回调（最后执行的用户回调时机）
└── 16. 执行本轮所有 pending 回调
```

### Watcher 体系


| Watcher     | 基类            | 核心数据                   | 触发方式        | 典型用途           |
| ----------- | --------------- | -------------------------- | --------------- | ------------------ |
| ev_io       | EV_WATCHER_LIST | ANFD 数组 + fdchanges 列表 | 内核 IO 就绪    | 网络、文件读写     |
| ev_timer    | EV_WATCHER_TIME | 最小堆（二叉/四叉）        | 时间到达        | 超时、心跳         |
| ev_periodic | EV_WATCHER_TIME | 最小堆                     | 绝对时间到达    | 定时任务、整点执行 |
| ev_signal   | EV_WATCHER_LIST | ANSIG 全局数组             | 信号到达        | 信号处理           |
| ev_child    | EV_WATCHER_LIST | 全局 hash 表               | SIGCHLD         | 子进程监控         |
| ev_stat     | EV_WATCHER_LIST | inotify + timer 轮询       | 文件变化        | 文件监控           |
| ev_idle     | EV_WATCHER      | 动态数组                   | 空闲时          | 后台低优先级任务   |
| ev_prepare  | EV_WATCHER      | 动态数组                   | ev_run 迭代开始 | 钩子函数           |
| ev_check    | EV_WATCHER      | 动态数组                   | ev_run 迭代结束 | 钩子函数           |
| ev_fork     | EV_WATCHER      | 动态数组                   | fork 后         | 子进程重建         |
| ev_async    | EV_WATCHER      | 动态数组                   | ev_async_send   | 跨线程通知         |
| ev_embed    | EV_WATCHER      | 内部 io/prepare/fork       | 子 loop 有事件  | 事件循环嵌套       |

### 关键技术实现


| 技术              | 实现方式                                                  |
| ----------------- | --------------------------------------------------------- |
| 宏模拟继承        | EV_WATCHER / EV_WATCHER_LIST / EV_WATCHER_TIME 三级继承   |
| X-Macro 变量管理  | ev_vars.h + ev_wrap.h 实现结构体成员与访问宏自动同步      |
| 优先级调度        | 5 级优先级，pending 队列按优先级分桶，高优先级先执行      |
| 四叉堆            | 默认使用四叉堆管理定时器，比二叉堆有更好的 CPU 缓存局部性 |
| 内存管理          | 倍增扩容 + 大数组 4096 对齐，减少内存碎片                 |
| 分支预测          | expect_true/expect_false 宏，优化热路径和冷路径的代码布局 |
| pipe/eventfd 中转 | 信号和异步事件通过内核 fd 统一到 IO 事件流                |
| 条件编译          | 通过宏控制每个 watcher 类型的编译开关，按需裁剪二进制体积 |
| EV_P / EV_A 宏    | 一套宏体系同时支持多循环和单循环模式，零开销抽象          |
| offsetof 技法     | 从内部对象反推外层容器结构体指针（ev_stat、ev_embed）     |

### 适用场景

- **高性能网络服务**：epoll/kqueue 后端，适合高并发网络应用
- **嵌入式/资源受限系统**：特性可裁剪，选 select/poll 后端
- **桌面应用事件循环**：统一处理 IO、定时器、信号
- **跨平台事件处理**：一套 API 适配 Linux/BSD/macOS/Windows/Solaris
- **库和中间件**：libev 作为底层事件引擎被 Redis、Node.js（早期）等项目使用

libev 的核心价值在于：用极其精简的 C 代码（核心 ev.c 约 5200 行），实现了高性能、跨平台、可裁剪的事件循环框架，在 API 设计上兼顾了易用性和灵活性，是学习事件驱动编程和 C 语言宏编程的优秀范本。
