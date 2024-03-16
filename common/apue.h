#ifndef __APUE_H__
#define __APUE_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
/***************************************************************
 * MACRO
 ***************************************************************/
#define UNUSED_PARAM(p)       ((void)p)

#define err_quit(format, ...) { printf(format"\r\n", ##__VA_ARGS__); \
                                exit(-1);                            \
                               }
#define err_exit(format, ...) { printf(format"\r\n", ##__VA_ARGS__); \
                                exit(-1);                            \
                               }
#define err_sys(format, ...)  ( printf(format"\r\n", ##__VA_ARGS__))
#define err_ret(format, ...)  { printf(format"\r\n", ##__VA_ARGS__); \
                                return;                              \
                               }
#define err_dump(format, ...)  { printf(format"\r\n", ##__VA_ARGS__); \
                                 exit(-1);                            \
                                }

// 仅仅测试使用, 正式代码中尽量不要用 assert
#define ASSERT(cond) {                                      \
    if (!cond) {                                            \
        printf("assert %s line:%d\n", #cond, __LINE__);     \
        exit(-1);                                           \
    }                                                       \
}

#define KB                       (1024)
#define MB                       (1024 * 1024)
#define GB                       (1024 * 1024 * 1024)

#define BUFFERSIZE                (4096)
#define MAXLINE                   (1)

#define FILE_MODE                 (0666)

#ifdef PATH_MAX
static long pathmax = PATH_MAX;
#else
static long pathmax = 0;
#endif

#define PATH_MAX_GUESS            (1024)

#ifdef OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif

#define OPEN_MAX_GUESS          (256)

#define USER_TIME_TEST          (0)
#define SYSTEM_TIME_TEST        (0)
#define CHILD_TIME_TEST         (1)

#define pr_signal               pr_mask // 打印信号集

typedef void Sigfunc(int signo); // 定义函数类型?
// typedef void (Sigfunc*)(int signo); // 定义函数指针类型

// 对文件记录锁进行宏定义(w阻塞的接口)
#define read_lock(fd, offset, whence, len)      lock_reg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define readw_lock(fd, offset, whence, len)     lock_reg((fd), F_SETLKW, F_RDLCK, (offset), (whence), (len))
#define write_lock(fd, offset, whence, len)     lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define writew_lock(fd, offset, whence, len)    (lock_reg((fd), F_SETLKW, F_WRLCK, (offset), (whence), (len)))
#define un_lock(fd, offset, whence, len)        lock_reg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))
#define is_read_lockable(fd, offset, whence, len) (lock_test((fd), F_RDLCK, (offset), (whence), (len)) == 0)
#define is_write_lockable(fd, offset, whence, len) (lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

//switch-case case return string
#define CASE_RET_STR(case_num, case_str) {case case_num: return case_str;}
/*****************
 * 
 *****************/
/***************************************************************
 * DECLARATION
 ***************************************************************/
void pr_exit(int status);
void pr_mask(const char *str);
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len);
pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len);
int lock_file(int fd);

extern ssize_t readn(int fd, void *buf, size_t nbytes);
extern ssize_t writen(int fd, void *buf, size_t nbytes);

// UNIX 域套接字用于同一个计算机上的进程之间的通信
int serv_listen(const char *name);
int serv_accept(int listenfd, uid_t *uidptr);
int cli_conn(const char *name);

int send_fd(int fd, int fd_to_send);
int send_err(int fd, int errcode, const char *msg);
int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t));

// defined at socket.h
// unsigned char *CMSG_DATA(struct cmsghdr *cp);
// struct cmsghdr *CMSG_FIRSTHDR(struct msghdr *mp);
// struct cmsghdr *CMSG_NXTHDR(struct msghdr *mp, struct cmsghdr *cp);
// unsigned int CMSG_LEN(unsigned int nbytes);
/***************************************************************
 * VARIABLE
 ***************************************************************/
static long posix_version = 0;
static long xsi_version;


#endif
