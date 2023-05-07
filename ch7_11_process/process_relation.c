#include "../common/apue.h"

/*
 *	ISO C99 Standard: 7.5 Errors	<errno.h>
 */
#include <errno.h>

/*
 *	ISO C99 Standard: 7.14 Signal handling <signal.h>
 */
#include <signal.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	POSIX Standard: 9.2.2 User Database Access	<pwd.h>
 */
#include <pwd.h>

/*
 *	POSIX Standard: 3.2.1 Wait for Process Termination	<sys/wait.h>
 */
#include <sys/wait.h>

/*
 *	ISO C99 Standard: 7.13 Nonlocal jumps	<setjmp.h>
 */
#include <setjmp.h>

#include <stdlib.h>
#include <stdio.h>

// 进程, 父进程, 进程组, 会话(进程组集合), 作业控制(进程组集合)

int kill(pid_t pid, int signo);
int raise(int signo); // 给进程自己发送信号
unsigned int alarm(unsigned int seconds); // 为进程注册一个闹钟
int pause(void); // 阻塞等待一个信号
// signal set
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);


// orphaned process group
static void sig_hup(int signo)
{
    printf("SIGHUB received, pid = %ld\n", (long)getpid());
}

static void pr_ids(char *name)
{
    printf("%s: pid=%ld, ppid=%ld, pgrp=%ld, tpgrp=%ld\n", 
            name, (long)getpid(), (long)getppid(), (long)getpgrp(), \
            (long)tcgetpgrp(STDIN_FILENO));
    fflush(stdout);
}

int test_main(void)
{
    char c;
    pid_t pid;

    pr_ids("parent");

    if ((pid = fork()) < 0) {
        err_sys("fork failed");
        return -1;
    } else if (pid > 0) { // parent
        sleep(5); // sleep to let child stop itself
    } else { // child
        pr_ids("child");
        signal(SIGHUP, sig_hup); // establish signal handler
        kill(getpid(), SIGQUIT); // stop ourself
        pr_ids("child"); // print onlyu if we're continued
        if (read(STDIN_FILENO, &c, 1) != 1) {
            err_sys("read err on controlling TTY, errno:%d", errno);
        }
    }
    exit(0);
}

// we'd better use sigaction() rather thann signal() cause the difference signal() realization
static void sig_usr(int signo);
void sig_test(void)
{
    if (signal(SIGUSR1, sig_usr) == SIG_ERR) {
        err_sys("can't catch SIGUSR1");
    }
    if (signal(SIGUSR2, sig_usr) == SIG_ERR) {
        err_sys("can't catch SIGUSR2");
    }
    for (;;) pause();
}

static void sig_usr(int signo)
{
    if (signo == SIGUSR1) {
        printf("%d SIGUSR1 received\n", getpid());
        exit(-1);
    } else if (signo == SIGUSR2) {
        printf("%d SIGUSR2 received\n", getpid());
    } else {
        err_quit("unknown signo:%d", signo);
    }
}

static void alarm_sig_handler(int signo);
void sig_test2(void)
{
    struct passwd *ptr;
    if (signal(SIGALRM, alarm_sig_handler) == SIG_ERR) {
        err_ret("Can't catch SIGALRM");
    }
    for (;;) {
        if((ptr = getpwnam("root")) == NULL) {
            err_sys("error");
        }
        if (strcmp(ptr->pw_name, "root") != 0) {
            printf("return value corrupted! pw_name=%s\n", ptr->pw_name);
        } else {
            printf("correct\n");
        }
    }
}
static void alarm_sig_handler(int signo)
{
    struct passwd *rootptr;
    /* 信号的处理函数中不能调用不可重入函数 , eg. malloc(), free(), printf()... */
    // getpwnam() 中使用了 static 变量, 是不可重入函数
    if ((rootptr = getpwnam("root")) == NULL) {
        err_sys("get present work name");
    }
    alarm(1);
}

static void childterm_sig_handler(int signo);
void sig_test3(void)
{
    pid_t pid;

    if (signal(SIGCLD, childterm_sig_handler) == SIG_ERR) {
        perror("signal error");
    }

    if ((pid = fork()) < 0) {
        perror("fork error");
    } else if (pid == 0) { // child
        printf("child:%d %d running\n", getpid(), getppid());
        sleep(2);
        printf("child exit\n");
        _exit(0);
    } else { // parent
        printf("parent:%d %d running\n", getpid(), getppid());
    }
    pause(); // 阻塞等待, 直到有信号到来
    printf("parent wake up\n");
    sleep(2);
    printf("parent exit\n");
    exit(0);
}
static void childterm_sig_handler(int signo)
{
    // 在信号处理函数中等待? 软件中断中!!! 应该不可行, 仅仅为了测试
    pid_t pid;
    int status;

    printf("SIGCLD received\n"); // printf() 也不可以调用
    if (signal(SIGCLD, childterm_sig_handler) == SIG_ERR) // 重新注册信号
        perror("signal error");
    if ((pid = wait(&status)) < 0) {
        perror("wait error");
    }
    printf("exit process pid = %d\n", pid);
}

/* 每个进程只能有一个闹钟时间 alarm */
static void alarm_sig_handler2(int signo) {}
unsigned int sleep1(unsigned int seconds);
unsigned int sleep2(unsigned int seconds);
void sig_test4(void)
{
    printf("Enter\n");
    // sleep1(2);
    sleep2(2);
    printf("exit\n");
}

/**
 * @fun: 自己实现一个 sleep() 使得进程休眠一段时间\
 * @desc: 这个 sleep1 存在三个问题: 如果 alarm 在 sleep1() 之前有使用, 竞争条件...
 * @param: 休眠时间
 * @ret: 返回没有休眠的时间
*/
unsigned int sleep1(unsigned int seconds)
{
    if (signal(SIGALRM, alarm_sig_handler2) == SIG_ERR) {
        return seconds;
    }
    alarm(seconds); // 在一个高速运行系统中, SIGALRM 的信号处理程序可能比 pause 先运行, 导致进程一直在 pause()
    pause();
    return alarm(0);
}

static jmp_buf env_alarm;
static void alarm_sig_handler3(int signo)
{
    longjmp(env_alarm, 1);
}

/**
 * @desc: 为了解决竞争条件
 *        问题:SIGALRM 会中断其他信号处理程序
*/
unsigned int sleep2(unsigned int seconds)
{
    if (signal(SIGALRM, alarm_sig_handler3) == SIG_ERR) {
        return seconds;
    }
    if (setjmp(env_alarm) == 0) {
        alarm(seconds);
        pause();
    }
    return alarm(0);
}

static void sig_int1(int signo);
int sig_test5(void)
{
    unsigned int unslept;
    if (signal(SIGINT, sig_int1) == SIG_ERR) {
        err_sys("register signal failed");
    }
    // pause();

    // longjmp() 如果打断之前的信号处理, 信号处理执行完毕后, 不会继续之前其他信号的信号处理程序
    unslept = sleep2(1); 
    printf("sleep return:%u\n", unslept);
    exit(0);
}
static void sig_int1(int signo)
{
    int i, j;
    volatile int k; // 防止被编译器优化

    // 延时一段时间, 大约 2s
    printf("sig_int start\n");
    for (i = 0; i < 300000; i++)
        for (j = 0; j < 4000; ++j)
            k += i * j;
    printf("sig_int exit\n");
}

static void alarm_sig_handler4(int signo);
// alarm() 还可以用来中断对一些低速设备的访问, 一段时间后停止执行该操作
void sig_alarm_test1(void)
{
    int n;
    char line[MAXLINE];

    if (signal(SIGALRM, alarm_sig_handler4) == SIG_ERR) {
        err_sys("error");
    }
    alarm(5);
    /* 运行正常 难道是 Linux signal() 注册的信号, 被中断的系统调用默认是继续执行的? */
    if ((n = read(STDIN_FILENO, line, MAXLINE)) < 0) {
        err_ret("read error");
    }
    alarm(0); // 取消闹钟
    line[n] = '\n';
    if (write(STDOUT_FILENO, line, n) != n) {
        err_ret("write failed");
    }
    exit(0);
}
static void alarm_sig_handler4(int signo)
{
    // 什么都不干, 仅仅中断阻塞的调用
    printf("%s\n", __func__);
}

/* longjmp() 可以确保低速的系统调用不被信号中断 */
static void alarm_sig_handler5(int signo);
void sig_test6(void)
{
    int n;
    char line[MAXLINE];
    unsigned int unslept;

    if (signal(SIGALRM, alarm_sig_handler5) == SIG_ERR)
        err_sys("register signal failed");
    
    if (setjmp(env_alarm) != 0) {
        err_sys("read timeout");
    }
    printf("prepare to read\n");
    /* alarm 为什么只会运行一次??? */
    unslept = alarm(2);
    printf("unslept:%u\n", unslept);
    if ((n = read(STDIN_FILENO, line, MAXLINE)) < 0) {
        err_sys("read err");
    }
    alarm(0);
    if (write(STDOUT_FILENO, line, n) != n) {
        err_sys("write err");
    }
    exit(0);
}
static void alarm_sig_handler5(int signo)
{
    printf("%s\n", __func__);
    longjmp(env_alarm, 1);
}


// 自己通过位的方式实现 sigset_t 类型
#define sigemptyset_m(ptr) (*ptr = 0)
#define sigfillset_m(ptr)  (*(ptr) = ~(sigset_t_m)0, 0) /* 逗号表达式, 最后的是返回值 */
#define SIGBAD_M(signo)    ((signo) <= 0 || (signo) >= NSIG_M)
#define NSIG_M              32
typedef int sigset_t_m;
int sigaddset_m(sigset_t_m *set, int signo)
{
    if (SIGBAD_M(signo)) {
        errno = EINVAL;
        return -1;
    }
    *set |= 1 << (signo - 1); // 置1
    return 0;
}

int sigdelset_m(sigset_t_m *set, int signo)
{
    if (SIGBAD_M(signo)) {
        errno = EINVAL;
        return -1;
    }
    *set &= ~(1 << (signo - 1)); // 置0
    return 0;
}

int sigismember_m(sigset_t_m *set, int signo)
{
    if (SIGBAD_M(signo)) {
        errno = EINVAL;
        return -1;
    }
    return ((*set & (1 << (signo - 1))) != 0);
}
// 只支持 int 类型二进制
static void itoa_m(int num, char *out, int jinzhi)
{
    for (int ix = 31, jx = 0; ix >= 0; --ix, ++jx) {
        if ((num >> ix) & 0x01) {
            out[jx] = '1';
        } else {
            out[jx] = '0';
        }
    }
}
static void print_sig(sigset_t_m *ptr)
{
    char str[64];
    itoa_m(*ptr, str, 2);
    // itoa并不是一个标准的C函数，它是Windows特有的，如果要写跨平台的程序，请用sprintf。
    printf("value:%s\n", str);
}

void sig_test_m(void)
{
    sigset_t_m sig;
    sigemptyset_m(&sig);
    print_sig(&sig);
    // sigfillset_m(&sig);
    // print_sig(&sig);
    sigaddset_m(&sig, SIGINT);
    if (sigismember_m(&sig, SIGINT)) {
        printf("bit:%d is set\n", SIGINT);
    } else {
        printf("bit:%d is not set\n", SIGINT);
    }
    print_sig(&sig);
    sigdelset_m(&sig, SIGINT);
    print_sig(&sig);
}