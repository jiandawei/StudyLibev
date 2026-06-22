#include <ev.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

struct ev_async async_watcher;
struct ev_loop *loop;

static void async_cb(EV_P_ ev_async *w, int revents) {
    printf("主线程收到异步通知\n");
    printf("主线程退出事件循环\n");
    ev_break(loop, EVBREAK_ALL);
}

static void *worker_thread(void *arg) {
    sleep(2);
    printf("工作线程发送异步通知\n");
    ev_async_send(loop, &async_watcher);
    return NULL;
}

int main() {
    pthread_t tid;
    
    loop = ev_default_loop(0);
    ev_async_init(&async_watcher, async_cb);
    ev_async_start(loop, &async_watcher);
    
    pthread_create(&tid, NULL, worker_thread, NULL);
    
    printf("主线程运行中\n");
    ev_run(loop, 0);
    
    pthread_join(tid, NULL);
    return 0;
}