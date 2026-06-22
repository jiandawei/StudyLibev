#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct ComputeTask {
    ev_async async;
    int input;
    int result;
} ComputeTask;

static void compute_async_cb(EV_P_ ev_async *w, int revents) {
    ComputeTask *task = (ComputeTask*)w;
    printf("计算结果: %d * 2 = %d\n", task->input, task->result);
    free(task);
}

static void *compute_thread(void *arg) {
    ComputeTask *task = (ComputeTask*)arg;
    
    // 模拟耗时计算
    sleep(2);
    task->result = task->input * 2;
    
    // 通知主线程
    ev_async_send(EV_DEFAULT, &task->async);
    return NULL;
}

static void submit_compute_task(int input) {
    ComputeTask *task = malloc(sizeof(ComputeTask));
    task->input = input;
    ev_async_init(&task->async, compute_async_cb);
    ev_async_start(EV_DEFAULT, &task->async);
    
    pthread_t tid;
    pthread_create(&tid, NULL, compute_thread, task);
    pthread_detach(tid);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    submit_compute_task(42);
    submit_compute_task(100);
    
    printf("计算任务提交（等待结果）\n");
    ev_run(loop, 0);
    return 0;
}