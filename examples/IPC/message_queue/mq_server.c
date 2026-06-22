#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#ifdef __linux__
#include <fcntl.h>
#include <mqueue.h>

#define MQ_NAME "/my_mq"
#define MAX_MSG_SIZE 256
#define MAX_MSG_COUNT 10

static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

int main() {
    signal(SIGINT, sigint_handler);

    mq_unlink(MQ_NAME);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MSG_COUNT;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }
    printf("[POSIX] 消息队列已创建: %s\n", MQ_NAME);
    printf("等待接收消息（Ctrl+C 退出）...\n\n");

    char buf[MAX_MSG_SIZE];
    unsigned int prio;

    while (running) {
        ssize_t n = mq_receive(mq, buf, MAX_MSG_SIZE, &prio);
        if (n >= 0) {
            buf[n] = '\0';
            printf("收到消息 [优先级=%u]: %s", prio, buf);
        } else {
            if (errno != EINTR) {
                perror("mq_receive");
                break;
            }
        }
    }

    mq_close(mq);
    mq_unlink(MQ_NAME);
    printf("\n服务端已退出，消息队列已删除\n");
    return 0;
}

#else

#include <sys/msg.h>

#define MAX_TEXT_SIZE 256

struct msgbuf {
    long mtype;
    char mtext[MAX_TEXT_SIZE];
};

static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

int main() {
    signal(SIGINT, sigint_handler);

    key_t key = ftok(".", 'M');
    if (key < 0) {
        perror("ftok");
        return 1;
    }

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid < 0) {
        perror("msgget");
        return 1;
    }
    printf("[System V] 消息队列已创建, msqid=%d, key=0x%x\n", msqid, key);
    printf("等待接收消息（Ctrl+C 退出）...\n\n");

    struct msgbuf msg;

    while (running) {
        ssize_t n = msgrcv(msqid, &msg, sizeof(msg.mtext), 0, IPC_NOWAIT);
        if (n >= 0) {
            msg.mtext[n] = '\0';
            printf("收到消息 [类型=%ld]: %s", msg.mtype, msg.mtext);
        } else {
            if (errno == ENOMSG) {
                usleep(100000);
                continue;
            }
            if (errno != EINTR) {
                perror("msgrcv");
                break;
            }
        }
    }

    msgctl(msqid, IPC_RMID, NULL);
    printf("\n服务端已退出，消息队列已删除\n");
    return 0;
}

#endif
