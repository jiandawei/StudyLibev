/*
 * Poll 示例程序 - 重构版本
 * 使用公共函数，专注于 Poll 特有的逻辑
 */

#include "io_common.h"
#include <poll.h>

#define MAX_FDS 1024
#define SERVER_PORT 8889
#define POLL_TIMEOUT 1000  // 1000ms = 1s

// FD 信息
typedef struct {
    int fd;
    int is_server;
} fd_info_t;

static struct pollfd poll_fds[MAX_FDS];
static fd_info_t fd_info[MAX_FDS];
static int poll_fd_count = 0;

// 添加 fd 到 poll_fds 数组
static int add_poll_fd(int fd, short events, int is_server) {
    if (poll_fd_count >= MAX_FDS) {
        return -1;
    }
    
    poll_fds[poll_fd_count].fd = fd;
    poll_fds[poll_fd_count].events = events;
    poll_fds[poll_fd_count].revents = 0;
    
    fd_info[poll_fd_count].fd = fd;
    fd_info[poll_fd_count].is_server = is_server;
    
    poll_fd_count++;
    return 0;
}

// 从 poll_fds 移除 fd
static void remove_poll_fd(int fd) {
    int i;
    
    for (i = 0; i < poll_fd_count; i++) {
        if (poll_fds[i].fd == fd) {
            // 用最后一个元素覆盖当前元素
            poll_fds[i] = poll_fds[poll_fd_count - 1];
            fd_info[i] = fd_info[poll_fd_count - 1];
            poll_fd_count--;
            return;
        }
    }
}

// 处理新的连接（Poll 特定版本）
static void handle_new_connection(int server_fd) {
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
        
        // 添加到 poll_fds（Poll 特定：直接管理数组）
        add_poll_fd(client_fd, POLLIN, 0);
        
        printf("新客户端连接: %s (fd=%d)\n",
               io_get_client_addr_str(&client_addr),
               client_fd);
        
        // 发送欢迎消息
        io_send_welcome_message(client_fd);
    }
}

// 处理客户端数据（Poll 特定版本）
static void handle_client_data(int client_fd) {
    int ret;
    
    // 使用公共函数处理数据
    ret = io_handle_client_data(client_fd);
    
    if (ret < 0) {
        printf("客户端 (fd=%d) 关闭连接\n", client_fd);
        io_close_socket(client_fd);
        remove_poll_fd(client_fd);
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    int ret, i;
    int running = 1;
    
    // 初始化
    memset(poll_fds, 0, sizeof(poll_fds));
    memset(fd_info, 0, sizeof(fd_info));
    
    // 创建服务器 socket
    server_fd = io_create_server_socket(SERVER_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "创建服务器失败\n");
        return EXIT_FAILURE;
    }
    
    // 添加服务器 fd
    add_poll_fd(server_fd, POLLIN, 1);
    
    printf("\nPoll 循环运行中，按 Ctrl+C 退出...\n");
    
    while (running) {
        // 等待事件（Poll 特定：一次性获取所有就绪 fd）
        ret = poll(poll_fds, poll_fd_count, POLL_TIMEOUT);
        
        if (ret < 0) {
            if (errno == EINTR) {
                continue;  // 被信号中断，继续
            }
            perror("poll");
            break;
        } else if (ret == 0) {
            // 超时
            continue;
        }
        
        // 处理就绪事件（Poll 特定：遍历数组）
        for (i = 0; i < poll_fd_count; i++) {
            if (poll_fds[i].revents == 0) {
                continue;
            }
            
            // 检查错误
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                printf("fd=%d 发生错误 (revents=0x%x)\n",
                       poll_fds[i].fd, poll_fds[i].revents);
                if (!fd_info[i].is_server) {
                    io_close_socket(poll_fds[i].fd);
                    remove_poll_fd(poll_fds[i].fd);
                }
                continue;
            }
            
            // 检查可读事件
            if (poll_fds[i].revents & POLLIN) {
                if (fd_info[i].is_server) {
                    // 处理新连接
                    handle_new_connection(poll_fds[i].fd);
                } else {
                    // 处理客户端数据
                    handle_client_data(poll_fds[i].fd);
                }
            }
            
            // 检查可写事件（Poll 特定：可以监控写事件）
            if (poll_fds[i].revents & POLLOUT) {
                // 可以处理写事件
            }
        }
    }
    
    // 清理
    printf("\n清理资源...\n");
    for (i = 0; i < poll_fd_count; i++) {
        io_close_socket(poll_fds[i].fd);
    }
    
    printf("Poll 服务器关闭\n");
    return EXIT_SUCCESS;
}