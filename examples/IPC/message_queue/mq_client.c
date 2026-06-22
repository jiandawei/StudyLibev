#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef __linux__
#include <fcntl.h>
#include <mqueue.h>

#define MQ_NAME "/my_mq"
#define MAX_MSG_SIZE 256

int main(int argc, char *argv[]) {
    unsigned int prio = 0;
    if (argc > 1) {
        prio = (unsigned int)atoi(argv[1]);
    }

    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
    if (mq == (mqd_t)-1) {
        if (errno == ENOENT) {
            fprintf(stderr, "服务端未启动，请先运行 mq_server\n");
        } else {
            perror("mq_open");
        }
        return 1;
    }
    printf("[POSIX] 已连接到消息队列: %s (发送优先级=%u)\n", MQ_NAME, prio);
    printf("输入消息并回车发送（输入 exit 退出）\n\n");

    char buf[MAX_MSG_SIZE];
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (strcmp(buf, "exit\n") == 0) {
            printf("客户端退出\n");
            break;
        }

        if (mq_send(mq, buf, strlen(buf), prio) < 0) {
            perror("mq_send");
            break;
        }
        printf("已发送 %zu 字节 (优先级=%u)\n", strlen(buf), prio);
    }

    mq_close(mq);
    return 0;
}

#else

#include <sys/msg.h>

#define MSG_TYPE_TEXT 1
#define MAX_TEXT_SIZE 256

struct msgbuf {
    long mtype;
    char mtext[MAX_TEXT_SIZE];
};

int main(int argc, char *argv[]) {
    long mtype = MSG_TYPE_TEXT;
    if (argc > 1) {
        mtype = atol(argv[1]);
        if (mtype <= 0) mtype = MSG_TYPE_TEXT;
    }

    key_t key = ftok(".", 'M');
    if (key < 0) {
        perror("ftok");
        return 1;
    }

    int msqid = msgget(key, 0666);
    if (msqid < 0) {
        if (errno == ENOENT) {
            fprintf(stderr, "服务端未启动，请先运行 mq_server\n");
        } else {
            perror("msgget");
        }
        return 1;
    }
    printf("[System V] 已连接到消息队列, msqid=%d (消息类型=%ld)\n", msqid, mtype);
    printf("输入消息并回车发送（输入 exit 退出）\n\n");

    struct msgbuf msg;
    msg.mtype = mtype;

    while (fgets(msg.mtext, sizeof(msg.mtext), stdin) != NULL) {
        if (strcmp(msg.mtext, "exit\n") == 0) {
            printf("客户端退出\n");
            break;
        }

        size_t len = strlen(msg.mtext);
        if (msgsnd(msqid, &msg, len, 0) < 0) {
            perror("msgsnd");
            break;
        }
        printf("已发送 %zu 字节 (类型=%ld)\n", len, mtype);
    }

    return 0;
}

#endif
