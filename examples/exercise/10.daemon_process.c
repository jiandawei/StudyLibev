#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>

static void daemonize() {
    pid_t pid;
    
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }
    if (pid != 0) exit(0);
    
    setsid();
    
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }
    if (pid != 0) exit(0);
    
    chdir("/");
    umask(0);
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    openlog("my_daemon", LOG_PID, LOG_DAEMON);
}

static void timer_cb(EV_P_ ev_timer *w, int revents) {
    syslog(LOG_INFO, "守护进程运行中...");
}

int main() {
    daemonize();
    
    struct ev_loop *loop = ev_default_loop(0);
    ev_timer timer;
    
    ev_timer_init(&timer, timer_cb, 1.0, 5.0);
    ev_timer_start(loop, &timer);
    
    syslog(LOG_INFO, "守护进程启动成功");
    ev_run(loop, 0);
    
    closelog();
    return 0;
}