#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

struct ev_loop *main_loop;

struct ev_timer timer1;
struct ev_timer timer2;
struct ev_timer stop_timer;

int timer1_events = 0;
int timer2_events = 0;
bool should_exit = false;

static void timer1_cb(EV_P_ struct ev_timer *w, int revents) {
    timer1_events++;
    printf("定时器1触发 (事件: %d)\n", timer1_events);
}

static void timer2_cb(EV_P_ struct ev_timer *w, int revents) {
    timer2_events++;
    printf("定时器2触发 (事件: %d)\n", timer2_events);
}

static void stop_cb(EV_P_ struct ev_timer *w, int revents) {
    should_exit = true;
    ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
    printf("多事件示例（嵌入循环概念的演示）\n");
    printf("==================================\n\n");

    // 创建主循环
    main_loop = ev_default_loop(0);

    if (!main_loop) {
        fprintf(stderr, "无法创建事件循环\n");
        exit(1);
    }

    printf("使用后端: 0x%x\n", ev_backend(main_loop));

    // 在同一循环中设置两个不同间隔的定时器
    ev_timer_init(&timer1, timer1_cb, 1.0, 1.0);
    ev_timer_start(main_loop, &timer1);

    ev_timer_init(&timer2, timer2_cb, 1.5, 1.5);
    ev_timer_start(main_loop, &timer2);

    // 创建一个停止定时器
    ev_timer_init(&stop_timer, stop_cb, 5.0, 0.0);
    ev_timer_start(main_loop, &stop_timer);

    printf("定时器1每1秒触发，定时器2每1.5秒触发\n");
    printf("运行5秒后退出系统\n\n");

    ev_run(main_loop, 0);

    // 清理
    ev_timer_stop(main_loop, &timer1);
    ev_timer_stop(main_loop, &timer2);
    ev_timer_stop(main_loop, &stop_timer);

    printf("\n程序结束\n");
    printf("定时器1事件总数: %d\n", timer1_events);
    printf("定时器2事件总数: %d\n", timer2_events);

    return 0;
}