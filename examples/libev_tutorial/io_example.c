#include <ev.h>
#include <stdio.h>
#include <unistd.h>

struct ev_io io_watcher;

static void io_cb(EV_P_ struct ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("收到输入: %s", buf);
    } else {
        ev_io_stop(EV_A_ w);
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);

    ev_io_init(&io_watcher, io_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &io_watcher);

    printf("请输入内容（Ctrl+D 退出）：\n");
    ev_run(loop, 0);

    return 0;
}
