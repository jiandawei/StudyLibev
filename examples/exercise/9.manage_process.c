#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

struct ev_child child_watcher;

static void child_cb(EV_P_ ev_child *w, int revents) {
    printf("子进程 %d 退出，状态: %d\n", w->rpid, WEXITSTATUS(w->rstatus));
    ev_child_stop(EV_A_ w);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    pid_t pid;
    
    pid = fork();
    if (pid == 0) {
        // 子进程
        printf("子进程启动，PID: %d\n", getpid());
        sleep(3);
        printf("子进程退出\n");
        exit(42);
    }
    
    ev_child_init(&child_watcher, child_cb, pid, 0);
    ev_child_start(loop, &child_watcher);
    
    printf("监控子进程 %d\n", pid);
    ev_run(loop, 0);
    return 0;
}