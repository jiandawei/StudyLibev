#include <ev.h>
#include <stdio.h>
#include <signal.h>

struct ev_signal signal_watcher;

static void signal_cb(EV_P_ struct ev_signal *w, int revents) {
    printf("收到信号 %d，准备退出...\n", w->signum);
    ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_signal_init(&signal_watcher, signal_cb, SIGINT);
    ev_signal_start(loop, &signal_watcher);

    printf("等待信号（按 Ctrl+C）...\n");
    ev_run(loop, 0);

    printf("程序退出\n");
    return 0;
}
