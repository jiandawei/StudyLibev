#include <ev.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <queue>
#include <mutex>
#include <condition_variable>

struct ev_async async_watcher;
struct ev_loop *loop;

std::queue<int> task_queue;
std::mutex queue_mutex;
std::condition_variable cv;
bool running = true;

struct TaskResult {
    int task_id;
    int result;
};

// 在主线程执行
static void result_cb(EV_P_ ev_async *w, int revents) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    while (!task_queue.empty()) {
        int task_id = task_queue.front();
        task_queue.pop();
        printf("任务 %d 完成\n", task_id);
    }
}

static void *worker_thread(void *arg) {
    while (running) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock);
        
        // 模拟任务处理
        sleep(1);
        
        // 发送完成通知
        ev_async_send(loop, &async_watcher);
    }
    return NULL;
}

static void *producer_thread(void *arg) {
    int task_id = 0;
    while (running) {
        sleep(1);
        // 提交任务
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            task_queue.push(++task_id);
        }
        cv.notify_one();
    }
    return NULL;
}

int main() {
    pthread_t tid_worker, tid_producer;
    
    loop = ev_default_loop(0);
    ev_async_init(&async_watcher, result_cb);
    ev_async_start(loop, &async_watcher);
    
    pthread_create(&tid_worker, NULL, worker_thread, NULL);
    pthread_create(&tid_producer, NULL, producer_thread, NULL);
    
    printf("线程池运行中（Ctrl+C 退出）\n");
    ev_run(loop, 0);
    
    running = false;
    cv.notify_all();
    pthread_join(tid_worker, NULL);
    pthread_join(tid_producer, NULL);
    return 0;
}