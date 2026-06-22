# 消息队列（Message Queue）进程间通信示例

> **本示例使用条件编译**：Linux 上使用 **POSIX 消息队列**（`mqueue.h`），macOS 上使用 **System V 消息队列**（`sys/msg.h`）。

## 一、什么是消息队列

消息队列是内核维护的**消息链表**，进程可以按**消息类型/优先级**发送和接收消息。与管道不同，消息是有边界的，每条消息独立存在。

### 消息队列 vs FIFO

| 特性 | FIFO | 消息队列 |
|------|------|---------|
| 数据组织 | 字节流（无边界） | 消息（有边界） |
| 读取方式 | 顺序读取 | 可按类型/优先级筛选 |
| 优先级 | 无 | ✅ 按消息类型/优先级筛选 |
| 生命周期 | 随进程消失 | 需手动删除 |
| 内核持久化 | ❌ | ✅ 进程退出后消息仍在 |
| 缓冲区满 | 写端阻塞 | 阻塞或返回错误 |

### 消息队列的核心概念

**POSIX 消息队列（Linux）：**
```
消息队列（内核中）：
┌──────────────────────────────────────────────┐
│ 消息1 [优先级=0]: "hello"                     │
│ 消息2 [优先级=3]: "urgent data"   ← 高优先级  │
│ 消息3 [优先级=0]: "world"                     │
│ 消息4 [优先级=1]: "debug info"                │
└──────────────────────────────────────────────┘
接收时：优先级高的先出队
```

**System V 消息队列（macOS）：**
```
消息队列（内核中）：
┌──────────────────────────────────────────────┐
│ 消息1 [类型=1]: "hello"                       │
│ 消息2 [类型=2]: "urgent data"                 │
│ 消息3 [类型=1]: "world"                       │
│ 消息4 [类型=3]: "debug info"                  │
└──────────────────────────────────────────────┘

接收方可以：
  msgrcv(msqid, &msg, size, 1, 0)   → 只接收类型=1的消息
  msgrcv(msqid, &msg, size, 2, 0)   → 只接收类型=2的消息
  msgrcv(msqid, &msg, size, 0, 0)   → 接收任意类型（按顺序）
```

## 二、两种消息队列 API 对比

### POSIX 消息队列（Linux）

| API | 说明 |
|-----|------|
| `mq_open()` | 创建/打开消息队列 |
| `mq_send()` | 发送消息 |
| `mq_receive()` | 接收消息 |
| `mq_close()` | 关闭消息队列 |
| `mq_unlink()` | 删除消息队列 |
| `mq_notify()` | 注册通知（异步） |
| `mq_getattr()` / `mq_setattr()` | 获取/设置属性 |

### System V 消息队列（macOS）

| API | 说明 |
|-----|------|
| `ftok()` | 生成唯一 key |
| `msgget()` | 创建/打开消息队列 |
| `msgsnd()` | 发送消息 |
| `msgrcv()` | 接收消息 |
| `msgctl()` | 控制（删除、查询信息） |

### 详细对比

| 特性 | System V (`sys/msg.h`) | POSIX (`mqueue.h`) |
|------|----------------------|-------------------|
| 标识方式 | 整数 key + msqid | 路径名（字符串） |
| 优先级 | 消息类型（long） | 优先级（unsigned int） |
| 通知机制 | 无 | 可注册信号通知 |
| macOS 支持 | ✅ | ❌ |
| Linux 支持 | ✅ | ✅ |

> 本示例使用**条件编译**：`#ifdef __linux__` 选择 POSIX 消息队列，`#else` 选择 System V 消息队列。

## 三、条件编译结构

```c
#ifdef __linux__
  // POSIX 消息队列实现（mqueue.h）
  // Linux 专用，支持优先级、异步通知
#else
  // System V 消息队列实现（sys/msg.h）
  // macOS / 其他 Unix 通用
#endif
```

| 平台 | 编译时选择 | 链接库 |
|------|-----------|--------|
| Linux | POSIX (`mqueue.h`) | `-lrt` |
| macOS | System V (`sys/msg.h`) | 无需额外库 |

## 四、消息结构

### POSIX 消息队列（Linux）

POSIX 消息没有固定结构体，发送时直接指定缓冲区和优先级：

```c
mq_send(mq, buf, len, priority);      // 发送：指定优先级
mq_receive(mq, buf, size, &prio);     // 接收：获取优先级
```

### System V 消息队列（macOS）

```c
struct msgbuf {
    long mtype;              // 消息类型（必须 > 0）
    char mtext[MAX_TEXT_SIZE]; // 消息正文
};
```

```
消息结构：
┌──────────┬──────────────────────────────────┐
│ mtype    │ mtext                             │
│ (long)   │ (用户自定义)                       │
│ 必须>0   │ 可以是任意数据                      │
└──────────┴──────────────────────────────────┘
```

**关键规则**：`mtype` 必须 > 0，因为 `msgrcv` 用 0 表示"接收任意类型"。

## 五、示例程序结构

```
IPC/message_queue/
├── mq_server.c   ← 服务端（接收端），条件编译
├── mq_client.c   ← 客户端（发送端），条件编译
├── build.sh      ← 编译脚本（自动检测平台）
└── README.md     ← 本文档
```

## 六、服务端代码解析

### Linux 部分（POSIX 消息队列）

```c
#ifdef __linux__
#include <fcntl.h>
#include <mqueue.h>

#define MQ_NAME "/my_mq"

int main() {
    // 1. 清理可能残留的队列
    mq_unlink(MQ_NAME);

    // 2. 创建消息队列（设置属性）
    struct mq_attr attr;
    attr.mq_maxmsg = 10;       // 最大消息数
    attr.mq_msgsize = 256;     // 最大消息大小

    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);

    // 3. 阻塞式接收（无需轮询）
    char buf[256];
    unsigned int prio;
    ssize_t n = mq_receive(mq, buf, 256, &prio);
    // 高优先级消息优先出队！

    // 4. 清理
    mq_close(mq);
    mq_unlink(MQ_NAME);
}
#endif
```

**POSIX 消息队列的优势**：
- `mq_receive()` 阻塞等待，**无需轮询**（比 System V 的 `IPC_NOWAIT` + `usleep` 高效）
- 优先级高的消息**自动优先出队**
- 支持 `mq_notify()` 异步通知

### macOS 部分（System V 消息队列）

```c
#else
#include <sys/msg.h>

int main() {
    // 1. 生成唯一 key
    key_t key = ftok(".", 'M');

    // 2. 创建消息队列
    int msqid = msgget(key, IPC_CREAT | 0666);

    // 3. 非阻塞轮询接收
    struct msgbuf msg;
    while (running) {
        ssize_t n = msgrcv(msqid, &msg, sizeof(msg.mtext), 0, IPC_NOWAIT);
        if (n >= 0) {
            printf("收到消息 [类型=%ld]: %s", msg.mtype, msg.mtext);
        } else if (errno == ENOMSG) {
            usleep(100000);  // 队列为空，等 100ms
        }
    }

    // 4. 删除消息队列
    msgctl(msqid, IPC_RMID, NULL);
}
#endif
```

### 关键步骤详解

#### `ftok()` — 生成唯一 key（System V）

```c
key_t key = ftok(".", 'M');
```

- 用当前目录 `"."` 和字符 `'M'` 生成一个唯一整数 key
- 客户端必须用**相同的路径和字符**才能得到相同的 key
- key 相同 → 访问同一个消息队列

#### `mq_open()` vs `msgget()` — 创建消息队列

| | POSIX (Linux) | System V (macOS) |
|---|---|---|
| 创建 | `mq_open("/my_mq", O_CREAT \| O_RDONLY, 0666, &attr)` | `msgget(key, IPC_CREAT \| 0666)` |
| 标识 | 路径名字符串 | 整数 key |
| 属性 | 可设置最大消息数和大小 | 系统默认限制 |

#### `mq_receive()` vs `msgrcv()` — 接收消息

| | POSIX (Linux) | System V (macOS) |
|---|---|---|
| 阻塞模式 | 默认阻塞 | `msgrcv(..., 0)` 阻塞 |
| 非阻塞模式 | `O_NONBLOCK` 标志 | `IPC_NOWAIT` 标志 |
| 按类型筛选 | ❌（按优先级自动排序） | ✅ `msgtyp` 参数 |
| 优先级/类型 | `unsigned int *prio` | `long mtype` |

#### `mq_unlink()` vs `msgctl(IPC_RMID)` — 删除消息队列

消息队列不会随进程消失，**必须手动删除**。

| | POSIX (Linux) | System V (macOS) |
|---|---|---|
| 删除 | `mq_unlink("/my_mq")` | `msgctl(msqid, IPC_RMID, NULL)` |
| 查看残留 | `ls /dev/mqueue/` | `ipcs -q` |
| 手动删除 | `rm /dev/mqueue/my_mq` | `ipcrm -q <msqid>` |

## 七、客户端代码解析

### Linux 部分（POSIX 消息队列）

```c
mqd_t mq = mq_open(MQ_NAME, O_WRONLY);  // 只写模式打开

unsigned int prio = 0;  // 可通过命令行参数指定
mq_send(mq, buf, strlen(buf), prio);     // 发送：指定优先级
```

### macOS 部分（System V 消息队列）

```c
int msqid = msgget(key, 0666);  // 打开已有队列（不加 IPC_CREAT）

struct msgbuf msg;
msg.mtype = mtype;              // 可通过命令行参数指定
msgsnd(msqid, &msg, len, 0);   // 发送：len 不含 mtype
```

**注意**：`msgsnd` 的 `msgsz` 参数是 `mtext` 的长度，**不包含 `mtype`**。

## 八、运行演示

### 编译

```bash
cd examples/IPC/message_queue
./build.sh
# Linux 输出：检测到 Linux，使用 POSIX 消息队列 (mqueue)
# macOS 输出：检测到 Darwin，使用 System V 消息队列 (sys/msg)
```

### 运行（Linux — POSIX 消息队列）

```bash
# 终端 1：启动服务端
./mq_server
# 输出：[POSIX] 消息队列已创建: /my_mq
# 输出：等待接收消息（Ctrl+C 退出）...

# 终端 2：启动客户端（默认优先级 0）
./mq_client
# 输出：[POSIX] 已连接到消息队列: /my_mq (发送优先级=0)

# 终端 2：输入消息
hello message queue!
# 输出：已发送 20 字节 (优先级=0)

# 终端 1：收到消息
收到消息 [优先级=0]: hello message queue!

# 终端 3：高优先级客户端
./mq_client 3
urgent!
# 输出：已发送 7 字节 (优先级=3)

# 终端 1：高优先级消息优先出队！
收到消息 [优先级=3]: urgent!
```

### 运行（macOS — System V 消息队列）

```bash
# 终端 1：启动服务端
./mq_server
# 输出：[System V] 消息队列已创建, msqid=65536, key=0x4d018f2e
# 输出：等待接收消息（Ctrl+C 退出）...

# 终端 2：启动客户端（默认类型 1）
./mq_client
# 输出：[System V] 已连接到消息队列, msqid=65536 (消息类型=1)

# 终端 2：输入消息
hello message queue!
# 输出：已发送 20 字节 (类型=1)

# 终端 1：收到消息
收到消息 [类型=1]: hello message queue!

# 终端 3：指定消息类型
./mq_client 2
urgent message!
# 输出：已发送 16 字节 (类型=2)

# 终端 1：收到消息（类型不同）
收到消息 [类型=2]: urgent message!
```

### 查看消息队列状态

```bash
# Linux（POSIX 消息队列）
ls /dev/mqueue/
cat /dev/mqueue/my_mq

# Linux / macOS（System V 消息队列）
ipcs -q

# 输出示例：
# key        msqid      owner      perms      used-bytes   messages
# 0x4d018f2e 65536      user       666        0            0
```

## 八、常见问题

### Q1：消息队列满了怎么办？

```
默认行为：msgsnd() 阻塞，直到有消息被读走腾出空间

非阻塞方式：
  msgsnd(msqid, &msg, len, IPC_NOWAIT);
  // 队列满时返回 -1，errno = EAGAIN

队列容量限制：
  Linux: /proc/sys/kernel/msgmnb（默认 16384 字节）
  macOS: sysctl kern.sysv.msgmnb
```

### Q2：进程异常退出，消息队列会残留吗？

```
会！消息队列是内核资源，不随进程消失。

查看残留：
  ipcs -q

手动删除：
  ipcrm -q <msqid>

本示例的处理方式：
  服务端启动时不自动清理（因为 System V 消息队列没有"unlink"语义）
  服务端退出时 msgctl(IPC_RMID) 删除
```

### Q3：`ftok()` 的 key 冲突怎么办？

```
ftok() 用文件 inode + 字符生成 key
如果两个不同的文件碰巧生成相同的 key → 冲突

解决方案：
  1. 使用 IPC_PRIVATE（只能亲缘进程间通信）
  2. 直接指定一个固定 key（如 0x12345678）
  3. 确保路径和字符组合唯一
```

### Q4：消息类型有什么用？

```
典型应用：多优先级消息处理

  类型 1: 普通消息
  类型 2: 警告消息
  类型 3: 紧急消息

  // 优先处理紧急消息
  msgrcv(msqid, &msg, size, 3, IPC_NOWAIT);  // 先取类型=3
  msgrcv(msqid, &msg, size, 2, IPC_NOWAIT);  // 再取类型=2
  msgrcv(msqid, &msg, size, 1, IPC_NOWAIT);  // 最后取类型=1
```

### Q5：消息队列中的消息顺序？

```
同类型消息：先进先出（FIFO）
不同类型消息：按类型分别排队

队列内部：
  类型1: msg_a → msg_b → msg_c    （FIFO）
  类型2: msg_d → msg_e             （FIFO）
  类型3: msg_f                     （FIFO）

msgrcv(msqid, &msg, size, 0, 0)  → 取第一个消息（不论类型）
msgrcv(msqid, &msg, size, 1, 0)  → 取类型1的第一个
```

## 九、API 速查

### POSIX 消息队列（Linux）

| API | 说明 | 返回值 |
|-----|------|--------|
| `mq_open(name, oflag, mode, attr)` | 创建/打开消息队列 | mqd_t 成功，-1 失败 |
| `mq_send(mq, msg, len, prio)` | 发送消息 | 0 成功，-1 失败 |
| `mq_receive(mq, buf, len, &prio)` | 接收消息 | 字节数成功，-1 失败 |
| `mq_close(mq)` | 关闭消息队列 | 0 成功，-1 失败 |
| `mq_unlink(name)` | 删除消息队列 | 0 成功，-1 失败 |
| `mq_notify(mq, &sigevent)` | 注册异步通知 | 0 成功，-1 失败 |
| `mq_getattr(mq, &attr)` | 获取属性 | 0 成功，-1 失败 |

### System V 消息队列（macOS）

| API | 说明 | 返回值 |
|-----|------|--------|
| `ftok(path, id)` | 生成 key | key 成功，-1 失败 |
| `msgget(key, flags)` | 创建/打开消息队列 | msqid 成功，-1 失败 |
| `msgsnd(msqid, msgp, msgsz, msgflg)` | 发送消息 | 0 成功，-1 失败 |
| `msgrcv(msqid, msgp, msgsz, msgtyp, msgflg)` | 接收消息 | 字节数成功，-1 失败 |
| `msgctl(msqid, IPC_RMID, NULL)` | 删除消息队列 | 0 成功，-1 失败 |
| `msgctl(msqid, IPC_STAT, buf)` | 查询信息 | 0 成功，-1 失败 |

## 十、错误码速查

| errno | 含义 | 触发场景 |
|-------|------|---------|
| `ENOENT` | 消息队列不存在 | `msgget()` / `mq_open()` 时服务端未启动 |
| `EEXIST` | 消息队列已存在 | `msgget(IPC_CREAT\|IPC_EXCL)` / `mq_open(O_CREAT\|O_EXCL)` |
| `ENOMSG` | 队列为空 | `msgrcv(IPC_NOWAIT)` |
| `EAGAIN` | 队列已满 | `msgsnd(IPC_NOWAIT)` / `mq_send(O_NONBLOCK)` |
| `EINTR` | 被信号中断 | `msgrcv()` / `mq_receive()` |
| `EINVAL` | 参数无效 | `mtype <= 0` 或 `msgsz` 超限 |
| `EMFILE` | 打开的队列过多 | `mq_open()` / `msgget()` |

## 十一、三种 IPC 方式对比

| 特性 | FIFO | 共享内存 | 消息队列 |
|------|------|---------|---------|
| 数据格式 | 字节流 | 原始内存 | 有边界的消息 |
| 同步机制 | 内核自动 | 需要信号量 | 内核自动 |
| 速度 | 中等 | 最快 | 中等 |
| 消息优先级 | ❌ | ❌ | ✅ 按类型筛选 |
| 适用场景 | 流式数据 | 大量数据共享 | 命令/事件传递 |
