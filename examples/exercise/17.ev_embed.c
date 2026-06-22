#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct ev_loop *outer_loop;
struct ev_loop *inner_loop;
struct ev_embed embed_watcher;
struct ev_timer inner_timer;
struct ev_timer outer_timer;

static void inner_timer_cb(EV_P_ ev_timer *w, int revents) {
    static int count = 0;
    printf("内部循环定时器触发 (count=%d)\n", ++count);
    if (count >= 5) {
        printf("内部循环任务完成，停止内部循环\n");
        ev_break(EV_A_ EVBREAK_ONE);
    }
}

static void outer_timer_cb(EV_P_ ev_timer *w, int revents) {
    static int count = 0;
    printf("外部循环定时器触发 (count=%d)\n", ++count);
    if (count >= 10) {
        printf("外部循环任务完成，退出\n");
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

int main() {
    unsigned int supported = ev_supported_backends();
    unsigned int embeddable = ev_embeddable_backends();

    printf("支持的后端: 0x%08x\n", supported);
    printf("可嵌入的后端: 0x%08x\n", embeddable);

    outer_loop = ev_default_loop(EVBACKEND_KQUEUE);
    if (!outer_loop) outer_loop = ev_default_loop(EVBACKEND_EPOLL);
    if (!outer_loop) outer_loop = ev_default_loop(0);
    printf("外部循环后端: 0x%08x\n", ev_backend(outer_loop));

    inner_loop = ev_loop_new(EVBACKEND_SELECT);

    if (!inner_loop) {
        inner_loop = ev_loop_new(EVBACKEND_POLL);
    }

    if (!inner_loop) {
        fprintf(stderr, "无法创建内部事件循环\n");
        return 1;
    }
    printf("内部循环后端: 0x%08x\n", ev_backend(inner_loop));

    ev_timer_init(&inner_timer, inner_timer_cb, 0.5, 1.0);
    ev_timer_start(inner_loop, &inner_timer);

    ev_embed_init(&embed_watcher, NULL, inner_loop);
    ev_embed_start(outer_loop, &embed_watcher);

    ev_timer_init(&outer_timer, outer_timer_cb, 1.0, 1.0);
    ev_timer_start(outer_loop, &outer_timer);

    printf("外部循环中嵌入了内部循环\n");
    printf("两个循环的定时器将同时运行\n");

    ev_run(outer_loop, 0);

    ev_embed_stop(outer_loop, &embed_watcher);
    ev_loop_destroy(inner_loop);

    return 0;
}