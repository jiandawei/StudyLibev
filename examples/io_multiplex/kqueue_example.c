/*
 * kqueue 示例程序 - 重构版本
 * 使用公共函数，专注于 kqueue 特有的逻辑
 */

#include "io_common.h"
#include <sys/event.h>

#define MAX_EVENTS 1024
#define SERVER_PORT 8891
#define KQUEUE_TIMEOUT_SEC 1

// 添加 fd 到 kqueue
static int kqueue_add_fd(int kq, int fd) {
    struct kevent change;
    
    EV_SET(&change, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    
    if (kevent(kq, &change, 1, NULL, 0, NULL) < 0) {
        perror("kevent(ADD)");
        return -1;
    }
    
    return 0;
}

// 从 kqueue 移除 fd
static int kqueue_del_fd(int kq, int fd) {
    struct kevent change;
    
    EV_SET(&change, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    
    if (kevent(kq, &change, 1, NULL, 0, NULL) < 0) {
        perror("kevent(DEL)");
        return -1;
    }
    
    return 0;
}

// 处理新的 kqueue 连接
static void handle_new_connection(int kq, int server_fd) {
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
        
        // 添加到 kqueue
        kqueue_add_fd(kq, client_fd);
        
        printf("新客户端连接: %s (fd=%d)\n",
               io_get_client_addr_str(&client_addr),
               client_fd);
        
        // 发送欢迎消息
        io_send_welcome_message(client_fd);
    }
}

// 处理客户端数据（kqueue 特定版本）
static void handle_client_data(int kq, int client_fd) {
    int ret;
    
    // 使用公共函数处理数据
    ret = io_handle_client_data(client_fd);
    
    if (ret < 0) {
        printf("客户端 (fd=%d) 关闭连接\n", client_fd);
        kqueue_del_fd(kq, client_fd);
        io_close_socket(client_fd);
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    int kq;
    struct kevent events[MAX_EVENTS];
    struct timespec timeout;
    int nev, i;
    int running = 1;
    
    // 创建 kqueue 实例
    kq = kqueue();
    if (kq < 0) {
        perror("kqueue");
        return EXIT_FAILURE;
    }
    
    // 创建服务器 socket
    server_fd = io_create_server_socket(SERVER_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "创建服务器失败\n");
        close(kq);
        return EXIT_FAILURE;
    }
    
    // 添加服务器 fd 到 kqueue
    kqueue_add_fd(kq, server_fd);
    
    // 设置超时
    timeout.tv_sec = KQUEUE_TIMEOUT_SEC;
    timeout.tv_nsec = 0;
    
    printf("\nkqueue 循环运行中，按 Ctrl+C 退出...\n");
    
    while (running) {
        // 等待事件（kqueue 特定：事件过滤机制）
        nev = kevent(kq, NULL, 0, events, MAX_EVENTS, &timeout);
        
        if (nev < 0) {
            if (errno == EINTR) {
                continue;  // 被信号中断，继续
            }
            perror("kevent");
            break;
        } else if (nev == 0) {
            // 超时
            continue;
        }
        
        // 处理事件（kqueue 特定：事件过滤器）
        for (i = 0; i < nev; i++) {
            int fd = events[i].ident;
            short filter = events[i].filter;
            u_short flags = events[i].flags;
            int64_t data = events[i].data;
            
            printf("Event: fd=%d, filter=%d, flags=0x%x, data=%lld\n",
                   fd, filter, flags, (long long)data);
            
            // 检查错误或关闭
            if (flags & (EV_ERROR | EV_EOF)) {
                printf("fd=%d 连接关闭或错误 (flags=0x%x)\n", fd, flags);
                if (fd != server_fd) {
                    kqueue_del_fd(kq, fd);
                    io_close_socket(fd);
                }
                continue;
            }
            
            // 读事件
            if (filter == EVFILT_READ) {
                if (fd == server_fd) {
                    // 处理新连接
                    handle_new_connection(kq, server_fd);
                } else {
                    // 处理客户端数据
                    handle_client_data(kq, fd);
                }
            }
            
            // 写事件
            if (filter == EVFILT_WRITE) {
                // 可以处理写事件（kqueue 特定：可以监控多种事件）
            }
            
            // 事件数据（kqueue 特有：返回可读/可写字节数）
            if (data > 0) {
                printf("fd=%d 可读字节数: %lld\n", fd, (long long)data);
            }
        }
    }
    
    // 清理
    printf("\n清理资源...\n");
    kqueue_del_fd(kq, server_fd);
    io_close_socket(server_fd);
    close(kq);
    
    printf("kqueue 服务器关闭\n");
    return EXIT_SUCCESS;
}