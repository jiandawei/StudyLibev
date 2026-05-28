/*
 * Select 示例程序 - 重构版本
 * 使用公共函数，专注于 Select 特有的逻辑
 */

#include "io_common.h"
#include <sys/select.h>
#include <sys/time.h>

#define MAX_FDS 1024

#define SERVER_PORT 8888
#define SELECT_TIMEOUT_SEC 1

// fd 信息
typedef struct {
    int fd;
    int is_server;
} fd_info_t;

static fd_info_t fd_info[MAX_FDS];

// 添加 fd 到 fd_info
static int add_fd_info(int fd, int is_server) {
    if (fd < 0 || fd >= MAX_FDS) {
        return -1;
    }
    
    fd_info[fd].fd = fd;
    fd_info[fd].is_server = is_server;
    
    return 0;
}

// 获取最大的 fd
static int get_max_fd(void) {
    int max_fd = 0;
    int i;
    
    for (i = 0; i < MAX_FDS; i++) {
        if (fd_info[i].fd > max_fd) {
            max_fd = fd_info[i].fd;
        }
    }
    
    return max_fd;
}

// 处理新的 Select 连接
static void handle_new_connection(int server_fd, fd_set *readfds) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // 没有更多连接了
            } else {
                perror("accept");
                break;
            }
        }
        
        // 设置非阻塞
        if (io_set_nonblocking(client_fd) < 0) {
            close(client_fd);
            continue;
        }
        
        // 添加到 fd_info
        add_fd_info(client_fd, 0);
        
        // 关键：将新的 client_fd 添加到 readfds 中！
        FD_SET(client_fd, readfds);
        
        printf("新客户端连接: %s (fd=%d)\n",
               io_get_client_addr_str(&client_addr),
               client_fd);
        
        // 发送欢迎消息
        io_send_welcome_message(client_fd);
    }
}

// 处理客户端数据（Select 特定版本）
static void handle_client_data(int client_fd, fd_set *readfds) {
    int ret;
    
    // 使用公共函数处理数据
    ret = io_handle_client_data(client_fd);
    
    if (ret < 0) {
        printf("客户端 (fd=%d) 关闭连接\n", client_fd);
        io_close_socket(client_fd);
        fd_info[client_fd].fd = -1;
        FD_CLR(client_fd, readfds);
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    int max_fd;
    fd_set readfds;
    fd_set tempfds;
    int ret, i;
    struct timeval tv;
    int running = 1;
    
    // 初始化 fd_info
    memset(fd_info, -1, sizeof(fd_info));
    
    // 创建服务器 socket
    server_fd = io_create_server_socket(SERVER_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "创建服务器失败\n");
        return EXIT_FAILURE;
    }
    
    // 添加服务器 fd
    add_fd_info(server_fd, 1);
    
    // 初始化 fd_set
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    
    printf("\nSelect 循环运行中，按 Ctrl+C 退出...\n");
    
    while (running) {
        // 保存原始的 readfds
        tempfds = readfds;
        
        // 设置超时
        tv.tv_sec = SELECT_TIMEOUT_SEC;
        tv.tv_usec = 0;
        
        // 获取最大 fd
        max_fd = get_max_fd();
        
        // 等待事件
        ret = select(max_fd + 1, &tempfds, NULL, NULL, &tv);
        
        if (ret < 0) {
            if (errno == EINTR) {
                continue;  // 被信号中断，继续
            }
            perror("select");
            break;
        } else if (ret == 0) {
            // 超时
            continue;
        }
        
        // 检查哪个 fd 就绪了
        for (i = 0; i <= max_fd; i++) {
            if (fd_info[i].fd < 0) {
                continue;
            }
            
            if (FD_ISSET(fd_info[i].fd, &tempfds)) {
                if (fd_info[i].is_server) {
                    // 处理新连接（Select 特定：需要更新 readfds）
                    handle_new_connection(fd_info[i].fd, &readfds);
                } else {
                    // 处理客户端数据
                    handle_client_data(fd_info[i].fd, &readfds);
                }
            }
        }
    }
    
    // 清理
    printf("\n清理资源...\n");
    for (i = 0; i < MAX_FDS; i++) {
        if (fd_info[i].fd >= 0) {
            io_close_socket(fd_info[i].fd);
        }
    }
    
    printf("Select 服务器关闭\n");
    return EXIT_SUCCESS;
}