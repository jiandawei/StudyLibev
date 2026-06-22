#include <ev.h>
#include <stdio.h>

void timer_cb(EV_P_ struct ev_timer *w, int revents)
{
    printf("定时器触发，2秒后再次触发\n");
}

int main()
{
    struct ev_loop *loop = ev_default_loop(0);
    struct ev_timer timer;
    ev_timer_init(&timer, timer_cb, 2.0, 1.0);
    ev_timer_start(loop, &timer);

    ev_run(loop, 0);

    printf("示例程序结束\n");
    return 0;
}
