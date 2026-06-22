#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ev_stat stat_watcher;
char *config_file = "config.txt";

static void load_config() {
    FILE *fp = fopen(config_file, "r");
    if (!fp) return;
    
    char buf[1024];
    printf("重新加载配置, 配置文件内容如下:\n");
    while (fgets(buf, sizeof(buf), fp)) {
        printf("  %s", buf);
    }
    printf("\n");
    fclose(fp);
}

static void stat_cb(EV_P_ ev_stat *w, int revents) {
    printf("配置文件 %s 发生变化\n", config_file);
    load_config();
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    
    ev_stat_init(&stat_watcher, stat_cb, config_file, 0.);
    ev_stat_start(loop, &stat_watcher);
    
    printf("监控配置文件: %s\n", config_file);
    load_config();
    ev_run(loop, 0);
    return 0;
}