// libev 嵌入循环示例程序
// 演示如何使用 ev_embed 将一个事件循环嵌入到另一个事件循环中
// 本示例展示了 embed 的自动模式：embed_cb 回调会被自动调用
//
// 重要说明：embed_cb 何时被调用？
//
// embed_cb 会在以下条件下被自动触发：
// 1. 子循环中有实际的 I/O 事件（如 socket 可读/可写）
// 2. 这些 I/O 事件导致子循环的 backend_fd 变成可读状态
// 3. 主循环检测到子循环的 backend_fd 可读
// 4. 主循环的 io 观察器触发，进而调用 embed_cb
//
// 注意：仅有定时器事件（ev_timer）通常不足以触发 embed_cb，
// 因为定时器不涉及真正的 I/O 操作，不会让 backend_fd 变成可读。
// 只有当子循环中有真正的 I/O 事件时，embed_cb 才会被调用。
//
// 这是为什么之前 embed_cb 没有被调用的原因：
// - 只有定时器，没有真正的 I/O 事件
// - 子循环的 backend_fd 不会变成可读
// - 主循环的 io 观察器不会被触发
// - embed_cb 也就不会被调用
//
// 本示例通过添加一个管道（socketpair）创建真正的 I/O 事件，
// 从而让 embed_cb 能够被正确触发。

#include <ev.h>          // libev 核心头文件
#include <stdio.h>       // 标准输入输出
#include <stdlib.h>      // 标准库函数
#include <string.h>      // 字符串操作
#include <unistd.h>      // POSIX 系统 API
#include <sys/socket.h>  // socket 相关
#include <netinet/in.h>  // 网络地址结构

// 主事件循环：作为父循环，运行整个程序
struct ev_loop *main_loop;
// 子事件循环：将被嵌入到主循环中
struct ev_loop *embedded_loop;

// 嵌入观察器：用于将子循环嵌入到主循环
struct ev_embed embed_watcher;

// 用于触发 embed_cb 的 socket 对
int trigger_pipe[2];

// 定时器：在子循环中运行
struct ev_timer child_timer;
// 定时器：在主循环中运行
struct ev_timer main_timer;
// 定时器：用于停止整个程序
struct ev_timer stop_timer;

// io 观察器：用于在子循环中读取管道
struct ev_io pipe_reader;

// 统计变量：记录各定时器触发的次数
int child_events = 0;   // 子循环定时器触发次数
int main_events = 0;    // 主循环定时器触发次数
int embed_cb_calls = 0; // embed_cb 回调调用次数
int pipe_read_count = 0;// 管道读取次数

/**
 * 子循环定时器回调函数
 * @param EV_P_ 事件循环参数（libev 宏，展开为 struct ev_loop *loop）
 * @param w 触发此回调的定时器观察器
 * @param revents 事件类型掩码
 */
static void child_timer_cb(EV_P_ struct ev_timer *w, int revents) {
    child_events++;
    printf("  [子循环] 定时器触发 (事件: %d)\n", child_events);
}

/**
 * 主循环定时器回调函数
 * @param EV_P_ 事件循环参数
 * @param w 触发此回调的定时器观察器
 * @param revents 事件类型掩码
 *
 * 说明：在有 embed_cb 的情况下，不需要手动调用 ev_embed_sweep
 * 子循环会通过 backend_fd 通信自动触发主循环的 embed_cb
 */
static void main_timer_cb(EV_P_ struct ev_timer *w, int revents) {
    main_events++;
    printf("[主循环] 定时器触发 (事件: %d)\n", main_events);

    // 定期向管道写入数据，触发子循环的 io 事件
    // 这会让子循环的 backend_fd 变成可读，进而触发 embed_cb
    if (embedded_loop != main_loop && trigger_pipe[0] >= 0) {
        const char *msg = "trigger embed";
        write(trigger_pipe[1], msg, strlen(msg));
    }

    // 说明：在设置了 embed_cb 的情况下，不需要手动调用 ev_embed_sweep
    // 子循环会通过 I/O 多路复用机制自动通知主循环需要处理事件
    // embed_cb 会被自动调用，无需下面的代码：
    //
    // if (embedded_loop && embedded_loop != main_loop) {
    //     ev_embed_sweep(main_loop, &embed_watcher);
    // }
}

/**
 * 管道读取回调函数（在子循环中）
 * @param EV_P_ 事件循环参数
 * @param w io 观察器
 * @param revents 事件类型掩码
 *
 * 当管道中有数据时，此函数会被调用
 * 这会创建真正的 I/O 事件，触发 embed_cb
 */
static void pipe_reader_cb(EV_P_ ev_io *w, int revents) {
    char buffer[128];
    ssize_t bytes = read(w->fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        pipe_read_count++;
        printf("  [子循环] 读取管道数据: '%s' (读取: %d)\n", buffer, pipe_read_count);
    }
}

/**
 * 嵌入观察器回调函数
 * @param EV_P_ 事件循环参数
 * @param w 嵌入观察器
 * @param revents 事件类型掩码（总是 EV_EMBED）
 *
 * 当子循环需要处理事件时，此回调会被自动触发
 * 触发机制：
 * 1. 子循环有事件需要处理时，其 backend_fd 会变成可读
 * 2. 主循环的 io 观察器检测到 backend_fd 可读
 * 3. 触发 embed_io_cb，进而调用这里的 embed_cb
 *
 * 重要：在使用回调模式时，必须手动调用 ev_embed_sweep！
 */
static void embed_cb(EV_P_ ev_embed *w, int revents) {
    embed_cb_calls++;
    printf("[自动] 子循环需要处理事件 (调用: %d, revents: 0x%x)\n",
           embed_cb_calls, revents);

    // 在 embed_cb 中处理子循环事件
    // 当设置了回调时，必须手动调用 ev_embed_sweep
    ev_embed_sweep(EV_A_ w);
}

/**
 * 停止程序的回调函数
 * @param EV_P_ 事件循环参数
 * @param w 触发此回调的定时器观察器
 * @param revents 事件类型掩码
 *
 * ev_break 会中断事件循环的执行
 */
static void stop_cb(EV_P_ struct ev_timer *w, int revents) {
    ev_break(EV_A_ EVBREAK_ALL);  // 中断所有事件循环
}

/// 主函数
/// @return 程序退出状态码
int main(void) {
    printf("libev 嵌入循环示例\n");
    printf("===================\n\n");

    // 创建默认的（主）事件循环
    // 参数 0 表示使用 libev 的默认后端选择机制
    main_loop = ev_default_loop(0);
    if (!main_loop) {
        fprintf(stderr, "无法创建主事件循环\n");
        exit(1);
    }

    // 打印系统的事件循环后端信息
    printf("主循环后端: 0x%x\n", ev_backend(main_loop));                    // 主循环使用的后端
    printf("推荐的后端: 0x%x\n", ev_recommended_backends());               // libev 推荐的后端
    printf("可嵌入的后端: 0x%x\n", ev_embeddable_backends());             // 支持嵌入的后端

    // 确定可用的嵌入后端
    // 优先使用既推荐又支持嵌入的后端，如果没有则使用所有支持嵌入的后端
    unsigned int embed_backends = ev_embeddable_backends() & ev_recommended_backends();
    if (!embed_backends && ev_embeddable_backends()) {
        embed_backends = ev_embeddable_backends();
    }

    // 尝试创建并配置嵌入式子循环
    if (embed_backends) {
        // 使用嵌入支持的后端创建新的事件循环
        embedded_loop = ev_loop_new(embed_backends);
        if (embedded_loop) {
            printf("\n✓ 成功创建子循环，后端: 0x%x\n", ev_backend(embedded_loop));
            printf("✓ 真正演示 embed 功能：子循环被嵌入到主循环中\n");

            // 创建管道用于触发 embed_cb
            // socketpair 创建一个全双工的 socket 对，用于进程/线程间通信
            if (socketpair(AF_UNIX, SOCK_STREAM, 0, trigger_pipe) == 0) {
                printf("✓ 创建通信管道成功\n");

                // 在子循环中设置 io 观察器来读取管道
                ev_io_init(&pipe_reader, pipe_reader_cb, trigger_pipe[0], EV_READ);
                ev_io_start(embedded_loop, &pipe_reader);
            } else {
                perror("socketpair 失败");
                trigger_pipe[0] = trigger_pipe[1] = -1;
            }

            // 初始化嵌入观察器
            // 参数：观察器指针、回调函数、要嵌入的子循环
            ev_embed_init(&embed_watcher, embed_cb, embedded_loop);
            // 在主循环中启动嵌入观察器，这样子循环就可以通过主循环来处理事件
            ev_embed_start(main_loop, &embed_watcher);
        } else {
            fprintf(stderr, "✗ 无法创建子循环\n");
            embedded_loop = main_loop;  // 回退到使用主循环
            trigger_pipe[0] = trigger_pipe[1] = -1;
        }
    } else {
        printf("\n✗ 当前平台没有可嵌入的后端\n");
        printf("  说明：某些平台（如 macOS）可能不支持嵌入功能\n");
        printf("  现在演示 embed 的概念，使用独立模拟\n\n");
        embedded_loop = main_loop;  // 在不支持嵌入的平台上，使用主循环
        trigger_pipe[0] = trigger_pipe[1] = -1;
    }

    printf("\n设置定时器和观察器：\n");
    printf("- 主循环定时器：每 0.5 秒触发\n");
    printf("- 子循环定时器：每 1.0 秒触发\n");
    printf("- 子循环 io 观察器：读取管道数据\n");
    printf("- 停止定时器：5 秒后退出\n");
    printf("- 嵌入模式：自动模式（embed_cb 会被自动调用）\n\n");

    // 设置子循环定时器（在子循环中运行）
    // ev_timer_init 参数：定时器、回调、首次触发延迟(秒)、重复间隔(秒)
    if (embedded_loop != main_loop) {
        ev_timer_init(&child_timer, child_timer_cb, 1.0, 1.0);  // 1秒后首次触发，之后每1秒重复
        ev_timer_start(embedded_loop, &child_timer);             // 在子循环中启动
    } else {
        // 如果没有独立的子循环，将此定时器放在主循环中
        ev_timer_init(&child_timer, child_timer_cb, 1.0, 1.0);
        ev_timer_start(main_loop, &child_timer);
    }

    // 设置主循环定时器（在主循环中运行）
    ev_timer_init(&main_timer, main_timer_cb, 0.5, 0.5);  // 0.5秒后首次触发，之后每0.5秒重复
    ev_timer_start(main_loop, &main_timer);               // 在主循环中启动

    // 设置停止定时器（一次性触发，用于退出程序）
    // 最后一个参数为 0.0 表示不重复触发
    ev_timer_init(&stop_timer, stop_cb, 5.0, 0.0);  // 5秒后触发一次
    ev_timer_start(main_loop, &stop_timer);        // 在主循环中启动

    printf("开始运行事件循环...\n\n");

    // 运行主事件循环
    // 参数：事件循环、标志（0 表示正常模式）
    // 此函数会阻塞，直到 ev_break 被调用
    ev_run(main_loop, 0);

    if (embedded_loop != main_loop) {
        ev_embed_stop(main_loop, &embed_watcher);
    }

    ev_timer_stop(main_loop, &main_timer);
    ev_timer_stop(main_loop, &stop_timer);

    if (embedded_loop != main_loop) {
        ev_timer_stop(embedded_loop, &child_timer);

        // 停止 io 观察器
        if (trigger_pipe[0] >= 0) {
            ev_io_stop(embedded_loop, &pipe_reader);
        }

        ev_loop_destroy(embedded_loop);
    } else {
        ev_timer_stop(main_loop, &child_timer);
    }

    // 关闭通信管道
    if (trigger_pipe[0] >= 0) close(trigger_pipe[0]);
    if (trigger_pipe[1] >= 0) close(trigger_pipe[1]);

    printf("\n程序结束\n");
    printf("===================\n");
    printf("主循环定时器触发: %d 次\n", main_events);
    printf("子循环定时器触发: %d 次\n", child_events);
    printf("嵌入回调调用: %d 次\n", embed_cb_calls);
    printf("管道读取次数: %d 次\n", pipe_read_count);

    if (embedded_loop == main_loop) {
        printf("\n注意：由于平台限制，本演示使用了单一循环\n");
        printf("在有嵌入支持的平台（如某些 Linux 配置）上，\n");
        printf("子循环会是真正独立的，并通过 ev_embed 与主循环协同工作\n");
    } else {
        printf("\n🎯 embed 的两种模式对比：\n");
        printf("   1. 自动模式（当前模式）：embed_cb 会被自动调用\n");
        printf("      - 使用场景：子循环有真正的 I/O 事件（如 socket）\n");
        printf("      - 优点：自动化，无需手动触发\n");
        printf("      - 注意：必须在 embed_cb 中调用 ev_embed_sweep\n\n");
        printf("   2. 手动模式：回调设为 NULL，无需手动调用 ev_embed_sweep\n");
        printf("      - 使用场景：需要精确控制何时扫描子循环\n");
        printf("      - 优点：更灵活，可以自己决定触发时机\n");
        printf("      - 缺点：需要手动调用 ev_embed_sweep\n\n");
    }

    return 0;
}