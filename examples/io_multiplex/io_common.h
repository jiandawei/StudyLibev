/*
 * IO 多路复用机制的公共函数和头文件
 * 为 select、poll、epoll、kqueue 示例提供通用功能
 */

#ifndef IO_COMMON_H
#define IO_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 1024

// 设置文件描述符为非阻塞模式
int io_set_nonblocking(int fd);

// 创建并配置一个 TCP 监听 socket
int io_create_server_socket(int port);

// 获取客户端的地址字符串
const char *io_get_client_addr_str(struct sockaddr_in *addr);

// Send welcome message to client
void io_send_welcome_message(int fd);

// 处理客户端数据（Echo 服务）
// 处理所有可读数据直到 EAGAIN
int io_handle_client_data(int fd);

// 优雅关闭 socket
void io_close_socket(int fd);

#endif /* IO_COMMON_H */