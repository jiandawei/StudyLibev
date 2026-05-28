/*
 * Epoll 示例程序 - 重构版本
 * 使用公共函数，专注于 Epoll 特有的逻辑
 */

#include "io_common.h"
#include <sys/epoll.h>

#define MAX_EVENTS 1024
#define SERVER_PORT 8890
#define EPOLL_TIMEOUT 1000  // 1000ms = 1s

// 添加 fd 到 epoll
static int epoll_add_fd(int epfd, int fd, uint32_t events) {
    struct epoll_event ev;
    
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_ctl(ADD)");
        return -1;
    }
    
    return 0;
}

// 从 epoll 移除 fd
static int epoll_del_fd(int epfd, int fd) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
        perror("epoll_ctl(DEL)");
        return -1;
    }
    return 0;
}

// 修改 fd 的 epoll 事件
static int epoll_mod_fd(int epfd, int fd, uint32_t events) {
    struct epoll_event ev;
    
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        perror("epoll_ctl(MOD)");
        return -1;
    }
    
    return 0;
}

// 处理新的 Epoll 连接
static void handle_new_connection(int epfd, int server_fd) {
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
        
        // 添加到 epoll（Epoll 特定：边缘触发）
        epoll_add_fd(epfd, client_fd, EPOLLIN | EPOLLET);
        
        printf("新客户端连接: %s (fd=%d)\n",
               io_get_client_addr_str(&client_addr),
               client_fd);
        
        // 发送欢迎消息
        io_send_welcome_message(client_fd);
    }
}

// 处理客户端数据（Epoll 特定版本）
static void handle_client_data(int epfd, int client_fd) {
    int ret;
    
    // 使用公共函数处理数据
    ret = io_handle_client_data(client_fd);
    
    if (ret < 0) {
        printf("客户端 (fd=%d) 关闭连接\n", client_fd);
        epoll_del_fd(epfd, client_fd);
        io_close_socket(client_fd);
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    int epfd;
    struct epoll_event events[MAX_EVENTS];
    int nfds, i;
    int running = 1;
    
    // 创建 epoll 实例
    epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        return EXIT_FAILURE;
    }
    
    // 创建服务器 socket
    server_fd = io_create_server_socket(SERVER_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "创建服务器失败\n");
        close(epfd);
        return EXIT_FAILURE;
    }
    
    // 添加服务器 fd 到 epoll
    epoll_add_fd(epfd, server_fd, EPOLLIN | EPOLLET);
    
    printf("\nEpoll 循环运行中，按 Ctrl+C 退出...\n");
    
    while (running) {
        // 等待事件（Epoll 特定：批量获取就绪事件）
        nfds = epoll_wait(epfd, events, MAX_EVENTS, EPOLL_TIMEOUT);
        
        if (nfds < 0) {
            if (errno == EINTR) {
                continue;  // 被信号中断，继续
            }
            perror("epoll_wait");
            break;
        } else if (nfds == 0) {
            // 超时
            continue;
        }
        
        // 处理就绪事件（Epoll 特定：只遍历就绪的 fd）
        for (i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            uint32_t evs = events[i].events;
            
            // 检查错误
            if (evs & (EPOLLERR | EPOLLHUP)) {
                printf("fd=%d 发生错误 (events=0x%x)\n", fd, evs);
                if (fd != server_fd) {
                    epoll_del_fd(epfd, fd);
                    io_close_socket(fd);
                }
                continue;
            }
            
            // 检查读事件
            if (evs & EPOLLIN) {
                if (fd == server_fd) {
                    // 处理新连接
                    handle_new_connection(epfd, server_fd);
                } else {
                    // 处理客户端数据
                    handle_client_data(epfd, fd);
                }
            }
            
            // 检查写事件
            if (evs & EPOLLOUT) {
                // 可以处理写事件（Epoll 特定：水平/边缘触发）
            }
            
            // 显示事件信息（Epoll 特有）
            if (evs & ~(EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)) {
                printf("fd=%d 其他事件: 0x%x\n", fd, evs);
            }
        }
    }
    
    // 清理
    printf("\n清理资源...\n");
    epoll_del_fd(epfd, server_fd);
    io_close_socket(server_fd);
    close(epfd);
    
    printf("Epoll 服务器关闭\n");
    return EXIT_SUCCESS;
}