#include "apue.h"
#include <stdio.h>
#include <stdlib.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	POSIX Standard: 3.2.1 Wait for Process Termination	<sys/wait.h>
 */
#include <sys/wait.h>

// void *path_alloc()
// {
    
// }

// 根据 <sys/wait.h> 中的 API 打印进程退出原因
void pr_exit(int status)
{
    if (WIFEXITED(status))
        printf("normal termination, exit status:%d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status)) {
        printf("abnormal termination, signal number:%d %s\n", WTERMSIG(status),
#ifdef WCOREDUMP
            WCOREDUMP(status) ? "(core file generated)" : "");
#else
            "");
#endif
    }
    else if (WIFSTOPPED(status)) {
        printf("child stoped, signal number:%d\n", WSTOPSIG(status));
    }

    return;
}

// 打印信号集中的信号类型, 获取当前进程屏蔽的信号集合
// 可以在信号的回调函数中调用
void pr_mask(const char *str)
{
    sigset_t sigset;
    int errno_save;

    errno_save = errno;
    
    if (sigprocmask(0, NULL, &sigset) < 0) { // 第二个参数为空, 则第一个参数无意义, 调用仅仅是 get()
        err_ret("sigprocmask error");
    }
    printf("%s masked signals: ", str);
    if (sigismember(&sigset, SIGINT)) {
        printf("SIGINT\t");
    }
    if (sigismember(&sigset, SIGFPE)) {
        printf("SIGFPE\t");
    }
    if (sigismember(&sigset, SIGQUIT)) {
        printf("SIGQUIT\t");
    }
    if (sigismember(&sigset, SIGUSR1)) {
        printf("SIGUSR1\t");
    }
    if (sigismember(&sigset, SIGUSR2)) {
        printf("SIGUSR2\t");
    }
    if (sigismember(&sigset, SIGALRM)) {
        printf("SIGALRM\t");
    }
    if (sigismember(&sigset, SIGFPE)) {
        printf("SIGFPE\t");
    }
    // 其他信号
    printf("\n");
    errno = errno_save;

    return;
}

/**
 * @fun: 为了避免每次都创建 flock 结构体, 增加一个接口, 直接对记录锁(文件的字节范围锁)加锁/解锁 接口
 * @param fd 文件描述符
 * @param cmd 锁的操作:F_GETLK, F_SETLK, F_SETLKW
 * @param type: 锁的类型:F_RFLCK, F_WRLCK, F_UNLCK
 * @param offset: 起始位置偏移量
 * @param whence: 起始位置
 * @param len: 要加锁区域长度
*/
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;

    return (fcntl(fd, cmd, &lock));
}

pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;

    if (fcntl(fd, F_GETLK, &lock) < 0)
        err_sys("fcntl error");
    
    if (lock.l_type == F_UNLCK) // 该位置没有被其他进程锁住
        return 0;
    
    return (lock.l_pid);
}

// 使用文件记录锁锁住一个文件
// #define lockfile(fd)    write_lock((fd), 0, SEEK_SET, 0)
int lock_file(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}

ssize_t readn(int fd, void *ptr, size_t n)
{
    size_t nleft;
    ssize_t nread;

    nleft = n;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (nleft == n) // error
                return (-1);
            else
                break;
        } else if (nread == 0) {
            break; // EOF
        }
        nleft -= nread;
        ptr += nread;
    }

    return (n - nleft);
}

ssize_t writen(int fd, void *ptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;

    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) < 0) {
            if (nleft == n) // error
                return (-1);
            else
                break;
        } else if (nwritten == 0) { // write failed
            break;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    return (n - nleft);
}