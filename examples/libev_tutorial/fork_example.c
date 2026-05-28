#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

struct ev_fork fork_watcher;
int fork_count = 0;

static void fork_cb(EV_P_ struct ev_fork *w, int revents) {
    fork_count++;
    printf("Fork 回调触发 (次数: %d)\n", fork_count);
    printf("  当前 PID: %d\n", (int)getpid());
    printf("  父进程 PID: %d\n", (int)getppid());
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_fork_init(&fork_watcher, fork_cb);
    ev_fork_start(loop, &fork_watcher);

    printf("Fork watcher 启动\n");
    printf("准备创建第一个子进程...\n");

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // 第一个子进程
        printf("第一个子进程 (PID: %d)\n", getpid());
        sleep(1);
        printf("第一个子进程退出\n");
        exit(0);
    }

    waitpid(pid1, NULL, 0); // 等待第一个子进程

    printf("-----------------\n");
    printf("准备创建第二个子进程...\n");

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // 第二个子进程
        printf("第二个子进程 (PID: %d)\n", getpid());
        sleep(1);
        printf("第二个子进程退出\n");
        exit(0);
    }

    // 父进程继续运行事件循环
    ev_run(loop, 0);
    waitpid(pid2, NULL, 0);

    printf("程序结束\n");
    return 0;
}