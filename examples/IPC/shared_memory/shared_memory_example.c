/*
 * sem_example.c
 * 演示使用 POSIX 命名信号量 + 共享内存实现进程间通信（IPC）
 *
 * 原理：
 *  - 两个命名信号量 producer(初值1) / consumer(初值0) 协调读写
 *  - 共享内存（shm_open + mmap）作为数据传输区
 *  - fork() 创建父子进程，父进程作生产者，子进程作消费者
 *
 * 编译：gcc -o sem_example sem_example.c -lpthread -lrt
 * 运行：./sem_example
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

/* 共享内存与信号量名称 */
#define SHM_NAME  "/shm_example"
#define SEM_PROD  "/sem_producer"
#define SEM_CONS  "/sem_consumer"

#define SHM_SIZE  4096
#define MAX_MSG   128

/* 共享内存中的数据结构 */
typedef struct {
    char data[MAX_MSG];   /* 消息缓冲区 */
    int  done;            /* 结束标志 */
} shared_data;

/* 全局信号量指针，供清理函数使用 */
static sem_t *sem_prod = NULL;
static sem_t *sem_cons = NULL;

/*
 * cleanup - 清理所有 IPC 资源
 * 关闭并删除信号量，删除共享内存对象
 */
static void cleanup(void)
{
    if (sem_prod != SEM_FAILED && sem_prod != NULL) {
        sem_close(sem_prod);
        sem_unlink(SEM_PROD);
    }
    if (sem_cons != SEM_FAILED && sem_cons != NULL) {
        sem_close(sem_cons);
        sem_unlink(SEM_CONS);
    }
    shm_unlink(SHM_NAME);
}

/*
 * handle_signal - 信号处理函数
 * 收到 SIGINT/SIGTERM 时清理资源后退出
 */
static void handle_signal(int sig)
{
    (void)sig;
    cleanup();
    exit(0);
}

/*
 * producer - 生产者（父进程）
 * 1. 创建并映射共享内存
 * 2. 等待 producer 信号量（初始为 1，第一次可直接进入）
 * 3. 写入消息到共享内存
 * 4. 释放 consumer 信号量，唤醒消费者
 * 5. 重复直到发送 "quit"
 */
static void producer(void)
{
    /* 创建共享内存对象 */
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    /* 设置共享内存大小 */
    ftruncate(shm_fd, SHM_SIZE);

    /* 映射共享内存到进程地址空间 */
    shared_data *shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(shm_fd);

    /* 初始化共享内存 */
    memset(shm, 0, SHM_SIZE);
    shm->done = 0;

    /* 待发送的消息列表 */
    const char *messages[] = {
        "Hello from producer!",
        "This is IPC via semaphore.",
        "Process communication demo.",
        "quit"
    };

    for (int i = 0; i < 4; i++) {
        /* P 操作：等待写入权限 */
        sem_wait(sem_prod);

        /* 写入消息到共享内存 */
        strncpy(shm->data, messages[i], MAX_MSG - 1);
        shm->data[MAX_MSG - 1] = '\0';

        /* 检查是否为退出消息 */
        if (strcmp(messages[i], "quit") == 0)
            shm->done = 1;

        printf("[Producer] sent: %s\n", shm->data);

        /* V 操作：通知消费者可以读取 */
        sem_post(sem_cons);

        usleep(500000); /* 模拟生产间隔 */
    }

    munmap(shm, SHM_SIZE);
    printf("[Producer] exiting.\n");
}

/*
 * consumer - 消费者（子进程）
 * 1. 打开并映射已有的共享内存
 * 2. 等待 consumer 信号量（初始为 0，需生产者先 post）
 * 3. 读取并打印消息
 * 4. 释放 producer 信号量，允许生产者继续写入
 * 5. 收到 done 标志后退出
 */
static void consumer(void)
{
    /* 打开已有的共享内存对象 */
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    /* 映射共享内存 */
    shared_data *shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(shm_fd);

    while (1) {
        /* P 操作：等待可读信号 */
        sem_wait(sem_cons);

        printf("[Consumer] received: %s\n", shm->data);

        /* 检查结束标志 */
        if (shm->done)
            break;

        /* V 操作：通知生产者可以继续写入 */
        sem_post(sem_prod);
    }

    printf("[Consumer] exiting.\n");
    munmap(shm, SHM_SIZE);
}

int main(void)
{
    /* 注册信号处理和退出清理 */
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);
    atexit(cleanup);

    /* 清理残留的 IPC 对象（防止上次异常退出遗留） */
    sem_unlink(SEM_PROD);
    sem_unlink(SEM_CONS);
    shm_unlink(SHM_NAME);

    /* 创建命名信号量：
     *   sem_prod - 初始值 1，表示生产者初始可写
     *   sem_cons - 初始值 0，表示消费者初始需等待
     */
    sem_prod = sem_open(SEM_PROD, O_CREAT, 0666, 1);
    if (sem_prod == SEM_FAILED) {
        perror("sem_open producer");
        return 1;
    }

    sem_cons = sem_open(SEM_CONS, O_CREAT, 0666, 0);
    if (sem_cons == SEM_FAILED) {
        perror("sem_open consumer");
        return 1;
    }

    /* fork() 创建子进程作为消费者，父进程作为生产者 */
    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程：消费者 */
        consumer();
        return 0;
    } else if (pid > 0) {
        /* 父进程：生产者 */
        producer();
        wait(NULL); /* 等待子进程结束 */
    } else {
        perror("fork");
        return 1;
    }

    return 0;
}
