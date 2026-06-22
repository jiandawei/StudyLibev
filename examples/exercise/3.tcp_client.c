#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

struct ev_io conn_watcher;
int sockfd;

static void conn_cb(EV_P_ ev_io *w, int revents) {
    char buf[1024];
    int n;
    
    if (revents & EV_READ) {
        n = read(sockfd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            printf("服务器响应: %s", buf);
        } else {
            printf("连接关闭\n");
            ev_io_stop(EV_A_ w);
            ev_break(EV_A_ EVBREAK_ALL);
        }
    }
}

int main() {
    struct sockaddr_in addr;
    struct ev_loop *loop = ev_default_loop(0);
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    
    ev_io_init(&conn_watcher, conn_cb, sockfd, EV_READ);
    ev_io_start(loop, &conn_watcher);
    
    write(sockfd, "Hello Server!\n", 14);
    ev_run(loop, 0);
    
    close(sockfd);
    return 0;
}