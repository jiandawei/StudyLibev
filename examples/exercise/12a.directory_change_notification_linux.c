#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>

struct ev_io inotify_watcher;
int inotify_fd;

static void inotify_cb(EV_P_ ev_io *w, int revents) {
    char buf[4096];
    struct inotify_event *event;
    int n, i = 0;

    n = read(inotify_fd, buf, sizeof(buf));
    while (i < n) {
        event = (struct inotify_event *)&buf[i];
        printf("文件 %s 发生变化，mask: %x\n",
               event->len ? event->name : "(未知)", event->mask);
        i += sizeof(struct inotify_event) + event->len;
    }
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    int wd;

    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
        return 1;
    }

    wd = inotify_add_watch(inotify_fd, ".",
                           IN_CREATE | IN_DELETE | IN_MODIFY);
    if (wd < 0) {
        perror("inotify_add_watch");
        return 1;
    }

    ev_io_init(&inotify_watcher, inotify_cb, inotify_fd, EV_READ);
    ev_io_start(loop, &inotify_watcher);

    printf("监控当前目录（inotify，Ctrl+C 退出）\n");
    ev_run(loop, 0);

    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    return 0;
}
