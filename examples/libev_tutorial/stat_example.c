#include <ev.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

struct ev_stat stat_watcher;

static void stat_cb(EV_P_ struct ev_stat *w, int revents) {
    printf("文件状态变化检测: %s\n", w->path);
    printf("  当前大小: %ld 字节\n", (long)w->attr.st_size);
    printf("  当前mtime: %ld\n", (long)w->attr.st_mtime);
    printf("  上次大小: %ld 字节\n", (long)w->prev.st_size);
    printf("  上次mtime: %ld\n", (long)w->prev.st_mtime);
}

int main(void) {
    struct ev_loop *loop = ev_default_loop(0);
    const char *filename = "/tmp/test_libev_stat.txt";

    ev_stat_init(&stat_watcher, stat_cb, filename, 0.5);
    ev_stat_start(loop, &stat_watcher);

    printf("Stat watcher 启动，正在监控文件: %s\n", filename);
    printf("请在另一个终端执行以下命令来修改文件：\n");
    printf("  echo 'test' > /tmp/test_libev_stat.txt\n");
    printf("按 Ctrl+C 退出\n");

    ev_run(loop, 0);

    return 0;
}