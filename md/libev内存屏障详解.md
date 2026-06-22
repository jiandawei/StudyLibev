# libev 内存屏障详解

## 一、为什么需要内存屏障

### 1.1 编译器重排

编译器为了优化性能，可能会**重排指令顺序**：

```c
// 程序员写的顺序
data = 42;          // ① 写数据
flag = 1;           // ② 写标志

// 编译器可能重排为
flag = 1;           // ② 先写标志
data = 42;          // ① 后写数据
```

**后果**：另一个线程看到 `flag == 1` 后去读 `data`，可能读到旧值（0）。

### 1.2 CPU 重排

即使编译器不重排，CPU 也可能**乱序执行**：

```
线程 A (CPU 0)              线程 B (CPU 1)
data = 42;                  while (flag != 1) {}
flag = 1;                   print(data);  // 可能打印 0！
```

**原因**：CPU 0 的写操作可能还在**写缓冲区**中，尚未刷新到主存；CPU 1 的缓存中 `data` 还是旧值。

### 1.3 生活中的类比

```
没有内存屏障 = 没有快递签收确认

卖家（线程 A）              买家（线程 B）
把商品放进仓库              看到发货标志
贴上"已发货"标签            去仓库取货 → 可能取到空箱子！

有内存屏障 = 有快递签收确认

卖家（线程 A）              买家（线程 B）
把商品放进仓库              等待签收确认
─── 内存屏障 ───            ─── 内存屏障 ───
贴上"已发货"标签            确认收到后取货 → 一定能取到商品
```

---

## 二、三种内存屏障

| 屏障类型 | 作用 | 类比 |
|---------|------|------|
| **Full Fence** | 屏障前的所有操作不能移到屏障后，屏障后的所有操作不能移到屏障前 | 一堵完全的墙 |
| **Acquire** | 屏障后的操作不能移到屏障前（但屏障前的**写**可以移到屏障后） | 半堵墙：只挡后面的 |
| **Release** | 屏障前的操作不能移到屏障后（但屏障后的**读**可以移到屏障前） | 半堵墙：只挡前面的 |

### 2.1 精确理解 Release 屏障

Release 屏障的精确语义：

> **屏障前的操作** 不能重排到 **屏障后**；**屏障后的写操作** 不能重排到 **屏障前**

这意味着屏障前后的**写操作顺序是严格保证的**：

```c
data = 42;                 // ① 写操作（屏障前）
ECB_MEMORY_FENCE_RELEASE;  // ② Release 屏障
flag = 1;                  // ③ 写操作（屏障后）→ 不能越过屏障！
```

```
① data = 42
──── RELEASE ────
③ flag = 1       (写操作，不能往上越过屏障 ❌)

→ ① 一定在 ③ 之前完成，顺序严格保证 ✓
```

但屏障后的**读操作**可以越过屏障（因为读不影响写可见性）：

```c
data = 42;                 // ① 写操作（屏障前）
ECB_MEMORY_FENCE_RELEASE;  // ② Release 屏障
int x = something;         // ③ 读操作（屏障后）→ 可以越过屏障 ✅
flag = 1;                  // ④ 写操作（屏障后）→ 不能越过屏障 ❌
```

```
① data = 42
──── RELEASE ────
③ int x = ...   (读操作，可以往上越过屏障 ✅)
④ flag = 1      (写操作，不能往上越过屏障 ❌)
```

### 2.2 精确理解 Acquire 屏障

Acquire 屏障的精确语义：

> **屏障后的操作** 不能重排到 **屏障前**；**屏障前的读操作** 不能重排到 **屏障后**

```c
while (flag != 1) {}        // ④ 读操作（屏障前）
ECB_MEMORY_FENCE_ACQUIRE;   // ⑤ Acquire 屏障
printf("%d\n", data);       // ⑥ 读操作（屏障后）→ 不能越过屏障 ❌
data_copy = data;           // ⑦ 写操作（屏障后）→ 不能越过屏障 ❌
```

```
④ while (flag != 1)  (读操作，不能往下越过屏障 ❌)
──── ACQUIRE ────
⑥ printf(data)       (不能往上越过屏障 ❌)
⑦ data_copy = data   (不能往上越过屏障 ❌)
```

但屏障前的**写操作**可以越过屏障（Acquire 不阻止前面的写往下走）：

```c
something = 1;              // ③ 写操作（屏障前）→ 可以越过屏障 ✅
while (flag != 1) {}        // ④ 读操作（屏障前）→ 不能越过屏障 ❌
ECB_MEMORY_FENCE_ACQUIRE;   // ⑤ Acquire 屏障
printf("%d\n", data);       // ⑥ 读操作（屏障后）→ 不能越过屏障 ❌
```

### 2.3 屏障重排规则速查

| 操作 | 位置 | 能否越过 Release | 能否越过 Acquire |
|------|------|-----------------|-----------------|
| 写操作 | 屏障前，往下 | ❌ 不能 | ✅ 可以 |
| 读操作 | 屏障前，往下 | ❌ 不能 | ❌ 不能 |
| 写操作 | 屏障后，往上 | ❌ 不能 | ❌ 不能 |
| 读操作 | 屏障后，往上 | ✅ 可以 | ❌ 不能 |

### 2.4 简单例子：生产者-消费者

```c
int data = 0;
int flag = 0;

// 生产者线程
void producer() {
    data = 42;              // ① 写数据
    ECB_MEMORY_FENCE_RELEASE; // ② Release：确保 ① 在 ③ 之前完成
    flag = 1;               // ③ 写标志（写操作，不能越过 Release 屏障）
}

// 消费者线程
void consumer() {
    while (flag != 1) {}    // ④ 等待标志（读操作，不能越过 Acquire 屏障）
    ECB_MEMORY_FENCE_ACQUIRE; // ⑤ Acquire：确保 ⑥ 在 ④ 之后执行
    printf("%d\n", data);   // ⑥ 读数据 → 保证读到 42
}
```

**Release-Acquire 配对的保证**：

```
生产者                         消费者
data = 42                      │
──── RELEASE ────              │
flag = 1 ──────────────────►   while (flag != 1) {}
                               ──── ACQUIRE ────
                               printf(data) → 42 ✓

保证链：
  ① 一定在 ③ 之前完成（Release 保证：屏障前的写不能越过屏障，屏障后的写也不能越过）
  ③ 一定在 ④ 之前可见（循环等待保证）
  ④ 一定在 ⑥ 之前完成（Acquire 保证：屏障后的操作不能越过屏障）
  ∴ ① 一定在 ⑥ 之前完成 → data 一定是 42
```

### 2.5 没有 Release 的后果

```c
// 没有 Release
void producer_wrong() {
    data = 42;              // ①
    // 没有 Release 屏障！
    flag = 1;               // ② 可能被重排到 ① 之前
}

// 消费者可能看到 flag=1 但 data 还是 0
```

```
生产者                         消费者
flag = 1 ──────────────────►  while (flag != 1) {}
data = 42 (还在写缓冲区)       printf(data) → 0 ✗
```

---

## 三、libev 中的三种屏障定义

### 3.1 宏定义

```c
// 完全双向屏障
#define ECB_MEMORY_FENCE         ...

// 获取屏障（Acquire）
#define ECB_MEMORY_FENCE_ACQUIRE ...

// 释放屏障（Release）
#define ECB_MEMORY_FENCE_RELEASE ...
```

### 3.2 不同 CPU 架构的实现

| 架构 | Full Fence | Acquire | Release |
|------|-----------|---------|---------|
| **x86/x86-64** | `lock; orb` + `"memory"` | `"" : : : "memory"` | `""` |
| **ARM** | `dmb` / `dmb ish` | `dmb` | `dmb` |
| **PowerPC** | `sync` | `sync` | `sync` |
| **SPARC** | `membar #LoadStore\|#LoadLoad\|#StoreStore\|#StoreLoad` | `membar #LoadStore\|#LoadLoad` | `membar #LoadStore\|#StoreStore` |
| **MIPS** | `sync` | `sync` | `sync` |
| **C11/GCC** | `__ATOMIC_SEQ_CST` | `__ATOMIC_ACQUIRE` | `__ATOMIC_RELEASE` |

**关键理解**：

- **x86 是强序模型（TSO）**：CPU 本身保证大部分顺序，Acquire/Release 只需编译器屏障（`"memory"` clobber）
- **ARM/PowerPC 是弱序模型**：需要真正的硬件内存屏障指令

---

## 四、libev 中的核心同步问题

### 4.1 背景

libev 使用 **pipe** 将信号/异步事件从信号处理函数传递到主循环：

```
信号到达 → ev_sighandler() → evpipe_write() → 写 pipe → 主循环被唤醒 → pipecb() → 处理事件
```

**核心矛盾**：

| 场景 | 问题 |
|------|------|
| 信号处理函数中调用 `evpipe_write` | 主循环**可能正在** `backend_poll` 中等待 |
| 主循环正在 `backend_poll` 中 | 信号处理函数**可能想要**写 pipe |

两个操作**可能同时发生**，形成竞态条件。而且信号处理函数中**不能使用锁**（`pthread_mutex_lock` 不是异步安全函数）。

### 4.2 两个关键标志位

```c
pipe_write_wanted;   // 主循环设置："我在等待，有事叫醒我"
pipe_write_skipped;  // 信号处理函数设置："我来过但没叫醒你，因为你没在等"
```

| 变量 | 设置者 | 含义 | 类比 |
|------|--------|------|------|
| `pipe_write_wanted` | 主循环 | "我在睡觉，有事叫醒我" | 门上的"请敲门"牌子 |
| `pipe_write_skipped` | 信号处理函数 | "我来过但没敲门，因为你没在睡觉" | 门缝里留的纸条 |

---

## 五、`evpipe_write` 中的内存屏障详解

### 5.1 完整代码

```c
inline_speed void evpipe_write(EV_P_ EV_ATOMIC_T *flag) {
    ECB_MEMORY_FENCE;              // ① Full Fence
    if (*flag) return;

    *flag = 1;                     // 设置 sig_pending 或 async_pending
    ECB_MEMORY_FENCE_RELEASE;      // ② Release

    pipe_write_skipped = 1;
    ECB_MEMORY_FENCE;              // ③ Full Fence

    if (pipe_write_wanted) {
        pipe_write_skipped = 0;
        ECB_MEMORY_FENCE_RELEASE;  // ④ Release
        write(evpipe[1], ...);
    }
}
```

### 5.2 逐个屏障解析

#### ① `ECB_MEMORY_FENCE` — 函数入口

```
目的：确保调用 evpipe_write 之前的所有写操作对当前线程可见

场景：信号处理函数中，可能在设置某些数据后才调用 evpipe_write
      → 需要确保这些数据已经写入内存
```

#### ② `ECB_MEMORY_FENCE_RELEASE` — 设置 flag 之后

```
目的：确保 *flag = 1 在 pipe_write_skipped = 1 之前对其他线程可见

时序保证：
  *flag = 1              ← 必须先完成
  ──── RELEASE ────
  pipe_write_skipped = 1 ← 后完成

为什么需要？
  主循环检查 *flag 来判断是否有事件
  如果 pipe_write_skipped 先于 *flag 可见
  → 主循环可能看到 skipped=1 但 flag=0
  → 逻辑混乱
```

#### ③ `ECB_MEMORY_FENCE` — 设置 skipped 之后、检查 wanted 之前

```
目的：确保 pipe_write_skipped = 1 在读取 pipe_write_wanted 之前可见

这是最关键的屏障！防止以下竞态：

信号处理函数                    主循环
  pipe_write_skipped = 1         │
  ──── FENCE ────                │
  pipe_write_wanted == ?         pipe_write_wanted = 1
                                 检查 pipe_write_skipped == ?

如果没有屏障：
  信号处理函数可能先读到旧的 pipe_write_wanted (=0)
  主循环可能先读到旧的 pipe_write_skipped (=0)
  → 双方都做出错误判断 → 事件丢失！
```

#### ④ `ECB_MEMORY_FENCE_RELEASE` — 清除 skipped 之后、写 pipe 之前

```
目的：确保 pipe_write_skipped = 0 在 write() 之前可见

防止主循环在 backend_poll 返回后检查 skipped 时
看到旧值（=1），导致重复处理
```

---

## 六、`ev_run` 中的内存屏障详解

### 6.1 进入 backend_poll 前

```c
pipe_write_wanted = 1;            // 告知：我马上要等待了
ECB_MEMORY_FENCE;                 // 确保 wanted=1 在检查 skipped 之前可见

if (pipe_write_skipped) {         // 有被跳过的事件？
    // 不进入 backend_poll，直接处理
} else {
    backend_poll(EV_A_ waittime); // 正常等待
}
```

### 6.2 退出 backend_poll 后

```c
pipe_write_wanted = 0;            // 不再需要 pipe 唤醒
ECB_MEMORY_FENCE_ACQUIRE;         // 确保读到最新的 skipped 值

if (pipe_write_skipped) {         // 等待期间有事件被跳过吗？
    ev_feed_event(EV_A_ &pipe_w, EV_CUSTOM);  // 补发事件
}
```

**为什么退出时用 Acquire？**

```
pipe_write_wanted = 0;        ← 写操作
──── ACQUIRE ────
if (pipe_write_skipped) ...   ← 读操作

Acquire 保证：读 skipped 不会被重排到写 wanted 之前
否则可能看到旧的 skipped 值
```

---

## 七、完整时序图

### 7.1 正常情况（主循环在等待时信号到达）

```
主循环                              信号处理函数
  │                                    │
  │  pipe_write_wanted = 1             │
  │  ──── FENCE ────                   │
  │  pipe_write_skipped == 0           │
  │  进入 backend_poll()               │
  │  (等待中...)                       │
  │                          信号到达 ──┤
  │                          ── FENCE ──┤
  │                          *flag = 1  │
  │                          ── RELEASE ─┤
  │                          pipe_write_skipped = 1
  │                          ── FENCE ──┤
  │                          pipe_write_wanted == 1 ?
  │                                    │ 是！
  │                          pipe_write_skipped = 0
  │                          ── RELEASE ─┤
  │                          write(pipe) ──► 唤醒！
  │  ◄──────────────────────           │
  │  backend_poll() 返回               │
  │  pipe_write_wanted = 0             │
  │  ── ACQUIRE ──                     │
  │  pipe_write_skipped == 0 → 无需补发 │
  │  继续处理事件                       │
```

### 7.2 竞态情况（信号在主循环进入等待前到达）

```
主循环                              信号处理函数
  │                                    │
  │  (正在处理回调...)                  │
  │                          信号到达 ──┤
  │                          ── FENCE ──┤
  │                          *flag = 1  │
  │                          ── RELEASE ─┤
  │                          pipe_write_skipped = 1
  │                          ── FENCE ──┤
  │                          pipe_write_wanted == 0 ?
  │                                    │ 否！主循环没在等待
  │                          (不写 pipe) │
  │                                    │
  │  pipe_write_wanted = 1             │
  │  ── FENCE ────                     │
  │  pipe_write_skipped == 1 ?         │
  │  是！→ 不进入 backend_poll         │
  │  直接处理被跳过的事件 ✓             │
```

### 7.3 没有内存屏障的灾难场景

```
主循环                              信号处理函数
  │                                    │
  │  pipe_write_wanted = 1             │
  │  (没有 FENCE！)                    │
  │                          pipe_write_skipped = 1
  │                          (没有 FENCE！)│
  │  读到 pipe_write_skipped == 0      pipe_write_wanted == 0
  │  (旧值！)                          (旧值！)
  │                                    │
  │  进入 backend_poll()               不写 pipe
  │  (等待中...)                       │
  │                                    │
  │  → 永远不会被唤醒！事件丢失！ ✗     │
```

---

## 八、其他使用场景

### 8.1 `pipecb` — pipe 读端回调

```c
static void pipecb(EV_P_ ev_io *w, int revents) {
    // ... 读取 pipe 数据 ...

    pipe_write_skipped = 0;
    ECB_MEMORY_FENCE;              // 确保 skipped=0 在读取 flag 之前可见

    if (sig_pending) {
        sig_pending = 0;
        ECB_MEMORY_FENCE;          // 确保 sig_pending=0 在遍历 signals 之前可见
        for (i = EV_NSIG - 1; i--; )
            if (signals[i].pending)
                ev_feed_signal_event(EV_A_ i + 1);
    }

    if (async_pending) {
        async_pending = 0;
        ECB_MEMORY_FENCE;
        for (i = asynccnt; i--; )
            if (asyncs[i]->sent) {
                asyncs[i]->sent = 0;
                ECB_MEMORY_FENCE_RELEASE;  // 确保 sent=0 在 ev_feed_event 之前可见
                ev_feed_event(EV_A_ asyncs[i], EV_ASYNC);
            }
    }
}
```

### 8.2 `ev_signal_start` — 绑定信号到事件循环

```c
signals[w->signum - 1].loop = EV_A;
ECB_MEMORY_FENCE_RELEASE;          // 确保 loop 赋值在其他线程可见后再继续
```

### 8.3 `ev_feed_signal` — 跨线程发送信号

```c
void ev_feed_signal(int signum) {
    EV_P;
    ECB_MEMORY_FENCE_ACQUIRE;       // 确保读到最新的 loop 指针
    EV_A = signals[signum - 1].loop;
    // ...
}
```

---

## 九、为什么不用锁？

| 对比 | 锁（mutex） | 内存屏障 |
|------|------------|---------|
| **信号处理函数中可用** | ❌ `pthread_mutex_lock` 不是异步安全函数 | ✅ 内存屏障是 CPU 指令 |
| **性能** | 较慢（系统调用，可能陷入内核态） | 极快（单条 CPU 指令） |
| **复杂度** | 简单直观 | 需要仔细推理时序 |
| **适用场景** | 通用并发 | 简单标志位同步 |
| **死锁风险** | 有 | 无 |

**核心原因**：信号处理函数中**不能使用锁**，必须用内存屏障实现无锁同步。

---

## 十、简单易懂的类比总结

### 10.1 两个标志位 = 门牌 + 留言条

```
pipe_write_wanted = 门上的"请敲门"牌子
  - 主循环挂上牌子 → "我在屋里，有事敲门"
  - 主循环摘下牌子 → "我出去了，别敲门"

pipe_write_skipped = 门缝里的留言条
  - 访客留条 → "我来过，但你不在，回头处理"
  - 主循环看到留言 → "有人找过我，主动处理"
```

### 10.2 内存屏障 = 快递签收确认

```
没有内存屏障：
  快递员把包裹放门口 → 贴"已送达"标签
  收件人看到标签 → 去门口取 → 可能包裹还在路上！

有内存屏障：
  快递员把包裹放门口
  ──── RELEASE ──── (签收确认)
  贴"已送达"标签
  收件人看到标签
  ──── ACQUIRE ──── (确认读取)
  去门口取 → 包裹一定在！
```

### 10.3 屏障重排规则最终总结

| 屏障 | 挡什么 | 不挡什么 |
|------|--------|---------|
| **Release** | 屏障前的操作 ↔ 屏障后的**写**操作（双向保证写顺序） | 屏障后的**读**操作可以往上越过 |
| **Acquire** | 屏障后的操作 ↔ 屏障前的**读**操作（双向保证读顺序） | 屏障前的**写**操作可以往下越过 |
| **Full Fence** | 全部操作，双向 | 无 |

---

## 十一、同步协议规则速查

| 规则 | 屏障类型 | 目的 |
|------|---------|------|
| 写 `*flag` 后 | `ECB_MEMORY_FENCE_RELEASE` | 确保 flag 可见后再写 skipped |
| 写 `skipped` 后 | `ECB_MEMORY_FENCE` (Full) | 确保 skipped 可见后再读 wanted |
| 写 `wanted` 后 | `ECB_MEMORY_FENCE` (Full) | 确保 wanted 可见后再读 skipped |
| 读 `skipped` 前 | `ECB_MEMORY_FENCE_ACQUIRE` | 确保读到最新的 skipped 值 |
| 写 `sent` 后 | `ECB_MEMORY_FENCE_RELEASE` | 确保 sent=0 可见后再 feed_event |
| 读 `loop` 前 | `ECB_MEMORY_FENCE_ACQUIRE` | 确保读到最新的 loop 指针 |

---

## 参考资料

- `ev.c:644-781` — 内存屏障的宏定义
- `ev.c:2510-2550` — `evpipe_write` 中的屏障使用
- `ev.c:3770-3840` — `ev_run` 中的屏障使用
- `ev.c:2590-2650` — `pipecb` 中的屏障使用
- [Memory Ordering - Wikipedia](https://en.wikipedia.org/wiki/Memory_ordering)
- [C11 Atomic Operations](https://en.cppreference.com/w/c/atomic)
