# 命名管道（Named Pipe / FIFO）进程间通信示例

## 一、什么是命名管道

命名管道（Named Pipe），也叫 FIFO，是一种**进程间通信（IPC）**机制。它在文件系统中存在一个路径名，**不相关的进程**可以通过这个路径名打开同一个管道进行通信。

### 命名管道 vs 匿名管道

| 特性 | 匿名管道（Pipe） | 命名管道（FIFO） |
|------|-----------------|-----------------|
| 创建方式 | `pipe()` | `mkfifo()` |
| 文件系统可见 | ❌ 不存在路径名 | ✅ 存在于文件系统 |
| 通信范围 | 仅限父子进程 | **任意进程** |
| 生命周期 | 随进程消失 | 需手动 `unlink()` 删除 |
| 半双工/全双工 | 半双工 | 半双工 |

### 命名管道的特性

- **半双工**：同一时刻只能单向传输（一端读、一端写）
- **先进先出**：数据按写入顺序读出
- **内核缓冲区**：数据不写入磁盘，存在于内核缓冲区中
- **阻塞语义**：默认情况下，`open()` 会阻塞直到对端也打开

## 二、阻塞语义详解

这是理解 FIFO 行为的关键：

```
open(FIFO_PATH, O_RDONLY)  ← 阻塞，直到有进程以写方式打开
open(FIFO_PATH, O_WRONLY)  ← 阻塞，直到有进程以读方式打开

两者同时发生 → 都返回 → 通信建立
```

```
时间线：

服务端 (读端)                    客户端 (写端)
    │                                │
    open(O_RDONLY)                   │
    │ ← 阻塞等待...                  │
    │                                open(O_WRONLY)
    │ ←────────────────────────────→ │
    两者都返回！通信建立               │
    │                                │
    read() ← 阻塞等待数据 ─────────── write()
    │                                │
```

如果使用 `O_NONBLOCK`：

```c
open(FIFO_PATH, O_RDONLY | O_NONBLOCK);  // 立即返回，不等待写端
open(FIFO_PATH, O_WRONLY | O_NONBLOCK);  // 如果没有读端 → 返回 -1，errno = ENXIO
```

## 三、示例程序结构

```
IPC/fifo/
├── fifo_server.c   ← 服务端（读端）
├── fifo_client.c   ← 客户端（写端）
└── build.sh        ← 编译脚本
```

## 四、服务端代码解析

### 完整代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define FIFO_PATH "/tmp/my_fifo"
#define BUF_SIZE 1024

static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

int main() {
    signal(SIGINT, sigint_handler);

    // 1. 如果管道文件已存在，先删除
    if (access(FIFO_PATH, F_OK) == 0) {
        unlink(FIFO_PATH);
    }

    // 2. 创建命名管道
    if (mkfifo(FIFO_PATH, 0666) < 0) {
        perror("mkfifo");
        return 1;
    }
    printf("命名管道已创建: %s\n", FIFO_PATH);

    // 3. 以只读方式打开（阻塞，直到有写端连接）
    printf("等待客户端连接...\n");
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("open fifo");
        unlink(FIFO_PATH);
        return 1;
    }
    printf("客户端已连接，开始接收消息（Ctrl+C 退出）\n\n");

    // 4. 循环读取
    char buf[BUF_SIZE];
    while (running) {
        ssize_t n = read(fifo_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("收到消息: %s", buf);
        } else if (n == 0) {
            // 写端关闭 → 重新打开等待新连接
            printf("客户端已断开，等待重新连接...\n");
            close(fifo_fd);
            fifo_fd = open(FIFO_PATH, O_RDONLY);
            if (fifo_fd < 0) {
                perror("open fifo");
                break;
            }
            printf("客户端已重新连接\n");
        } else {
            // n < 0：被信号中断（SIGINT）→ running=0，退出循环
            if (errno != EINTR) {
                perror("read fifo");
                break;
            }
        }
    }

    // 5. 清理
    close(fifo_fd);
    unlink(FIFO_PATH);
    printf("\n服务端已退出，管道已删除\n");
    return 0;
}
```

### 关键点解析

#### 1. `mkfifo()` — 创建命名管道

```c
mkfifo(FIFO_PATH, 0666);
```

- 在文件系统创建一个 FIFO 特殊文件
- `0666`：所有用户可读写
- 创建后用 `ls -l` 查看，文件类型为 `p`：

```
prw-rw-rw-  1 user  staff  0 Jun  8 10:00 /tmp/my_fifo
```

#### 2. `read()` 返回 0 — 写端关闭

```c
if (n == 0) {
    close(fifo_fd);
    fifo_fd = open(FIFO_PATH, O_RDONLY);  // 重新等待
}
```

当所有写端都关闭后，`read()` 返回 0（EOF）。服务端需要重新 `open()` 等待新的写端连接。

#### 3. 信号安全退出

```c
static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;  // 原子操作，信号安全
}
```

- `sig_atomic_t`：保证信号处理函数中的赋值是原子的
- `volatile`：防止编译器优化缓存
- Ctrl+C 触发 SIGINT → `running = 0` → 循环退出 → 清理资源

## 五、客户端代码解析

### 完整代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_PATH "/tmp/my_fifo"
#define BUF_SIZE 1024

int main() {
    // 1. 以只写方式打开（阻塞，直到有读端）
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0) {
        if (errno == ENXIO) {
            fprintf(stderr, "服务端未启动，请先运行 fifo_server\n");
        } else {
            perror("open fifo");
        }
        return 1;
    }
    printf("已连接到命名管道: %s\n", FIFO_PATH);
    printf("输入消息并回车发送（输入 exit 退出）\n\n");

    // 2. 从 stdin 读取，写入 FIFO
    char buf[BUF_SIZE];
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (strcmp(buf, "exit\n") == 0) {
            printf("客户端退出\n");
            break;
        }

        ssize_t n = write(fifo_fd, buf, strlen(buf));
        if (n < 0) {
            perror("write fifo");
            break;
        }
        printf("已发送 %zd 字节\n", n);
    }

    // 3. 关闭 FIFO（写端关闭 → 服务端 read() 返回 0）
    close(fifo_fd);
    return 0;
}
```

### 关键点解析

#### 1. `ENXIO` 错误 — 读端不存在

```c
if (errno == ENXIO) {
    fprintf(stderr, "服务端未启动，请先运行 fifo_server\n");
}
```

如果以 `O_WRONLY` 打开 FIFO 但没有读端，`open()` 返回 -1，`errno` 设为 `ENXIO`（No such device or address）。

#### 2. `fgets()` + `write()` 组合

```c
fgets(buf, sizeof(buf), stdin);  // 从键盘读取一行
write(fifo_fd, buf, strlen(buf)); // 写入 FIFO
```

- `fgets()` 会保留换行符 `\n`
- `strlen()` 包含换行符，所以服务端收到的消息自带换行

## 六、运行演示

### 编译

```bash
cd examples/IPC/fifo
./build.sh
```

### 运行

```bash
# 终端 1：启动服务端
./fifo_server
# 输出：命名管道已创建: /tmp/my_fifo
# 输出：等待客户端连接...

# 终端 2：启动客户端
./fifo_client
# 输出：已连接到命名管道: /tmp/my_fifo
# 输出：输入消息并回车发送（输入 exit 退出）

# 终端 2：输入消息
hello FIFO!
# 输出：已发送 12 字节

# 终端 1：收到消息
收到消息: hello FIFO!

# 终端 2：退出
exit
# 输出：客户端退出

# 终端 1：检测到断开
客户端已断开，等待重新连接...
# 可以再次启动客户端重新连接
```

### 也可以用命令行直接写入 FIFO

```bash
# 不需要客户端程序，直接用 echo 写入
echo "from command line" > /tmp/my_fifo

# 服务端输出
收到消息: from command line
```

## 七、常见问题

### Q1：为什么服务端 `read()` 返回 0 后要重新 `open()`？

```
FIFO 的语义：当所有写端关闭后，read() 返回 0（EOF）
如果不重新 open()，继续 read() 会一直返回 0 → 死循环
重新 open() 会阻塞，直到有新的写端连接
```

### Q2：能不能同一个 FIFO 双向通信？

```
不能！FIFO 是半双工的。

如果双方都既读又写：
  服务端 open(O_RDWR) → 不会阻塞（自己既是读端又是写端）
  客户端 open(O_RDWR) → 也不会阻塞
  → 通信建立失败

解决方案：使用两个 FIFO
  /tmp/my_fifo_req  → 客户端写，服务端读
  /tmp/my_fifo_resp → 服务端写，客户端读
```

### Q3：FIFO 的缓冲区有多大？

```
Linux：至少 65536 字节（64KB），可通过 /proc/sys/fs/pipe-max-size 调整
macOS：至少 65536 字节

写入超过缓冲区大小时：
  - 阻塞模式：write() 阻塞，直到有数据被读走
  - 非阻塞模式：write() 返回 -1，errno = EAGAIN
```

### Q4：进程异常退出，FIFO 文件会残留吗？

```
会！FIFO 文件是文件系统中的特殊文件，不会随进程消失。

本示例的处理方式：
  服务端启动时检查文件是否存在，存在则先删除
    if (access(FIFO_PATH, F_OK) == 0)
        unlink(FIFO_PATH);

  服务端退出时主动删除
    unlink(FIFO_PATH);
```

## 八、API 速查

| API | 说明 | 返回值 |
|-----|------|--------|
| `mkfifo(path, mode)` | 创建命名管道 | 0 成功，-1 失败 |
| `open(path, flags)` | 打开 FIFO | fd 成功，-1 失败 |
| `read(fd, buf, count)` | 从 FIFO 读取 | >0 字节数，0 EOF，-1 错误 |
| `write(fd, buf, count)` | 向 FIFO 写入 | 写入字节数，-1 错误 |
| `close(fd)` | 关闭 FIFO | 0 成功，-1 失败 |
| `unlink(path)` | 删除 FIFO 文件 | 0 成功，-1 失败 |
| `access(path, F_OK)` | 检查文件是否存在 | 0 存在，-1 不存在 |

## 九、错误码速查

| errno | 含义 | 触发场景 |
|-------|------|---------|
| `EEXIST` | 文件已存在 | `mkfifo()` 时路径已存在 |
| `ENXIO` | 没有读端 | `open(O_WRONLY)` 时没有读端（非阻塞） |
| `EAGAIN` | 非阻塞操作无法完成 | `write()` 缓冲区满 / `read()` 无数据 |
| `EINTR` | 被信号中断 | `read()` / `write()` 被信号打断 |
| `PIPE_BUF` | 保证原子写入的最大字节数 | ≤ PIPE_BUF 的写入是原子的 |

> `PIPE_BUF` 通常为 4096 字节（Linux），可通过 `pathconf(path, _PC_PIPE_BUF)` 查询。
