#include <ev.h>
#include <stdio.h>
#include <stdlib.h>

struct ev_cleanup cleanup_watcher;

static void cleanup_cb(EV_P_ struct ev_cleanup *w, int revents) {
    printf("Cleanup: 事件循环即将退出\n");
    printf("正在清理资源...\n");
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_cleanup_init(&cleanup_watcher, cleanup_cb);
    ev_cleanup_start(loop, &cleanup_watcher);

    printf("Cleanup watcher 启动\n");
    printf("事件循环将在2秒后退出\n");

    struct ev_timer timer;
    ev_timer_init(&timer, NULL, 2.0, 0.0);
    ev_timer_start(loop, &timer);

    ev_run(loop, 0);

    printf("程序已安全退出\n");
    return 0;
}