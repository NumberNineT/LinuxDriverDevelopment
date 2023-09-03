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
// extern void pthread_cleanup_push(void (*rtn)(void*), void *arg); // 线程清理处理程序
// extern void pthread_cleanup_pop(int execute);
int pthread_detach(pthread_t tid); // 分离一个线程,作用:线程结束后内存资源立马被系统收回
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t * restrict attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t * restrict mutex, const struct timespec *restrict tsptr);

int pthread_rwlock_init(pthread_rwlock_t * restrict rwlock, const pthread_rwlockattr_t * restrict attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t * restrict rwlock, const struct timespec * restrict tsptr);
int pthread_rwlock_timedwrlock(pthread_rwlock_t * restrict rwlock, const struct timespec * restrict tsptr);

int pthread_cond_init(pthread_cond_t * restrict cond, const pthread_condattr_t * restrict attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t  * restrict cond, pthread_mutex_t * restrict mutex);
int pthread_cond_timedwait(pthread_cond_t * restrict cond, pthread_mutex_t * mutex, const struct timespec * restrict tsptr);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_breadcast(pthread_cond_t *cond);

int pthread_spin_init(pthread_spinlock_t *lock, int shared); // 不阻塞(阻塞会导致线程休眠,调度器重新调度),忙等
int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);


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


void cleanup(void *arg);

void *thread_fun5(void *arg)
{
    printf("thread5 start\n");
    pthread_cleanup_push(cleanup, "thread5 first handler");
    pthread_cleanup_push(cleanup, "thread5 second handler");
    if (arg)
        return ((void*)1);
    pthread_cleanup_pop(0); // 不会调用
    pthread_cleanup_pop(0);

    return ((void*)1);
}

void *thread_fun6(void *arg)
{
    printf("thread6 start\n");
    pthread_cleanup_push(cleanup, "thread6 first handler");
    pthread_cleanup_push(cleanup, "thread6 second handler");
    if (arg)
        pthread_exit((void*)2);
    pthread_cleanup_pop(0); // 不会调用
    pthread_cleanup_pop(0);

    pthread_exit((void*)2);
}

void thread_cleanup_test1(void)
{
    int err;
    pthread_t tid5, tid6;
    void *tret;

    err = pthread_create(&tid5, NULL, thread_fun5, (void*)1);
    if (err != 0) {
        err_ret("can't create thread5");
    }
    err = pthread_create(&tid6, NULL, thread_fun6, (void*)1);
    if (err != 0) {
        err_ret("can't create thread6");
    }

    err = pthread_join(tid5, &tret);
    if (err != 0) {
        err_ret("Can't join with thread5");
    }
    printf("thread5 exit with code:%ld\n", (long)tret);
    err = pthread_join(tid6, &tret);
    if (err != 0) {
        err_ret("Can't join with thread6");
    }
    printf("thread6 exit with code:%ld\n", (long)tret);
    exit(0);
}

void cleanup(void *arg)
{
    printf("cleanup arg:%s\n", (char*)arg);
}

// 线程同步
// 当多个线程共享同一内存时, 需要确保每一个线程看到一致的数据视图
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; // 静态内存
pthread_mutex_t *pMutex2 = NULL; // 动态分配内存

void thread_sync_test1(void)
{

}


// 避免死锁
// 1. 同一个锁加锁两次;
// 2. 两个锁, 不同线程使用的顺序相反时也会导致死锁
// 解决措施:
// 1. 仔细检查上锁顺序, 确保不会出现相反顺序上锁情况;
// 2. 需要对多个锁进行上锁时, 可以使用 pthread_mutex_trylock() 获取成功, 前进;失败则释放之前的锁, 一段时间后重试
static pthread_mutex_t mutex8 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex9 = PTHREAD_MUTEX_INITIALIZER;
static uint32_t test_cnt8 = 0;
pthread_t tid8, tid9;
void *thread9_entry(void *arg);

void thread_lock_test2(void)
{
    int ret;

    ret = pthread_create(&tid8, NULL, thread9_entry, "thread8");
    if (ret != 0) {
        err_ret("create thread8 failed\n");
    }
    ret = pthread_create(&tid9, NULL, thread9_entry, "thread9");
    if (ret != 0) {
        err_ret("create thread9 failed\n");
    }

    pause();
}

void *thread9_entry(void *arg)
{
    while(1) {
        pthread_mutex_lock(&mutex8);
        pthread_mutex_lock(&mutex9);
        printf("%s:%u\n", (char*)arg, ++test_cnt8);
        // pthread_mutex_lock(&mutex9); // 死锁:重复上锁
        pthread_mutex_unlock(&mutex9);
        pthread_mutex_unlock(&mutex8);
    }
}

void thread_lock_test3(void)
{
    int ret;
    struct timespec timeout;
    struct tm *tmp;
    char buf[64];
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&lock);
    printf("mutex is locked\n");
    clock_gettime(CLOCK_REALTIME, &timeout);
    tmp = localtime(&timeout.tv_sec);
    strftime(buf, sizeof(buf), "%I:%M:%S %p.", tmp);
    printf("current time:%s\n", buf);

    // pthread_mutex_timed_lock 要使用 GMT 时间, 从当前时间开始阻塞 10s
    timeout.tv_sec += 10; 
    // 这里会导致死锁
    ret = pthread_mutex_timedlock(&lock, &timeout);
    clock_gettime(CLOCK_REALTIME, &timeout);
    tmp = localtime(&timeout.tv_sec);
    strftime(buf, sizeof(buf), "%r", tmp);
    printf("This time now is:%s\n", buf);
    if (ret == 0) {
        printf("locked again\n");
    } else {
        printf("cant' lock again errno:%d %s\n", errno, strerror(ret));
    }
}


