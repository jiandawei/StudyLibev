#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct ev_loop *main_loop;
struct ev_loop *embed_loop;

struct ev_timer main_timer;
struct ev_timer embed_timer;
struct ev_embed embed_watcher;

int main_events = 0;
int embed_events = 0;

static void main_timer_cb(EV_P_ struct ev_timer *w, int revents) {
    main_events++;
    printf("主循环定时器 (事件: %d)\n", main_events);
    
    if (main_events >= 5) {
        ev_timer_stop(EV_A_ w);
        ev_embed_stop(EV_A_ &embed_watcher);
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

static void embed_timer_cb(EV_P_ struct ev_timer *w, int revents) {
    embed_events++;
    printf("嵌入循环定时器 (事件: %d)\n", embed_events);
}

int main(void) {
    printf("Embed watcher 示例\n");
    printf("==================\n\n");

    // 创建主循环和嵌入循环
    main_loop = ev_default_loop(0);
    embed_loop = ev_loop_new(EVFLAG_AUTO);

    if (!main_loop || !embed_loop) {
        fprintf(stderr, "无法创建事件循环\n");
        exit(1);
    }

    // 在嵌入循环中设置定时器
    ev_timer_init(&embed_timer, embed_timer_cb, 1.5, 1.5);
    ev_timer_start(embed_loop, &embed_timer);

    // 在主循环中设置定时器
    ev_timer_init(&main_timer, main_timer_cb, 1.0, 1.0);
    ev_timer_start(main_loop, &main_timer);

    // 设置嵌入 watcher
    ev_embed_init(&embed_watcher, embed_loop);
    ev_embed_start(main_loop, &embed_watcher);

    printf("启动主事件循环 (包含嵌入循环)\n");
    printf("主定时器每1秒触发，嵌入定时器每1.5秒触发\n");
    printf("运行5个主事件后退出\n\n");

    ev_run(main_loop, 0);

    // 清理
    ev_loop_destroy(embed_loop);

    printf("\n程序结束\n");
    printf("主事件总数: %d\n", main_events);
    printf("嵌入事件总数: %d\n", embed_events);

    return 0;
}