#include <ev.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

struct ev_io io_watcher;
struct ev_timer timer_watcher;

static void io_cb(EV_P_ struct ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf(">>> 你的输入: %s", buf);
    }
}

static void timer_cb(EV_P_ struct ev_timer *w, int revents) {
    time_t now = time(NULL);
    printf("[定时器] 当前时间: %s", ctime(&now));
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    // 设置 IO watcher
    ev_io_init(&io_watcher, io_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &io_watcher);

    // 设置定时器
    ev_timer_init(&timer_watcher, timer_cb, 2.0, 2.0);  // 2秒后开始，之后每2秒触发
    ev_timer_start(loop, &timer_watcher);

    printf("程序启动（按 Ctrl+C 退出）：\n");
    ev_run(loop, 0);

    // 清理
    ev_io_stop(loop, &io_watcher);
    ev_timer_stop(loop, &timer_watcher);

    return 0;
}
