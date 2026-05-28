#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

struct ev_child child_watcher;

static void child_cb(EV_P_ struct ev_child *w, int revents) {
    printf("子进程事件触发\n");
    printf("  PID: %d\n", w->rpid);
    printf("  进程返回状态: %d\n", WEXITSTATUS(w->rstatus));
    
    if (WIFEXITED(w->rstatus)) {
        printf("  进程正常退出\n");
    } else if (WIFSIGNALED(w->rstatus)) {
        printf("  进程被信号终止: %d\n", WTERMSIG(w->rstatus));
    }
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_child_init(&child_watcher, child_cb, -1, 0);
    ev_child_start(loop, &child_watcher);

    printf("Child watcher 启动\n");
    
    // 创建子进程
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        printf("子进程 (PID: %d) 开始运行...\n", getpid());
        sleep(2);
        printf("子进程即将退出，返回状态码 42\n");
        exit(42);
    } else if (pid > 0) {
        // 父进程
        printf("父进程创建子进程，PID: %d\n", pid);
        printf("父进程等待子进程...\n");
    } else {
        perror("fork 失败");
        exit(1);
    }

    ev_run(loop, 0);

    printf("程序结束\n");
    return 0;
}