#include <ev.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

struct ev_async async_watcher;
int async_received = 0;

static void async_cb(EV_P_ struct ev_async *w, int revents) {
    printf("Async 回调被触发\n");
    async_received++;
    printf("Async 事件计数: %d\n", async_received);
}

void* other_thread(void* arg) {
    struct ev_loop* loop = (struct ev_loop*)arg;
    
    sleep(2);
    printf("其他线程发送 async 事件...\n");
    ev_async_send(loop, &async_watcher);
    
    sleep(2);
    printf("其他线程再次发送 async 事件...\n");
    ev_async_send(loop, &async_watcher);
    
    sleep(1);
    printf("其他线程发送退出信号...\n");
    ev_async_send(loop, &async_watcher);
    
    return NULL;
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);
    pthread_t thread;

    ev_async_init(&async_watcher, async_cb);
    ev_async_start(loop, &async_watcher);

    printf("Async watcher 启动\n");
    printf("创建其他线程...\n");
    
    pthread_create(&thread, NULL, other_thread, loop);

    ev_run(loop, 0);

    pthread_join(thread, NULL);
    printf("程序结束\n");

    return 0;
}