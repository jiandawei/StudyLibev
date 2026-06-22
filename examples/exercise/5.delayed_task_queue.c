#include <ev.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    ev_timer timer;
    int task_id;
} DelayedTask;

static void task_cb(EV_P_ ev_timer *w, int revents) {
    DelayedTask *task = (DelayedTask*)w;
    printf("执行任务: %d\n", task->task_id);
    free(task);
}

static void add_delayed_task(EV_P_ double delay, int task_id) {
    DelayedTask *task = malloc(sizeof(DelayedTask));
    task->task_id = task_id;
    ev_timer_init(&task->timer, task_cb, delay, 0.);
    ev_timer_start(EV_A_ &task->timer);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    add_delayed_task(loop, 1.0, 1);  // 1秒后执行
    add_delayed_task(loop, 2.5, 2);  // 2.5秒后执行
    add_delayed_task(loop, 5.0, 3);  // 5秒后执行
    
    printf("延迟任务队列启动\n");
    ev_run(loop, 0);
    return 0;
}