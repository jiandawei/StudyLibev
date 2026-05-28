#include <ev.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

struct ev_prepare prepare_watcher;
struct ev_check check_watcher;
int loop_count = 0;

static void prepare_cb(EV_P_ struct ev_prepare *w, int revents) {
    printf("Prepare: 事件循环即将开始 (次数: %d)\n", ++loop_count);
}

static void check_cb(EV_P_ struct ev_check *w, int revents) {
    printf("Check: 事件循环刚刚结束\n");
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_prepare_init(&prepare_watcher, prepare_cb);
    ev_check_init(&check_watcher, check_cb);
    
    ev_prepare_start(loop, &prepare_watcher);
    ev_check_start(loop, &check_watcher);

    printf("Prepare/Check watcher 启动，将运行5次循环\n");
    
    for (int i = 0; i < 5; i++) {
        ev_run(loop, EVRUN_ONCE);
        usleep(1000); // 1ms
    }

    ev_prepare_stop(loop, &prepare_watcher);
    ev_check_stop(loop, &check_watcher);

    printf("程序结束\n");
    return 0;
}