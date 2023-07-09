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