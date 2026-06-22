#include <ev.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

struct ev_loop *loop;
int *resource;

static void cleanup() {
    printf("清理资源...\n");
    if (resource) free(resource);
}

static void signal_cb(EV_P_ ev_signal *w, int revents) {
    printf("收到信号 %d\n", w->signum);
    cleanup();
    ev_break(EV_A_ EVBREAK_ALL);
}

int main() {
    struct ev_signal sig_watcher;
    
    loop = ev_default_loop(0);
    resource = malloc(1024);
    
    ev_signal_init(&sig_watcher, signal_cb, SIGINT);
    ev_signal_start(loop, &sig_watcher);
    
    printf("程序运行中（Ctrl+C 退出）\n");
    ev_run(loop, 0);
    
    return 0;
}
