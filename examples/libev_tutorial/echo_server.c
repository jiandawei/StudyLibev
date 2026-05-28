#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 10

int listen_fd;
struct ev_io accept_watcher;
struct ev_io *client_watchers[MAX_CLIENTS] = {0};

static void client_cb(EV_P_ struct ev_io *w, int revents) {
    char buf[1024];
    int n = read(w->fd, buf, sizeof(buf));

    if (n <= 0) {
        printf("客户端断开连接\n");
        ev_io_stop(loop, w);
        close(w->fd);
        free(w);
        return;
    }

    printf("收到数据 (%d 字节): %.*s\n", n, n, buf);

    // Echo 回去
    write(w->fd, buf, n);
}

static void accept_cb(EV_P_ struct ev_io *w, int revents) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(w->fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd < 0) {
        perror("accept");
        return;
    }

    printf("新客户端连接: %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // 创建客户端 watcher
    struct ev_io *client_watcher = malloc(sizeof(struct ev_io));
    ev_io_init(client_watcher, client_cb, client_fd, EV_READ);
    ev_io_start(loop, client_watcher);

    // 保存到数组
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_watchers[i] == NULL) {
            client_watchers[i] = client_watcher;
            break;
        }
    }
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);
    struct sockaddr_in addr;

    // 创建监听 socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Echo 服务器启动，监听端口 %d\n", PORT);

    // 设置 accept watcher
    ev_io_init(&accept_watcher, accept_cb, listen_fd, EV_READ);
    ev_io_start(loop, &accept_watcher);

    // 运行事件循环
    ev_run(loop, 0);

    // 清理
    printf("服务器关闭\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_watchers[i]) {
            ev_io_stop(loop, client_watchers[i]);
            close(client_watchers[i]->fd);
            free(client_watchers[i]);
        }
    }
    close(listen_fd);

    return 0;
}
