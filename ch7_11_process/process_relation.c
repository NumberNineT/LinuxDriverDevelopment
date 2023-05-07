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

// 进程, 父进程, 进程组, 会话(进程组集合), 作业控制(进程组集合)

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
        kill(getpid(), SIGTSTP); // stop ourself
        pr_ids("child"); // print onlyu if we're continued
        if (read(STDIN_FILENO, &c, 1) != 1) {
            err_sys("read err on controlling TTY, errno:%d", errno);
        }
    }
    exit(0);
}