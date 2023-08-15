#include "../common/apue.h"

#include <pthread.h>
#include <time.h>


extern int pthread_equal(pthread_t tid1, pthread_t tid2);
extern pthread_t pthread_self(void);
extern int pthread_create(pthread_t *restrict tidp, const pthread_attr_t *restrict attr,
                    void *(*start_rtn)(void*), void * restrict arg);
//2.自己调用 pthread_exit();3.同一进程中的其他线程调用 pthread_cancel()
extern void pthread_exit(void *rval_ptr); // 线程退出的三种方式:1.函数运行结束退出; 
extern int pthread_join(pthread_t thread, void **rval_ptr); // 阻塞等待线程结束
extern int pthread_cancel(pthread_t tid);
// extern void pthread_cleanup_push(void (*rtn)(void*), void *arg); // 线程退出清理程序
// extern void pthread_cleanup_pop(int execute);

static void printids(const char *s)
{
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    printf("%s pid:%lu tid:%lu 0x%lx\n", s, (unsigned long)pid, 
            (unsigned long)tid, (unsigned long)tid);
}

static pthread_t ntid1;
void *thread_fun1(void *arg);

void thread_test1(void)
{
    // printf("thread id:%ld\n", pthread_self());
    int err;

    err = pthread_create(&ntid1, NULL, thread_fun1, NULL);
    if (err != 0) {
        err_ret("Can't create new thread");
    }
    printids("main thread");
    sleep(1);
    exit(0);
}

void *thread_fun1(void *arg)
{
    printids("child thread");
    return NULL;
}

void *thread_fun2(void *arg);
void *thread_fun3(void *arg);

void thread_exit_test2(void)
{
    int err;
    pthread_t tid1, tid2;
    void *tret;

    err = pthread_create(&tid1, NULL, thread_fun2, NULL);
    if (err != 0)
        err_ret("create thread1 failed");
    err = pthread_create(&tid2, NULL, thread_fun3, NULL);
    if (err != 0)
        err_ret("create thread1 failed");

    err = pthread_join(tid1, &tret); // 阻塞等待线程结束
    if (err != 0)
        err_ret("can't join with thread1")
    printf("thread1 exit, code:%ld\n", (long)tret);
    err = pthread_join(tid2, &tret);
    if (err != 0)
        err_ret("can't join with thread1")
    printf("thread2 exit, code:%ld\n", (long)tret);
    exit(0); // 主线程退出, 进程退出
}
// 线程正常退出
void *thread_fun2(void *arg)
{
    printf("thread2 running..\n");
    return ((void*)1);
}

// 线程自身调用 pthread_exit() 退出
void *thread_fun3(void *arg)
{
    int ix = 0;
    printf("thread3 running\n");

    while (1) {
        if (ix == 5) {
            printf("thread2 exit");
            pthread_exit((void*)2);
        }
        sleep(1);
        printf("ix:%d\n", ++ix);
    }
}

// pthread_exit() 返回的参数需要是堆内存/全局变量,不可以是栈内存
// 错误示范:
struct foo {
    int a, b, c, d;
};
static void printfoo(const char *s, const struct foo *fp);
void *thread_fun4(void *arg);

void thread_exit_test3(void)
{
    pthread_t tid;
    int ret;
    struct foo *fp;

    ret = pthread_create(&tid, NULL, thread_fun4, NULL);
    if (ret != 0)
        err_ret("create thread failed");

    ret = pthread_join(tid, (void*)&fp);
    if (ret != 0)
        err_ret("join thread failed");
    
    sleep(1);
    printfoo("main thread", fp);
    exit(0);
}

static void printfoo(const char *s, const struct foo *fp)
{
    printf("%s: fp:0x%lx\n", s, (unsigned long)fp); // 打印指针的值
    printf("a:%d b:%d c:%d d:%d\n\n", fp->a, fp->b, fp->c, fp->d);
}

void *thread_fun4(void *arg)
{
    struct foo f;
    f.a = 1;
    f.b = 2;
    f.c = 3;
    f.d = 4;
    printf("thread fun4 running\n");
    printfoo("child thread", &f);
    // 可以改为堆内存/全局变量
    pthread_exit((void*)&f); // pthread_exit() 线程退出传栈内存
}

