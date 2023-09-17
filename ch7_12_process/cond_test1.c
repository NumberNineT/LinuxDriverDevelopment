#include "../common/apue.h"
#include <pthread.h>

// 条件

struct msg {
    struct msg *m_next;
    // more stuff here
    char *p_data;
    uint16_t len;
};

struct msg *workq = NULL;
pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

// 消费者
static void* process_msg(void *arg)
{
    struct msg *mp;

    for (;;) {
        pthread_mutex_lock(&qlock);
        while (workq == NULL)
            pthread_cond_wait(&qready, &qlock);
        mp = workq;
        workq = workq->m_next;
        pthread_mutex_unlock(&qlock);
        // now process the message mp
        // printf("pid:%d ppid:%d process message start:\n", getpid(), getppid());
        printf("tid:%lu process message start:\n", pthread_self());
        printf("msg:%s len:%d\n", mp->p_data, mp->len);
        // printf("pid:%d ppid:%d process message end\n\n", getpid(), getppid());
        printf("tid:%lu process message end\n\n", pthread_self());
        free(mp->p_data);
        free(mp);
        sleep(5);
    }
}

// 生产者
static void enqueue_msg(struct msg *mp)
{
    pthread_mutex_lock(&qlock);
    mp->m_next = workq;
    workq = mp;
    pthread_mutex_unlock(&qlock);
    // send signal
    pthread_cond_signal(&qready); // 线程同步
}

void cond_test1(void)
{
    int ret;
    int cnt = 0;
    pthread_t tids[10];

    // 线程池
    for (int ix = 0; ix < 10; ++ix) {
        ret = pthread_create(&tids[ix], NULL, process_msg, "worker_ix");
        if (ret != 0)
            err_ret("create process idx:%d failed", ix);
    }

    while (1) {
        struct msg *mp = (struct msg *)malloc(sizeof(struct msg));
        if (!mp)
            err_ret("malloc failed");
        mp->p_data = (char *)malloc(256);
        if (!mp->p_data) {
            free(mp);
            err_ret("malloc failed");
        }
        sprintf(mp->p_data, "This is a test string:%d\n", ++cnt);
        mp->len = 20;
        enqueue_msg(mp);
        sleep(1);
    }
}
