#include "../common/apue.h"

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

/*
 *	ISO C99 Standard: 7.14 Signal handling <signal.h>
 */
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
/* Get the system-dependent definitions of structures and bit values.  */
#include <sys/resource.h>

/*
  comp_t is a 16-bit "floating" point number with a 3-bit base 8
  exponent and a 13-bit fraction. See linux/kernel/acct.c for the
  specific encoding system used.
*/
#include <sys/acct.h>

/*****************
 * DECLARATION
 *****************/
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
pid_t fork(void);
// 调用 exec 或者 exit 前都在父进程空间运行
// 2.保证子进程先运行, exec 或者 exit 后父进程才可以运行 
pid_t vfork(void); 
pid_t wait(int *statloc); // 任一子进程退出, wait() 都会返回
pid_t waitpid(pid_t pid, int *statloc, int options); // 等待指定子进程
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
// 获取进程使用资源情况
pid_t wait3(int *statloc, int options, struct rusage *rusage);
pid_t wait4(pid_t pid, int *statloc, int options, struct rusage *rusage);
// exec()
// int execl(const char *pathname, const char *arg0, ...);
// int execv(const char *pathname, char *const argv[]);
// int execle(const char *pathname, const char *argv[0], ... /*0, char * const envp[]*/);
// int execve(const char *pathname, char * const argv[], char * const envp[]);
// int execlp(const char *filename, const char *arg0, ...);
// int execvp(const char *filename, char * const argv[]);
// int fexecve(int fd, char * const argv[], char * const envp[]);
// 实际用户 ID, 有效用户 ID, 设置用户 ID
int setuid(uid_t uid);
int setgid(gid_t gid);
int setreuid(uid_t uid, uid_t euid);
int setregid(gid_t gid, gid_t egid);
int seteuid(uid_t uid);
int setegid(gid_t gid);
// system
int ssystem(const char *cmdstring);
/* Switch process accounting on and off.  */
int acct (const char *__filename);

// 子进程是父进程的副本, 例如:子进程获得父进程数据空间, 栈 和 堆的副本
// 但是，拥有的仅仅是副本, 父子进程并不共享存储空间部分
// 父子进程共享正文段
int g_var = 6;
char buff[] = "This is a process test.\n"; // .data section
void process_test1()
{
    int var; // stack area
    pid_t pid;

    var = 88;
    if (write(STDOUT_FILENO, buff, sizeof(buff)-1) != sizeof(buff) -1) {
        err_ret("write error");
    }
    printf("before fork:\n");

    if ((pid = fork()) < 0) {
        err_ret("create porcess failed");
    } else if (pid == 0) { // child
        g_var++;
        var++;
    } else { // parent
        sleep(2); // 父子进程共享文件描述符, 父进程等待子进程操作完毕
    }

    printf("pid:%ld, g_var:%d, var:%d\n", (long)getpid(), g_var, var);
    exit(0);
}

void process_test2()
{
    int var; // stack area
    pid_t pid;

    var = 88;
    printf("before fork:\n");

    if ((pid = vfork()) < 0) {
        err_ret("vfork failed");
    } else if (pid == 0) { // child
        g_var++; // 使用父进程的数据空间
        var++;
        _exit(0); // does not flush stdout
    }

    // parent continue here
    printf("pid:%ld, g_var:%d, var:%d\n", (long)getpid(), g_var, var);
    exit(0);
}

// gcc main.c test.c ../common/apue.c
void process_test3()
{
    pid_t pid;
    int status;

    // process1
    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid == 0) {
        exit(7); // normal exit
    }

    if (wait(&status) != pid) {
        err_sys("wait error");
    }
    pr_exit(status);

    // process2
    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid == 0) {
        abort(); // SIGABRT
    }

    if (wait(&status) != pid) {
        err_sys("wait error");
    }
    pr_exit(status);

    // process3
    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid == 0) {
        status /= 0; // SIGFPE
    }

    if (wait(&status) != pid) {
        err_sys("wait error");
    }
    pr_exit(status);

    // process4
    // if ((pid = fork()) < 0) {
    //     err_ret("fork failed");
    // } else if (pid == 0) { // child
    //     printf("child\n");
    // } else { // parent
    //     sleep(1000);
    // }
}

// 一个进程 fork 一个子进程, 但不要它等待子进程终止, 也不希望子进程成为僵尸进程
// fork() 两次
void process_test4()
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid == 0) { // first child
        if ((pid = fork()) < 0) {
            err_ret("fork error");
        } else if (pid > 0) { // parent from second fork == first child
            printf("first child pid:%ld\n", (long)getpid());
            exit(0);
        }

        // 第二个子进程, 第一个"子进程"终止后(第二个进程成为孤儿进程)由 init 进程"收养"
        // 第二个子进程结束后, 进程状态由 init 进程获取
        sleep(2);
        printf("second child, parent pid:%ld\n", (long)getppid());
        exit(0); 
    } else { // main process
        printf("main process ID:%ld\n", (long)getpid());
    }

    if (waitpid(pid, NULL, 0) != pid) { // 主进程等第一个子进程退出
        err_sys("wait pid error");
    }

    exit(0);
}

static void charatatime(char *str);
#define TELL_WAIT
#define WAIT_PARENT
#define TELL_CHILD

void rise_condition_test()
{
    pid_t pid;

    // TELL_WAIT();
    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid == 0) { // child
        // WAIT_PARENT();
        charatatime("output from child\n");
    } else if (pid > 0) { // parent
        charatatime("output from parent\n");
        // TELL_CHILD(); // 让父进程先运行
    }

    exit(0);
}

static void charatatime(char *str)
{
    char *ptr;
    int c;

    setbuf(stdout, NULL); // stdout null buffer

    for (ptr = str; ptr != '\0'; ++ptr) {
        putc(*ptr, stdout);
    }

    return;
}

// exec() 只是用磁盘上的新的程序替换了当前进程的正文段, 数据段, 堆段, 栈段
char *env_init[]  = {"USER=unknown", "PATH=/tmp", NULL};
void exec_test1()
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        err_sys("fork failed");
    } else if (pid == 0) { // child
        if (execle("/home/qz/code/LinuxDriverDevelopment/ch7_11_process/test.exe", "echoall", "argv0", "argv1", (char*)0, env_init) < 0) {
            err_sys("execle failed");
        }
    } else if (pid > 0) {
        sleep(2);
    }

    if (waitpid(pid, NULL, 0) < 0) {
        err_sys("wait child process error");
    } else {
        printf("child process terminated\n");
    }

    if ((pid = fork()) < 0) {
        err_ret("fork2 error");
    } else if (pid == 0) {
        if (execlp("test.exe", "test.exe", "only 1 arg", (char*)0) < 0) {
            err_ret("excelp failed");
        }
    }

    exit(0);
}

// a standard method to create a new interpreate process
void interp_test()
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        err_ret("fork error");;
    } else if (pid == 0) { // child
        if (execl("/home/sar/bin/testinterp", "testinterp.temp", \
                "myarg1", "myarg2", (char*)0) < 0) {
                    err_ret("fork error");
                }
    }

    if (waitpid(pid, NULL, 0) < 0) { // parent
        err_sys("wait pid error");
    }

    exit(0);
}

void shell_test()
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        err_ret("fork failed");
    } else if (pid == 0) {
        printf("child pid:%ld\n", (long)getpid);
        if (execl("/bin/sh", "shell_test.sh", (char*)0) < 0) {
            err_sys("execl failed");
        }
    } else if (pid > 0) {
        printf("parent pid:%ld\n", (long)getpid);
        sleep(2);
    }

    if (waitpid(pid, NULL, 0) < 0) {
        err_sys("waitpid failed");
    }

    exit(0);
}

// system function
void system_test()
{
    int status;

    if (system(NULL) == 0) {
        printf("not support system()\n");
    } else {
        printf("support system()\n");
    }

    if ((status = system("date")) < 0) {
        err_sys("system err");
    }
    pr_exit(status);

    if ((status = system("no such command")) < 0) {
        err_sys("system err");
    }
    pr_exit(status);

    if ((status = system("who; exit 44")) < 0) {
        err_sys("system() err");
    }
    pr_exit(status);
    // system("date");
    exit(0);
}

// system function defination
// int system(const char *cmdstring)
// {
//     pid_t pid;
//     int status;

//     if (cmdstring == NULL) {
//         return 1;
//     }

//     if ((pid = fork()) < 0) {
//         err_ret("fork err");
//     } else if (pid == 0) { // child
//         execl("/bin/sh", "sh", "-c", cmdstring, (char*)0);
//         _exit(127);
//     } else { // parent
//         while (waitpid(pid, &status, 0) < 0) {
//             if (errno != EINTR) {
//                 status = -1;
//                 break;
//             }
//         }
//     }

//     return status;
// }

void uid_test()
{
    printf("real uid:%d, effiective uid:%d\n", getuid(), geteuid());
}

void process_counter_test()
{
    
}