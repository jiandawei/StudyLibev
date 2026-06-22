# C++ 内存屏障（Memory Barrier / Memory Order）

## 为什么需要内存屏障？

现代 CPU 和编译器为了优化性能，会**重排指令**。单线程下重排不影响结果，但多线程下会导致**可见性问题**：

```cpp
// 线程1                  // 线程2
data = 100;               while (!ready);
ready = true;             assert(data == 100);  // 可能失败！
```

即使 `ready` 是原子变量，没有内存屏障时，线程2可能看到 `ready=true` 但 `data` 还是旧值。

内存屏障约束 CPU 和编译器的重排行为，保证跨线程的**可见性**和**顺序**。

## C++ 的六种内存序

### 1. `memory_order_relaxed` — 最宽松


| 保证             | 不保证               |
| ---------------- | -------------------- |
| 原子操作的原子性 | 任何顺序约束、可见性 |

仅保证操作本身原子（不撕裂），但其他线程看到的值可能是任意顺序。

**场景**：计数器（只关心最终值）、自增统计。

```cpp
atomic<int> x{0}, y{0};
// 线程1
x.store(1, memory_order_relaxed);
y.store(1, memory_order_relaxed);

// 线程2
while (y.load(memory_order_relaxed) == 0);
// 可能 x 还是 0！因为 store(x) 和 store(y) 可能重排
```

### 2. `memory_order_release` + `memory_order_acquire` — 最常用


| 操作             | 屏障效果                              |
| ---------------- | ------------------------------------- |
| `store(release)` | 之前所有写操作不能重排到该 store 之后 |
| `load(acquire)`  | 之后所有读操作不能重排到该 load 之前  |

**happens-before 关系**：线程A的 `store(release)` 与线程B的 `load(acquire)` 配对，A在 release 之前的所有写入对 B 在 acquire 之后的所有读取可见。

**场景**：生产者-消费者、消息传递、互斥锁实现。

```cpp
atomic<bool> ready{false};
int data = 0;

// 生产者
data = 100;
ready.store(true, memory_order_release);

// 消费者
while (!ready.load(memory_order_acquire));
// 此时 data == 100 一定成立
cout << data;  // 100
```

### 3. `memory_order_seq_cst` — 严格（默认）

全称：sequential consistency（顺序一致性）

所有 seq_cst 操作在所有线程中维持**单一全局顺序**。禁止一切重排，性能开销最大。

**场景**：需要全局一致性的场景、默认内存序（不指定时用此值）。

```cpp
atomic<int> a{0}, b{0};
int x, y;

// 线程1                      // 线程2
a.store(1, seq_cst);          b.store(1, seq_cst);
x = b.load(seq_cst);          y = a.load(seq_cst);

// seq_cst 保证：不可能出现 x==0 && y==0
// 因为全局顺序只能有两种：(a=1→b=1) 或 (b=1→a=1)
```

### 4. `memory_order_acq_rel` — 双向

同时拥有 acquire 和 release 语义。用于 **RMW（读-改-写）** 操作，如 `fetch_add`、`compare_exchange`。

**场景**：RMW 原子操作。

```cpp
atomic<int> cnt{0};

// 等价于：先 acquire 读旧值，再 release 写新值
int old = cnt.fetch_add(1, memory_order_acq_rel);
```

### 5. `memory_order_consume` — 有争议

C++17 后不推荐使用。语义是"依赖关系"，但编译器实现困难，通常直接 upgrade 为 acquire。

---

## seq_cst vs acq_rel 的核心区别

关键差异在 **StoreLoad 重排**——`acq_rel` **不禁止** StoreLoad，`seq_cst` **禁止**：

```cpp
atomic<int> a{0}, b{0};
int x, y;

// seq_cst: 不可能 x==0 && y==0
// 全局只有一个顺序：(a=1→b=1) 或 (b=1→a=1)
线程1: a.store(1, seq_cst); x = b.load(seq_cst);
线程2: b.store(1, seq_cst); y = a.load(seq_cst);

// acq_rel: 可能出现 x==0 && y==0
// StoreLoad 未禁止，每个线程的 store 可能重排到 load 之后
线程1: a.store(1, acq_rel); x = b.load(acq_rel);
线程2: b.store(1, acq_rel); y = a.load(acq_rel);
// 允许的执行顺序: x=b → a=1 → y=a → b=1  得出 x=0, y=0
```


|                    | `seq_cst`                            | `acq_rel`                                     |
| ------------------ | ------------------------------------ | --------------------------------------------- |
| **范围**           | 全局所有线程维持**单一总顺序**       | 仅当前线程的 acquire + release 语义           |
| **StoreLoad 重排** | **禁止**（最严格）                   | **不禁止**                                    |
| **典型场景**       | Dekker 算法等需全局一致性的场景      | RMW 操作（fetch_add, CAS）                    |
| **开销**           | 最大（x86 上生成`mfence` 或 `lock`） | 较小（x86 上 RMW 自带`lock`，几乎零额外开销） |

> 实际中 99% 的场景用 `release/acquire` 就够了，`seq_cst` 几乎只在需要 StoreLoad 屏障的特定并发算法中使用。

## 汇总对比


| 内存序    | 方向                 | 开销 | 用途                    |
| --------- | -------------------- | ---- | ----------------------- |
| `relaxed` | 无                   | 最低 | 计数器、统计            |
| `acquire` | 读屏障（后不可前移） | 低   | load 搭配 release store |
| `release` | 写屏障（前不可后移） | 低   | store 搭配 acquire load |
| `acq_rel` | 双向                 | 中   | RMW 操作                |
| `seq_cst` | 全局顺序             | 最高 | 需要全局一致的场景      |
| `consume` | 数据依赖             | 低   | 已废弃，勿用            |

## 常见模式

### 模式1：释放-获取（Release-Acquire）

```
线程A (release)                   线程B (acquire)
      |                               |
 data = 100                    while (!ready);
      |                               |
 ready = true ---happens-before--- 读 ready
                                       |
                                 assert(data == 100) ✓
```

### 模式2：释放-消费（Release-Consume，已不推荐）

### 模式3：顺序一致（Seq_Cst）

```
所有 seq_cst 操作都在同一个全局时间线上
线程A：a=1 —————→ x=b
                    ↓  全局顺序
线程B：b=1 —————→ y=a
最多有 2!=2 种顺序，不可能是 x=0 && y=0
```

## 常见错误

```cpp
// 错误：store 不能用 acquire
ready.store(true, memory_order_acquire);   // ❌

// 错误：load 不能用 release
while (!ready.load(memory_order_release));  // ❌

// 正确搭配
ready.store(true, memory_order_release);    // ✅
while (!ready.load(memory_order_acquire));  // ✅
```

## 参考

- `std::atomic` 头文件
- [cppreference: memory_order](https://en.cppreference.com/w/cpp/atomic/memory_order)
- libev 源码中 `EV_ATOMIC_T`、`ECB_MEMORY_FENCE` 宏的使用
