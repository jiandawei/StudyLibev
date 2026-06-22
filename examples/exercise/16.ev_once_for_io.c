#include <ev.h>
#include <stdio.h>
#include <unistd.h>

static void once_cb(int revents, void *arg) {
    char buf[1024];
    if (revents & EV_READ) {
        int n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("ev_once 读取到: %s", buf);
        } else if (n == 0) {
            printf("EOF\n");
        }
    } else if (revents & EV_TIMER) {
        printf("等待超时！\n");
    }
    printf("ev_once 回调执行完毕，watcher 自动停止\n");
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);

    printf("等待标准输入（输入一行后自动停止监控）...\n");
    ev_once(loop, STDIN_FILENO, EV_READ, 10.0, once_cb, NULL);

    ev_run(loop, 0);
    printf("事件循环退出\n");
    return 0;
}