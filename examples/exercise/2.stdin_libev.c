#include <ev.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

void stdin_cb(EV_P_ struct ev_io *w, int revents)
{
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    read(STDIN_FILENO, buf, BUF_SIZE);
    printf("%s", buf);
}

int main()
{
    struct ev_loop *loop = ev_default_loop(0);
    struct ev_io io_watcher;
    ev_io_init(&io_watcher, stdin_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, &io_watcher);

    ev_run(loop, 0);
}