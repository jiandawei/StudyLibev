#include <ev.h>
#include <stdio.h>
#include <time.h>

struct ev_periodic periodic_watcher;

static void periodic_cb(EV_P_ struct ev_periodic *w, int revents) {
    time_t now = time(NULL);
    printf("周期性定时器触发: %s", ctime(&now));
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_periodic_init(&periodic_watcher, periodic_cb, 
                     ev_now(loop), 2.0, NULL);
    ev_periodic_start(loop, &periodic_watcher);

    printf("周期性定时器启动，每2秒触发一次，按 Ctrl+C 退出\n");
    ev_run(loop, 0);

    return 0;
}