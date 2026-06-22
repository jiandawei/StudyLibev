#include <ev.h>
#include <stdio.h>
#include <signal.h>

struct ev_signal sigint_watcher;
struct ev_signal sigterm_watcher;

static void sigint_cb(EV_P_ ev_signal *w, int revents) {
    printf("收到 SIGINT，准备退出...\n");
    ev_break(EV_A_ EVBREAK_ALL);
}

static void sigterm_cb(EV_P_ ev_signal *w, int revents) {
    printf("收到 SIGTERM，准备退出...\n");
    ev_break(EV_A_ EVBREAK_ALL);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    ev_signal_init(&sigint_watcher, sigint_cb, SIGINT);
    ev_signal_start(loop, &sigint_watcher);
    
    ev_signal_init(&sigterm_watcher, sigterm_cb, SIGTERM);
    ev_signal_start(loop, &sigterm_watcher);
    
    printf("等待信号（Ctrl+C 或 kill 命令）\n");
    ev_run(loop, 0);
    
    printf("程序已退出\n");
    return 0;
}