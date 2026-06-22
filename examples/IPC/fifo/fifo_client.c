#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_PATH "/tmp/my_fifo"
#define BUF_SIZE 1024

int main() {
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0) {
        if (errno == ENXIO) {
            fprintf(stderr, "服务端未启动，请先运行 fifo_server\n");
        } else {
            perror("open fifo");
        }
        return 1;
    }
    printf("已连接到命名管道: %s\n", FIFO_PATH);
    printf("输入消息并回车发送（输入 exit 退出）\n\n");

    char buf[BUF_SIZE];
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (strcmp(buf, "exit\n") == 0) {
            printf("客户端退出\n");
            break;
        }

        ssize_t n = write(fifo_fd, buf, strlen(buf));
        if (n < 0) {
            perror("write fifo");
            break;
        }
        printf("已发送 %zd 字节\n", n);
    }

    close(fifo_fd);
    return 0;
}
