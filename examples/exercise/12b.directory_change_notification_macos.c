#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/event.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_WATCHED 256

struct watched_file {
    int fd;
    char name[256];
};

struct ev_io kq_watcher;
int kq_fd;
struct watched_file watched[MAX_WATCHED];
int watched_cnt = 0;

static void scan_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(fullpath, &st) < 0) continue;
        if (!S_ISREG(st.st_mode)) continue;

        int fd = open(fullpath, O_EVTONLY);
        if (fd < 0) continue;

        if (watched_cnt < MAX_WATCHED) {
            watched[watched_cnt].fd = fd;
            strncpy(watched[watched_cnt].name, entry->d_name, 255);
            watched_cnt++;
        } else {
            close(fd);
        }
    }
    closedir(dir);
}

static void register_kevents(int kq_fd) {
    struct kevent changes[MAX_WATCHED + 1];
    int n = 0;

    for (int i = 0; i < watched_cnt; i++) {
        EV_SET(&changes[n], watched[i].fd, EVFILT_VNODE,
               EV_ADD | EV_ENABLE | EV_CLEAR,
               NOTE_WRITE | NOTE_DELETE | NOTE_RENAME,
               0, (void *)(long)i);
        n++;
    }

    int dir_fd = open(".", O_EVTONLY);
    if (dir_fd >= 0) {
        EV_SET(&changes[n], dir_fd, EVFILT_VNODE,
               EV_ADD | EV_ENABLE | EV_CLEAR,
               NOTE_WRITE,
               0, (void *)(long)-1);
        n++;
    }

    kevent(kq_fd, changes, n, NULL, 0, NULL);
}

static void kq_cb(EV_P_ ev_io *w, int revents) {
    struct kevent event;
    struct timespec timeout = {0, 0};

    while (kevent(kq_fd, NULL, 0, &event, 1, &timeout) > 0) {
        long idx = (long)event.udata;

        if (idx == -1) {
            printf("目录内容发生变化（新文件或文件被删除）\n");
        } else if (idx >= 0 && idx < watched_cnt) {
            if (event.fflags & NOTE_WRITE)
                printf("文件 %s 被修改\n", watched[idx].name);
            if (event.fflags & NOTE_DELETE)
                printf("文件 %s 被删除\n", watched[idx].name);
            if (event.fflags & NOTE_RENAME)
                printf("文件 %s 被重命名\n", watched[idx].name);
        }
    }
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);

    kq_fd = kqueue();
    if (kq_fd < 0) {
        perror("kqueue");
        return 1;
    }

    scan_directory(".");
    register_kevents(kq_fd);

    ev_io_init(&kq_watcher, kq_cb, kq_fd, EV_READ);
    ev_io_start(loop, &kq_watcher);

    printf("监控当前目录（kqueue，Ctrl+C 退出）\n");
    ev_run(loop, 0);

    for (int i = 0; i < watched_cnt; i++)
        close(watched[i].fd);
    close(kq_fd);
    return 0;
}
