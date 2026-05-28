#include <ev.h>
#include <stdio.h>
#include <unistd.h>

static int counter = 0;

static void idle_cb1(EV_P_ struct ev_idle *w, int revents) {
    counter++;
    printf("[idle_cb1]空闲回调执行计数: %d\n", counter);
    
    if (counter >= 10) {
        ev_idle_stop(EV_A_ w);
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

static void idle_cb2(EV_P_ struct ev_idle *w, int revents) {
    counter++;
    printf("[idle_cb2]空闲回调执行计数: %d\n", counter);
    
    if (counter >= 10) {
        ev_idle_stop(EV_A_ w);
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

int main(void) {
    struct ev_idle idle_watcher1, idle_watcher2;
    struct ev_loop *loop = ev_default_loop(0);

    ev_idle_init(&idle_watcher1, idle_cb1);
    ev_idle_start(loop, &idle_watcher1);

    ev_idle_init(&idle_watcher2, idle_cb2);
    ev_idle_start(loop, &idle_watcher2);

    printf("Idle watcher 启动，将在执行10次后退出\n");
    ev_run(loop, 0);

    printf("程序结束\n");
    return 0;
}