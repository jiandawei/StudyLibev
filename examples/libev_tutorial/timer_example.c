#include <ev.h>
#include <stdio.h>
#include <time.h>

struct ev_timer timer_watcher;

static void timer_cb(EV_P_ struct ev_timer *w, int revents) {
    time_t now = time(NULL);
    printf("定时器触发: %s", ctime(&now));
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_timer_init(&timer_watcher, timer_cb, 1.0, 1.0);  // 1秒后开始，之后每1秒触发
    ev_timer_start(loop, &timer_watcher);

    printf("定时器启动，按 Ctrl+C 退出\n");
    ev_run(loop, 0);

    return 0;
}
