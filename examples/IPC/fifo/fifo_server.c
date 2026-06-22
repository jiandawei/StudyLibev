#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#define FIFO_PATH "/tmp/my_fifo"
#define BUF_SIZE 1024

static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

int main() {
    signal(SIGINT, sigint_handler);

    if (access(FIFO_PATH, F_OK) == 0) {
        unlink(FIFO_PATH);
    }

    if (mkfifo(FIFO_PATH, 0666) < 0) {
        perror("mkfifo");
        return 1;
    }
    printf("命名管道已创建: %s\n", FIFO_PATH);

    printf("等待客户端连接...\n");
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("open fifo");
        unlink(FIFO_PATH);
        return 1;
    }
    printf("客户端已连接，开始接收消息（Ctrl+C 退出）\n\n");

    char buf[BUF_SIZE];
    while (running) {
        ssize_t n = read(fifo_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("收到消息: %s", buf);
        } else if (n == 0) {
            printf("客户端已断开，等待重新连接...\n");
            close(fifo_fd);
            fifo_fd = open(FIFO_PATH, O_RDONLY);
            if (fifo_fd < 0) {
                perror("open fifo");
                break;
            }
            printf("客户端已重新连接\n");
        } else {
            if (errno != EINTR) {
                perror("read fifo");
                break;
            }
        }
    }

    close(fifo_fd);
    unlink(FIFO_PATH);
    printf("\n服务端已退出，管道已删除\n");
    return 0;
}
