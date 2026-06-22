#include <ev.h>
#include <stdio.h>
#include <stdlib.h>

// 全局 cleanup 监听器
struct ev_cleanup cleanup_watcher;

// 清理回调函数
static void cleanup_cb(EV_P_ struct ev_cleanup *w, int revents) {
    printf("Cleanup: 事件循环即将退出\n");
    printf("正在清理资源...\n");
}

// 定时器必须要有回调！否则会段错误
static void timer_cb(EV_P_ struct ev_timer *w, int revents) {
    printf("定时器触发，停止事件循环\n");
    ev_break(loop, EVBREAK_ALL); // 主动退出事件循环
}

int main(void) {
    struct ev_loop *loop = EV_DEFAULT; // 等价 ev_default_loop(0)

    // 初始化并启动 cleanup
    ev_cleanup_init(&cleanup_watcher, cleanup_cb);
    ev_cleanup_start(loop, &cleanup_watcher);

    printf("Cleanup watcher 启动\n");
    printf("事件循环将在2秒后退出\n");

    // 定时器：必须给合法回调！
    struct ev_timer timer;
    ev_timer_init(&timer, timer_cb, 2.0, 0.0); // 修复：传入 timer_cb
    ev_timer_start(loop, &timer);

    // 运行事件循环
    ev_run(loop, 0);

    printf("程序已安全退出\n");
    return 0;
}