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