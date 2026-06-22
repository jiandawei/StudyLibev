# libev 时钟与时间管理详解

## 一、为什么 libev 需要多种时钟？

事件循环的核心是**定时器调度**，而定时器依赖时间。但系统时间存在以下问题：

| 问题 | 说明 |
|------|------|
| **时间回拨** | NTP 同步、用户手动修改时间，导致时间突然变小 |
| **时间跳变** | 系统休眠后恢复，时间突然变大 |
| **精度不足** | `gettimeofday` 只有微秒精度 |
| **性能开销** | 每次都调用系统调用获取时间太慢 |

libev 的解决方案：**使用两种时钟，互相配合**。

---

## 二、两种系统时钟

### 2.1 CLOCK_REALTIME（实时时钟 / 墙钟时间）

```c
clock_gettime(CLOCK_REALTIME, &ts);
```

| 特性 | 说明 |
|------|------|
| **含义** | 从 1970-01-01 00:00:00 UTC 到现在的秒数 |
| **受 NTP 影响** | ✅ 是，可能被调整 |
| **受用户修改影响** | ✅ 是，可能被回拨或跳变 |
| **精度** | 纳秒级 |
| **类比** | 墙上的挂钟，别人可以拨动 |

### 2.2 CLOCK_MONOTONIC（单调时钟）

```c
clock_gettime(CLOCK_MONOTONIC, &ts);
```

| 特性 | 说明 |
|------|------|
| **含义** | 从系统启动到现在的秒数 |
| **受 NTP 影响** | ❌ 否，不会被调整 |
| **受用户修改影响** | ❌ 否，只增不减 |
| **精度** | 纳秒级 |
| **类比** | 秒表，只会往前走，不会倒退 |

### 2.3 对比

```
CLOCK_REALTIME（墙钟）:
  10:00 → 10:01 → 10:02 → 9:30（NTP 回拨！）→ 9:31 → ...
                                        ↑ 可能回退！

CLOCK_MONOTONIC（秒表）:
  0s → 1s → 2s → 3s → 4s → 5s → ...
                          ↑ 永远递增
```

---

## 三、libev 中的时间变量

### 3.1 四个核心变量

```c
ev_tstamp mn_now;       // 单调时钟的当前时间（基于 CLOCK_MONOTONIC）
ev_tstamp ev_rt_now;    // 实时时钟的当前时间（基于 CLOCK_REALTIME）
ev_tstamp now_floor;    // 本轮循环开始时的 mn_now（用于去重）
ev_tstamp rtmn_diff;    // 实时时钟与单调时钟的差值 = ev_rt_now - mn_now
```

### 3.2 初始化（loop_init）

```c
ev_rt_now  = ev_time();    // 获取实时时间
mn_now     = get_clock();  // 获取单调时间
now_floor  = mn_now;       // 记录本轮起始时间
rtmn_diff  = ev_rt_now - mn_now;  // 计算差值
```

### 3.3 各变量的用途

| 变量 | 时钟源 | 用途 |
|------|--------|------|
| `mn_now` | CLOCK_MONOTONIC | **ev_timer** 的超时判断（相对定时器） |
| `ev_rt_now` | CLOCK_REALTIME | **ev_periodic** 的超时判断（绝对定时器） |
| `now_floor` | CLOCK_MONOTONIC | 避免同一轮循环中 timer 被重复触发 |
| `rtmn_diff` | 差值 | 在单调时钟和实时时钟之间转换 |

---

## 四、两个时间获取函数

### 4.1 `ev_time()` — 获取实时时间

```c
ev_tstamp ev_time(void) {
#if EV_USE_REALTIME
    if (have_realtime) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);  // 优先用 CLOCK_REALTIME
        return ts.tv_sec + ts.tv_nsec * 1e-9;
    }
#endif
    struct timeval tv;
    gettimeofday(&tv, 0);  // 兜底用 gettimeofday
    return tv.tv_sec + tv.tv_usec * 1e-6;
}
```

| 优先级 | 方法 | 精度 | 特点 |
|--------|------|------|------|
| 1 | `clock_gettime(CLOCK_REALTIME)` | 纳秒 | 现代 POSIX 标准 |
| 2 | `gettimeofday()` | 微秒 | 老旧接口，已过时 |

### 4.2 `get_clock()` — 获取单调时间

```c
inline_size ev_tstamp get_clock(void) {
#if EV_USE_MONOTONIC
    if (have_monotonic) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);  // 优先用 CLOCK_MONOTONIC
        return ts.tv_sec + ts.tv_nsec * 1e-9;
    }
#endif
    return ev_time();  // 兜底用 ev_time（退化为实时时钟）
}
```

| 优先级 | 方法 | 精度 | 特点 |
|--------|------|------|------|
| 1 | `clock_gettime(CLOCK_MONOTONIC)` | 纳秒 | 单调递增，不受回拨影响 |
| 2 | `ev_time()` | 纳秒/微秒 | 兜底，可能回拨 |

### 4.3 时钟检测（loop_init）

```c
// 检测系统是否支持 CLOCK_REALTIME
if (!have_realtime) {
    struct timespec ts;
    if (!clock_gettime(CLOCK_REALTIME, &ts))
        have_realtime = 1;  // 支持则标记
}

// 检测系统是否支持 CLOCK_MONOTONIC
if (!have_monotonic) {
    struct timespec ts;
    if (!clock_gettime(CLOCK_MONOTONIC, &ts))
        have_monotonic = 1;  // 支持则标记
}
```

**设计思路**：运行时检测，而非编译时假设。即使编译时启用了 `EV_USE_MONOTONIC`，运行时仍需验证。

---

## 五、`time_update` — 时间更新与时间跳变处理

这是 libev 时间管理的核心函数，在每次 `backend_poll` 前后被调用。

### 5.1 完整逻辑流程

```
有单调时钟（have_monotonic）？
    │
    ├── 是 ──────────────────────────────────────────┐
    │   mn_now = get_clock()  // 更新单调时间         │
    │   │                                              │
    │   mn_now - now_floor < 0.5s ?                    │
    │   ├── 是 → ev_rt_now = rtmn_diff + mn_now       │
    │   │        （插值计算，避免频繁系统调用）          │
    │   │        return                                │
    │   │                                              │
    │   └── 否 → now_floor = mn_now                   │
    │            ev_rt_now = ev_time()  // 真正获取实时时间
    │            │                                     │
    │            检测时间跳变（循环4次）                  │
    │            │                                     │
    │            ├── 跳变 < 1s → 正常，return           │
    │            └── 跳变 >= 1s → 重新调度 periodics    │
    │                                                  │
    └── 否 ───────────────────────────────────────────┘
        ev_rt_now = ev_time()                          │
        │                                              │
        检测时间回拨或大跳变                              │
        ├── 回拨或跳变 > max_block + 1s                  │
        │   → timers_reschedule()  // 重新调度 timers   │
        │   → periodics_reschedule()                    │
        └── 正常                                        │
                                                       │
        mn_now = ev_rt_now  // 没有单调时钟时退化为实时时钟
```

### 5.2 有单调时钟时的优化

```c
if (have_monotonic) {
    mn_now = get_clock();  // 只获取单调时间（快）

    // 距离上次更新不到 0.5 秒 → 用插值代替系统调用
    if (mn_now - now_floor < MIN_TIMEJUMP * .5) {
        ev_rt_now = rtmn_diff + mn_now;  // 插值计算实时时间
        return;  // 不需要调用 ev_time()，省一次系统调用
    }

    // 超过 0.5 秒 → 真正获取实时时间
    now_floor = mn_now;
    ev_rt_now = ev_time();
}
```

**为什么用插值？**

```
每次调用 clock_gettime() 是一次系统调用，开销约 20-100ns
而 mn_now 已经获取了，rtmn_diff 是已知的差值
所以 ev_rt_now ≈ rtmn_diff + mn_now  （误差 < 0.5s）

这样在 0.5s 内不需要调用第二次 clock_gettime(CLOCK_REALTIME)
```

### 5.3 时间跳变检测

```c
for (i = 4; --i; ) {
    ev_tstamp diff;
    rtmn_diff = ev_rt_now - mn_now;
    diff = odiff - rtmn_diff;  // 新旧差值的差

    if (|diff| < MIN_TIMEJUMP)  // 跳变 < 1s
        return;  // 正常

    // 跳变 >= 1s，重新获取时间
    ev_rt_now = ev_time();
    mn_now    = get_clock();
    now_floor = mn_now;
}

// 4 次都检测到跳变 → 重新调度 periodics
periodics_reschedule(EV_A);
```

**为什么要循环 4 次？**

```
可能在 ev_time() 和 get_clock() 之间被抢占（preempted）
导致两个时间不一致

第 1 次：可能被抢占，结果不可靠
第 2 次：几乎一定能成功
第 3-4 次：额外保险

注释原文：
"on the choice of '4': one iteration isn't enough,
 in case we get preempted during the calls to
 ev_time and get_clock. a second call is almost guaranteed
 to succeed in that case, though."
```

### 5.4 没有单调时钟时的处理

```c
else {
    ev_rt_now = ev_time();

    // 检测回拨或大跳变
    if (mn_now > ev_rt_now || ev_rt_now > mn_now + max_block + MIN_TIMEJUMP) {
        // 时间回拨 或 跳变超过 max_block + 1s
        timers_reschedule(EV_A_ ev_rt_now - mn_now);
        periodics_reschedule(EV_A);
    }

    mn_now = ev_rt_now;  // 退化为实时时钟
}
```

**没有单调时钟时，timer 和 periodic 都可能受时间回拨影响**，所以需要重新调度。

---

## 六、不同 Watcher 使用不同时钟

### 6.1 ev_timer — 使用 `mn_now`（单调时钟）

```c
// timers_reify: 检查 timer 是否超时
if (timercnt && ANHE_at(timers[HEAP0]) < mn_now) {
    // 堆顶 timer 的超时时间 < mn_now → 超时了
}

// 计算 backend_poll 等待时间
ev_tstamp to = ANHE_at(timers[HEAP0]) - mn_now;
```

**为什么用单调时钟？**

```
ev_timer 是相对定时器（"5秒后触发"）
如果用实时时钟，时间回拨会导致：
  - timer 提前触发（时间回拨 → mn_now 变小 → 超时条件提前满足）
  - 或延迟触发（时间前调 → mn_now 变大 → 超时条件延迟满足）

用单调时钟，时间只增不减，timer 行为可预测
```

### 6.2 ev_periodic — 使用 `ev_rt_now`（实时时钟）

```c
// periodic_recalc: 计算下次超时时间
ev_tstamp at = w->offset + interval * ev_floor((ev_rt_now - w->offset) / interval);

// periodics_reify: 检查 periodic 是否超时
while (periodiccnt && ANHE_at(periodics[HEAP0]) < ev_rt_now) {
    // 堆顶 periodic 的超时时间 < ev_rt_now → 超时了
}

// 计算 backend_poll 等待时间
ev_tstamp to = ANHE_at(periodics[HEAP0]) - ev_rt_now;
```

**为什么用实时时钟？**

```
ev_periodic 是绝对定时器（"每天 8:00 触发"）
offset 是墙钟时间，必须用实时时钟比较

如果用单调时钟，"8:00" 这个概念没有意义
因为单调时钟从系统启动开始计时，与墙钟无关
```

### 6.3 对比

| Watcher | 时钟 | 时间变量 | 原因 |
|---------|------|---------|------|
| `ev_timer` | CLOCK_MONOTONIC | `mn_now` | 相对定时，不受时间回拨影响 |
| `ev_periodic` | CLOCK_REALTIME | `ev_rt_now` | 绝对定时，需要墙钟时间 |

---

## 七、`now_floor` 的去重作用

### 7.1 问题

同一轮事件循环中，`time_update` 可能被多次调用，`mn_now` 可能微小变化：

```
同一轮循环：
  time_update() → mn_now = 100.001
  处理 timer A（耗时 0.001s）
  time_update() → mn_now = 100.002
  处理 timer B → 如果 B 的超时时间是 100.001，会被再次触发！
```

### 7.2 解决方案

```c
// timer 重复触发时，确保 at 不小于 now_floor
if (ev_at(w) < mn_now)
    ev_at(w) = mn_now;
```

`now_floor` 记录本轮循环开始时的 `mn_now`，确保同一轮中不会因为 `mn_now` 微小增长而重复触发 timer。

---

## 八、`ev_sleep` — 休眠函数

```c
void ev_sleep(ev_tstamp delay) {
    if (delay > 0.) {
#if EV_USE_NANOSLEEP
        struct timespec ts;
        EV_TS_SET(ts, delay);
        nanosleep(&ts, 0);       // 优先：纳秒级精度
#elif defined _WIN32
        Sleep((unsigned long)(delay * 1e3));  // Windows：毫秒级
#else
        struct timeval tv;
        EV_TV_SET(tv, delay);
        select(0, 0, 0, 0, &tv); // 兜底：用 select 模拟休眠
#endif
    }
}
```

| 优先级 | 方法 | 精度 | 平台 |
|--------|------|------|------|
| 1 | `nanosleep()` | 纳秒 | POSIX |
| 2 | `Sleep()` | 毫秒 | Windows |
| 3 | `select()` | 微秒 | 老旧 POSIX 兜底 |

---

## 九、`ev_now` 与 `ev_now_update`

### 9.1 `ev_now()` — 获取 loop 缓存的时间

```c
ev_tstamp ev_now(EV_P) {
    return ev_rt_now;  // 返回缓存值，不是实时获取
}
```

**注意**：`ev_now()` 返回的是**上次更新时**的缓存值，不是当前真实时间。

### 9.2 `ev_now_update()` — 强制更新缓存

```c
void ev_now_update(EV_P) {
    time_update(EV_A_ 1e100);
}
```

**使用场景**：在长时间的计算后，需要获取更精确的当前时间。

---

## 十、完整时间流程图

```
ev_run() 主循环
    │
    ├── time_update(max_block=1e100)  // 初始更新
    │   ├── mn_now = get_clock()      // 获取单调时间
    │   └── ev_rt_now = ev_time()     // 获取实时时间
    │
    ├── 计算 waittime（基于 mn_now 和 ev_rt_now）
    │   ├── timer:  waittime = timers[HEAP0].at - mn_now
    │   └── periodic: waittime = periodics[HEAP0].at - ev_rt_now
    │
    ├── pipe_write_wanted = 1
    ├── backend_poll(waittime)        // 等待 IO 事件
    ├── pipe_write_wanted = 0
    │
    ├── time_update(max_block=waittime+sleeptime)  // poll 后更新
    │   ├── 快速路径：插值计算 ev_rt_now
    │   └── 慢速路径：真正获取时间 + 跳变检测
    │
    ├── timers_reify()                // 用 mn_now 判断 timer 超时
    ├── periodics_reify()             // 用 ev_rt_now 判断 periodic 超时
    │
    └── invokes_pending()             // 执行回调
```

---

## 十一、总结

### 11.1 设计原则

| 原则 | 实现方式 |
|------|---------|
| **不受时间回拨影响** | timer 用 CLOCK_MONOTONIC |
| **支持绝对时间** | periodic 用 CLOCK_REALTIME |
| **减少系统调用** | 0.5s 内用插值代替 `ev_time()` |
| **检测时间跳变** | 循环 4 次比较新旧 `rtmn_diff` |
| **避免重复触发** | `now_floor` 去重 |
| **优雅降级** | 没有单调时钟时退化为实时时钟 |

### 11.2 变量速查

| 变量 | 时钟源 | 更新时机 | 用途 |
|------|--------|---------|------|
| `mn_now` | CLOCK_MONOTONIC | `time_update()` | ev_timer 超时判断 |
| `ev_rt_now` | CLOCK_REALTIME | `time_update()` | ev_periodic 超时判断 |
| `now_floor` | CLOCK_MONOTONIC | `time_update()` 慢速路径 | timer 去重 |
| `rtmn_diff` | 差值 | `time_update()` | 时钟转换 + 跳变检测 |

### 11.3 函数速查

| 函数 | 时钟源 | 开销 | 用途 |
|------|--------|------|------|
| `ev_time()` | CLOCK_REALTIME | 系统调用 | 获取实时时间 |
| `get_clock()` | CLOCK_MONOTONIC | 系统调用 | 获取单调时间（内部使用） |
| `ev_now()` | 缓存 | 无 | 获取 loop 缓存的实时时间 |
| `ev_now_update()` | 两者 | 系统调用 | 强制更新缓存时间 |
| `ev_sleep()` | — | 阻塞 | 休眠指定时间 |

---

## 参考资料

- `ev.c:1869-1900` — `ev_time()` 和 `get_clock()` 实现
- `ev.c:1913-1940` — `ev_sleep()` 实现
- `ev.c:3635-3700` — `time_update()` 实现
- `ev.c:2990-3010` — `loop_init()` 中的时间初始化
- `ev.c:3488-3520` — `timers_reify()` 使用 `mn_now`
- `ev.c:3530-3590` — `periodics_reify()` 使用 `ev_rt_now`
- `ev_vars.h:41-50` — 时间变量定义
