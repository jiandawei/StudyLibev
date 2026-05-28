/*
 * IO 多路复用机制的公共函数实现
 */

#include "io_common.h"

// 设置文件描述符为非阻塞模式
int io_set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

// 创建并配置一个 TCP 监听 socket
int io_create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in addr;
    int opt = 1;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }
    
    // 设置地址重用
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(server_fd);
        return -1;
    }
    
    // 设置非阻塞
    if (io_set_nonblocking(server_fd) < 0) {
        close(server_fd);
        return -1;
    }
    
    // 绑定地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }
    
    // 开始监听
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }
    
    printf("服务器启动，监听端口 %d\n", port);
    printf("在另一个终端连接: nc localhost %d\n", port);
    
    return server_fd;
}

// 获取客户端的地址字符串（静态缓冲区，线程不安全）
const char *io_get_client_addr_str(struct sockaddr_in *addr) {
    static char addr_str[128];
    snprintf(addr_str, sizeof(addr_str), "%s:%d",
             inet_ntoa(addr->sin_addr),
             ntohs(addr->sin_port));
    return addr_str;
}

// 发送欢迎消息给客户端
void io_send_welcome_message(int fd) {
    const char *welcome = "欢迎使用 IO 服务器！\n";
    send(fd, welcome, strlen(welcome), 0);
}

// 处理客户端数据（Echo 服务）
// 处理所有可读数据直到 EAGAIN
// 返回：0 表示正常，-1 表示连接关闭或错误
int io_handle_client_data(int fd) {
    char buffer[BUFFER_SIZE];
    ssize_t n;
    
    while (1) {
        n = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有更多数据了，这是正常的（非阻塞模式）
                return 0;
            } else {
                perror("recv");
                return -1;
            }
        } else if (n == 0) {
            // 客户端关闭连接
            return -1;
        }
        
        buffer[n] = '\0';
        printf("收到数据 (fd=%d, %zd 字节): %s", fd, n, buffer);
        
        // Echo 回去
        send(fd, buffer, n, 0);
    }
}

// 优雅关闭 socket
void io_close_socket(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}