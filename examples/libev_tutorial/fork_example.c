#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void fork_cb(EV_P_ struct ev_fork *w, int revents) {
    printf("Fork 检测到！当前 PID: %d, 父进程 PID: %d\n", (int)getpid(), (int)getppid());
}

static void timer_cb(EV_P_ struct ev_timer *w, int revents) {
    printf("Timer 触发 - PID: %d\n", (int)getpid());
    ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(EVFLAG_FORKCHECK);

    struct ev_fork fork_watcher;
    ev_fork_init(&fork_watcher, fork_cb);
    ev_fork_start(loop, &fork_watcher);

    printf("父进程 (PID: %d) 准备 fork\n", getpid());

    pid_t pid = fork();
    if (pid == 0) {
        printf("子进程 (PID: %d) 开始运行\n", getpid());

        struct ev_timer tw;
        ev_timer_init(&tw, timer_cb, 0.5, 0.);
        ev_timer_start(loop, &tw);

        ev_run(loop, 0);
        printf("子进程退出\n");
        exit(0);
    }

    struct ev_timer tw;
    ev_timer_init(&tw, timer_cb, 1., 0.);
    ev_timer_start(loop, &tw);

    ev_run(loop, 0);
    waitpid(pid, NULL, 0);
    printf("父进程结束\n");
    return 0;
}
