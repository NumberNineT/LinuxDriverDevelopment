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

/*
 *	POSIX Standard: 9.2.2 User Database Access	<pwd.h>
 */

#include <pwd.h>

/*
 *	POSIX Standard: 4.5.2 Process Times	<sys/times.h>
 */
#include <sys/times.h>

/*
 *	POSIX Standard: 7.1-2 General Terminal Interface	<termios.h>
 */
#include <termios.h>

/*
 *	ISO C99 Standard: 7.14 Signal handling <signal.h>
 */
#include <signal.h>


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
int acct(const char *__filename);
// 用户标识
extern struct passwd *getpwuid (__uid_t __uid);
extern struct passwd *getpwnam(const char *name);
// 进程调度
extern int nice (int __inc); // nice 越小, 进程优先级越高;只能修改自己的优先级
extern int getpriority(int which, id_t who);
extern int setpriority(int which, id_t who, int value);
// 进程时间
clock_t times(struct tms *buf);
// 进程组
pid_t getpgrp(void); // 获取当前进程所属进程组 ID
pid_t getpgid(pid_t pid); // 获取指定进程的进程组 ID
int setpgid(pid_t pid, pid_t pgid); // 创建一个进程组/将进程转移到其他进程组(设置进程组ID)
int setpgrp(pid_t pid); // setpgrp(0) == getpgrp()
pid_t setsid(void); // 创建一个会话
pid_t getsid(pid_t pid); // 获取会话 ID(会话首进程的进程组 ID)
// 会话
pid_t tcgetprrp(int fd); // 返回前台进程组 ID
int tcsetpgrp(int fd, pid_t pgrpid); // 设置前台进程组
pid_t tcgetsid(int fd); // 获取会话首进程 ID-
// 信号

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
        // status /= 0; // SIGFPE
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

void tsys_test(int argc, char *argv[])
{
    if (argc < 2) {
        err_quit("param not enough:%d", argc);
    }

    int status;
    if ((status = system(argv[1])) < 0) {
        err_sys("system error");
    }
    pr_exit(status);
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

// 打印实际用户 ID 和有效用户 ID
void uid_test()
{
    printf("real uid:%d, effiective uid:%d\n", getuid(), geteuid());
}

void process_counter_test()
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        err_sys("fork error");
    } else if (pid != 0) { // parent
        sleep(2);
        exit(2); // terminate with 2
    }

    if ((pid = fork()) < 0) { // first child
        err_sys("fork error");
    } else if (pid != 0) {
        sleep(4);
        abort(); // terminate with core dump
    }

    if ((pid = fork()) < 0) { // second child
        err_sys("fork error");
    } else if (pid != 0) {
        execl("/bin/dd", "dd", "if=/etc/libao.conf", "of=/dev/null", NULL);
        exit(7); // shouldn't get here
    }

    if ((pid = fork()) < 0) { // third child
        err_sys("fork error");
    } else if (pid != 0) {
        sleep(8);
        exit(0); // normal exit
    }

    sleep(6); // fourth child
    kill(getpid(), SIGKILL); // terminate with SIGKILL
    exit(6); // shouldn't get here
}

// 获取进程会计记录信息
#if defined(BSD)
#define acct acctv2
#define ac_flag ac_trailer.ac_flag
#define FMT "%-*.*s e = %6ld, chars = %.0f %c %c %c %c\n"
#elif defined(HAS_AC_STAT)
#define FMT "%-*.*s e = %6ld, chars = %7ld, stat = %3u: %c %c %c %c\n"
#else
#define FMT "%-*.*s e = %6ld, chars = %7ld, %c %c %c %c\n"
#endif

#if defined(LINUX)
#define acct acct_v3
#endif

#if !defined(HAS_ACORE)
#define ACORE 0
#endif

#if !defined(HAX_AXSIG)
#define AXSIG 0
#endif

#if !defined(BSD)
// convert compile time to unsigned long
static unsigned long compt2ulong(comp_t comptime)
{
    unsigned long val;
    int exp;

    val = comptime & 0x1FFF; // 地13位
    exp = (comptime >> 13) & 7; // 高三位
    while (exp-- > 0)
        val *= 8;

    return val;
}
#endif

void get_process_info(int argc, char *argv[])
{
//     FILE *fp;
//     struct acct acdata;

//     if (argc != 2) {
//         err_ret("param not enough:%d", argc);
//     }

//     if (fp = fopen(argv[1], "r") == NULL) {
//         err_sys("can't open file:%s", argv[1]);
//     }

//     while (fread(&acdata, sizeof(struct acct), 1, fp) == 1) {
//         printf(FMT, (int)sizeof(acdata.ac_comm), acdata.ac_comm,
// #if defined(BSD)
//                 acdata.ac_etime, acdata.ac_io,
// #else
//                 compt2ulong(acdata.ac_etime), compt2ulong(acdata.ac_io),
// #endif
// #if defined(HAS_AC_STAT)
//                 (unsigned char)acdata.ac_stat,
// #endif
//                 acdata.ac_flag & ACORE ? 'D' : ' ',
//                 acdata.ac_flag & AXSIG ? 'X' : ' ',
//                 acdata.ac_flag & AFORK ? 'F' : ' ',
//                 acdata.ac_flag & ASU ? 'S' : ' ');
//     }

//     if (ferror(fp)) {
//         err_sys("read error");
//     }
//     exit(0);
}

void get_user_info()
{
    struct passwd *pw;

    pw = getpwuid(getuid());
    if (!pw) {
        err_sys("getpwuid failed");
    }
    printf("name:%s uid:%d gid:%d\n", pw->pw_name, pw->pw_uid, pw->pw_gid);

    pw = getpwnam("qz");
    if (!pw) {
        err_sys("getpwnam failed");
    }
    printf("name:%s uid:%d gid:%d\n", pw->pw_name, pw->pw_uid, pw->pw_gid);

    printf("login name:%s\n", getlogin()); 
}

// 进程调度实验
#if defined(MACOS)
#include <sys/syslimits.h>
#elif defined(SOLARSIS)
#include <limits.h>
#elif defined(BSD)
#include <sys/param.h>
#endif
#include <sys/time.h>

unsigned long long count;
struct timeval end;
static void checktime(char *str)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    if (tv.tv_sec >= end.tv_sec && tv.tv_usec >= end.tv_usec) {
        printf("%s conunt = %lld\n", str, count);
        exit(0);
    }

    return;
}

void process_schedule_test(int argc, char *argv[])
{
    pid_t pid;
    char *s;
    int nzero, ret;
    int adj = 0;

    setbuf(stdout, NULL); // 无缓冲

#if defined(NZERO)
    nzero = NZERO;
#elif defined(_SC_NZERO)
    nzero = sysconf(_SC_NZERO);
    printf("nzero:%d\n", nzero);
#else
#error NZERO undefined
#endif

    if (argc == 2) {
        adj = strtol(argv[1], NULL, 10);
    }
    gettimeofday(&end, NULL);
    end.tv_sec += 5; // run for 10 secs

    if ((pid = fork()) < 0) {
        err_ret("fork err");
    } else if (pid == 0) { // child
        s = "child1";
        printf("current child nice:%d, addjusted:%d\n", nice(0), adj);
        errno = 0;
        if ((ret = nice(adj)) == -1 && errno != 0) {
            err_ret("child nice failed");
        }
        printf("now child nice:%d\n", ret);
        // 没有结束
    } else { // parent
        s = "parent";
        printf("parent nice:%d\n", nice(0)); // ???? + nzero
        // 没有结束
    }

    // 父进程子进程都会走到这里
    for (;;) {
        if (++count == 0) {
            err_quit("%s counter wrap", s);
        }
        checktime(s);
    }
}

static void tick_per_sec()
{
    printf("tick per sec:%ld\n", sysconf(_SC_CLK_TCK));
}
static void pr_times(clock_t real, struct tms *tmsstart, struct tms *tmsend)
{
    static long clktck = 0;
    if (clktck == 0) { // get clock per second
        if ((clktck = sysconf(_SC_CLK_TCK)) < 0) {
            err_ret("sysconf failed");
        }
    }

    // printf("%ld %ld %ld %ld\n", tmsstart->tms_utime, tmsstart->tms_stime, tmsstart->tms_cutime, tmsstart->tms_cstime);
    printf("real:%7.2fs\n", real / (double)clktck);
    printf("user:%7.2fs\n", (tmsend->tms_utime - tmsstart->tms_utime) / (double)clktck); // 用户 CPU 时间
    printf("system:%7.2fs\n", (tmsend->tms_stime - tmsstart->tms_stime) / (double)clktck); // 系统 CPU 时间
    printf("child user:%7.2fs\n", (tmsend->tms_cutime - tmsstart->tms_cutime) / (double)clktck); // 已终止子进程用户 CPU 时间
    printf("child user:%7.2fs\n", (tmsend->tms_cstime - tmsstart->tms_cstime) / (double)clktck); // 已终止子进程用户 CPU 时间
}
static void do_cmd(char *cmd)
{
    struct tms tmsstart, tmsend;
    clock_t start, end;
    int status;
    
    // times() 返回墙上时间
    if ((start = times(&tmsstart)) == -1) {
        err_ret("times failed\n");
    }
    printf("\ncommand:%s\n", cmd);
    if ((status = system(cmd)) < 0) {
        err_ret("run command:%s failed", cmd);
    }
    if ((end = times(&tmsend)) == -1) {
        err_ret("times failed\n");
    }
    pr_times(end-start, &tmsstart, &tmsend);
    pr_exit(status);
}
void  process_times_test(int argc, char *argv[])
{
    int i;

    tick_per_sec();
    setbuf(stdout, NULL); // no buffer
    for (i = 1; i < argc; ++i) {
        do_cmd(argv[i]);
    }
    exit(0);
}

void process_group_test()
{
    // 每个进程组都有一个组长进程
    // setpgrp() 可以用来创建一个新的进程组 或者 让当前进程/子进程加入进程组

}