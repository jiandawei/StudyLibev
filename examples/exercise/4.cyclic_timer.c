#include <ev.h>
#include <stdio.h>
#include <time.h>

static void periodic_cb(EV_P_ ev_periodic *w, int revents) {
    time_t now = time(NULL);
    printf("周期性任务: %s", ctime(&now));
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    ev_periodic periodic;
    
    // 每10秒触发一次
    ev_periodic_init(&periodic, periodic_cb, 0., 10., 0);
    ev_periodic_start(loop, &periodic);
    
    printf("周期性任务启动（Ctrl+C 退出）\n");
    ev_run(loop, 0);
    return 0;
}