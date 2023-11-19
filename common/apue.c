#include "apue.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h> // struct cmsghdr
#include <sys/un.h>

#define QLEN                10
#define STALE               30 // client's name can't be older than this
#define CLI_PATH            "/var/tmp/"
#define CLI_PERM            S_IRWXU     // rwx for user only

// 传送文件描述符
#define CONTROLLEN          CMSG_LEN(sizeof(int))

// 传送证书(Linux:pid, uid, gid)
#if defined(SCM_CREDS) // BSD interface
#define CREDSTRUCT          cmsgcred
#define SCM_CREDTYPE        SCM_CREDS
#define CR_UID              cmcred_uid
#elif defined(SCM_CREDENTIALS) // Linux Interface
#define CREDSTRUCT          ucred
#define CR_UID              uid
#define CREDOPT             SO_PASSCRED
#define SCM_CREDTYPE        SCM_CREDENTIALS
#else
// #error "passing credentials is not supported"
#endif

// #define RIGHTSLEN           CMSG_LEN(sizeof(int))
// #define CREDSLEN            CMSG_LEN(sizeof(struct CREDSTRUCT))
// #define CONTROLLEN          (RIGHTSLEN + CREDSLEN)



// void *path_alloc()
// {
    
// }

static struct cmsghdr *cmptr = NULL; // malloced first time


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

/**
 * @fun 读 n 个字节, 知道出错/读到 n 个
 * @param[in] fd
 * @param[in] ptr
 * @param[in] n
 * @ret
*/
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

/**
 * @fun 写 n 个字节, 知道出错/写到 n 个
 * @param[in] fd
 * @param[in] ptr
 * @param[in] n
 * @ret
*/
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

/**
 * @fun 监听客户端的连接请求, 当一个客户端想要连接时, 会通过这个名字发起连接
 * @param[in] name 一个众所周知的文件路径名, 客户端可以通过这个路径请求连接
 * @ret 用于接收客户端连接请求的 UNIX 域套接字
*/
int serv_listen(const char *name)
{
    int fd, len, err, rval;
    struct sockaddr_un un; // unix 域套接字地址

    if (strlen(name) >= sizeof(un.sun_path)) {
        errno = ENAMETOOLONG;
        return (-1);
    }

    // create a unix domain stream socket
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return (-2);
    unlink(name); // in case it already exist

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

    // bind the name to the descriptor
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        rval = -3;
        goto errout;
    }

    if (listen(fd, QLEN) < 0) { // tell kernel we're a server, wait for client connection
        rval = -4;
        goto errout;
    } 

    return (fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return (rval);
}

/**
 * @fun 接受客户端的连接
 * @param[in] listenfd
 * @param[out] uidptr we also obtain the user ID from the pathname
 * @ret
*/
int serv_accept(int listenfd, uid_t *uidptr)
{
    int clifd, err, rval;
    socklen_t len;
    time_t staletime;
    struct sockaddr_un un;
    struct stat statbuf;
    char *name;

    if ((name = malloc(sizeof(un.sun_path) + 1)) == NULL)
        return (-1);
    len = sizeof(un);

    if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0) {
        free(name);
        return (-2);
    }

    // obatin the client's uid from its calling address
    len -= offsetof(struct sockaddr_un, sun_path); // 获取 pathname 的长度
    memcpy(name, un.sun_path, len);
    name[len] = 0;
    if (stat(name, &statbuf) < 0) {
        rval = -3;
        goto errout;
    }

#ifdef S_ISSOCK // not defined fro SVR4
    if (S_ISSOCK(statbuf.st_mode) == 0) {
        rval = -4;
        goto errout;
    }
#endif

    // 判断是否有权限
    if ((statbuf.st_mode & (S_IRWXG | S_IRWXO)) ||
        ((statbuf.st_mode & S_IRWXU) != S_IRWXU)) {
            rval = -5;
            goto errout;
        }
    
    staletime = time(NULL) - STALE;
    if (statbuf.st_atime < staletime ||
        statbuf.st_ctime < staletime ||
        statbuf.st_mtime < staletime) {
            rval = -6;
            goto errout;
        }
    
    if (uidptr != NULL) {
        *uidptr = statbuf.st_uid; // return uid of caller
    }
    unlink(name); // we're done with pathname now
    free(name);
    return (clifd);

errout:
    err = errno;
    close(clifd);
    free(name);
    errno = err;
    return (rval);
}

/**
 * @fun create a clinent end point and connect to a server.
 * @param[in] pathname
 * @ret returns fd if all OK, <0 return error
*/
int cli_conn(const char *name)
{
    int fd, len, err, rval;
    struct sockaddr_un un, sun; // unix domain socket
    int do_unlink = 0;

    if (strlen(name) >= sizeof(un.sun_path)) {
        errno = ENAMETOOLONG;
        return (-1);
    }

    // create a unix domain stream socket
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return (-1);
    
    /* fill socket address structure with our structure */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "%s%05ld", CLI_PATH, (long)getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    unlink(un.sun_path);

    if (bind(fd, (struct sockaddr*)&un, len) < 0) {
        rval = -2;
        goto errout;
    }

    if (chmod(un.sun_path, CLI_PERM) < 0) {
        rval = -3;
        do_unlink = 1;
        goto errout;
    }

    // fill socket address structre with server's address
    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
    if (connect(fd, (struct sockaddr *)&sun, len) < 0) {
        rval = -4;
        do_unlink = 1;
        goto errout;
    }

    return (fd);
errout:
    err = errno;
    close(fd);
    if (do_unlink)
        unlink(un.sun_path);
    errno = err;
    return (rval);
}

/**
 * @fun 向同一主机的其他进程发送文件描述符发生 发送错误
 *      协议:errmsg + '\0' + status
 * @param[in] fd
 * @param[in] errcode
 * @param[in] msg
 * @ret
*/
int send_err(int fd, int errcode, const char *msg)
{
    int n;

    if ((n = strlen(msg)) > 0) {
        if (writen(fd, msg, n) != n)
            return (-1);
    }

    if (errcode >= 0) // 必须是负数
        return (-1);

    if (send_fd(fd, errcode) < 0)
        return (-1);
    
    return (0);
}

/**
 * @fun 向同一主机的其他进程发送文件描述符,一般是服务器进程调用
 * @param fd UNIX 域套接字描述符
 * @param fd_to_send 要发送的描述符
*/
int send_fd(int fd, int fd_to_send)
{
    struct iovec iov[1]; // 散步读/聚集写
    struct msghdr msg;
    char buf[2]; // 2 bytes 协议数据

    iov[0].iov_base = buf;
    iov[0].iov_len = 2;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    if (fd_to_send < 0) {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        buf[1] = -fd_to_send; // 返回错误信息
        if (buf[1] == 0)
            buf[1] = 1;
    } else {
        if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
            return (-1);
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS; // 传送访问权
        cmptr->cmsg_len = CONTROLLEN;
        msg.msg_control = cmptr; // 文件描述符通过 msg_control 字段发送
        msg.msg_controllen = CONTROLLEN;
        *(int*)CMSG_DATA(cmptr) = fd_to_send; // 要发送的文件描述符
        // printf("send fd:%d\n", fd_to_send);
        buf[1] = 0; // '\0'
    }

    buf[0] = 0; // '\0'
    if (sendmsg(fd, &msg, 0) != 2) // 聚集写，通过 sendmsg() 实现传送文件描述符的midi
        return (-1);
    // printf("send 2 bytes\n");

    return (0);
}

/**
 * @fun 接受文件描述符,一般是客户端进程调用
 * @param fd UNIX 域套接字描述符
 * @param userfunc 要执行的回调函数
*/
int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t))
{
    int newfd, nr, status;
    char *ptr;
    char buf[MAXLINE];
    struct iovec iov[1];
    struct msghdr msg;

    status = -1;
    for (;;) {
        iov[0].iov_base = buf;
        iov[0].iov_len = sizeof(buf);
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        if (cmptr == NULL && (malloc(CONTROLLEN)) == NULL)
            return (-1);
        msg.msg_control = cmptr; // 文件描述符通过 msg_control 字段发送
        msg.msg_controllen = CONTROLLEN;
        if ((nr = recvmsg(fd, &msg, 0)) < 0) { // 散布读
            err_ret("recv msg error");
            return (-1);
        } else if (nr == 0) {
            err_ret("connection closed by server");
            return (-1);
        }
        printf("receive msg(%d) nr:%d msg_controllen:%d\n", msg.msg_iovlen, nr, msg.msg_controllen);
        // for (int ix = 0; ix < msg.msg_iovlen; ++ix)
        //     printf("%c\n", msg.msg_iov[ix]);

        // 判断 buffer 中是否还有其他数据可以读
        for (ptr = buf; ptr < &buf[nr]; ) {
            if (*ptr++ == 0) { // 00 + status
                // printf("recv:%s\n", *ptr);
                if (ptr != &buf[nr - 1]) // if (nrf != 2)
                    err_ret("message format error");
                status = *ptr & 0xFF;
                if (status == 0) {
                    if (msg.msg_controllen < CONTROLLEN)
                        err_ret("status = 0 but no fd");
                    newfd = *(int*)CMSG_DATA(cmptr); // 提取描述符
                    printf("recv fd:%d\n", newfd);
                } else {
                    newfd = -status;
                }
                nr -= 2;
            }
        }

        if (nr > 0 && (*userfunc)(STDERR_FILENO, buf, nr) != nr)
            return (-1);
        if (status > 0) // 接收到正确的文件描述符
            return (newfd);
    }
}

// 同时发送证书
// int send_fd2(int fd, int fd_to_send)
// {
//     struct CREDSTRUCT *credp;
//     struct cmsghdr *cmp;
//     struct iovec    iov[1];
//     struct msghdr   msg;
//     char buf[2];

//     iov[0].iov_base = buf;
//     iov[0].iov_len = 2;
//     msg.msg_iov = iov;
//     msg.msg_iovlen = 1;
//     msg.msg_name = NULL;
//     msg.msg_namelen = 0;
//     msg.msg_flags = 0;
//     if (fd_to_send < 0) { // 错误, 要发送的文件描述符小于 0
//         msg.msg_control = NULL;
//         msg.msg_controllen = 0;
//         buf[1] = -fd_to_send; // 表示错误
//         if (buf[1] == 0)
//             buf[1] = 1; //
//     } else {
//         if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
//             return (-1);
//         msg.msg_controllen = cmptr;
//         msg.msg_controllen = CONTROLLEN;
//         cmp = cmptr;
//         cmp->cmsg_level = SOL_SOCKET;
//         cmp->cmsg_type = SCM_RIGHTS;
//         cmp->cmsg_level = RIGHTSLEN;
//         *(int*)CMSG_DATA(cmp) = fd_to_send;
//         cmp = CMSG_NXTHDR(&msg, cmp);
//         cmp->cmsg_level = SOL_SOCKET;
//         cmp->cmsg_type = SCM_CREDTYPE;
//         cmp->cmsg_len = CREDSLEN;
//         credp = (struct CREDSTRUCT *)CMSG_DATA(cmp);
// #if defined(SCM_CREDENTIALS)
//         credp->uid = geteuid();
//         credp->gid = getegid();
//         credp->pid = getpid();
// #endif
//         buf[1] = 0; // 状态位
//     }

//     buf[0] = 0; // null byte flag to recv_ufd()

//     if (sendmsg(fd, &msg, 0) != 2)
//         return (-1);
    
//     return (0);
// }

// // 同时接收证书
// int recv_fd2(int fd, uid_t *uidptr, ssize_t (*userfun)(int, const void *, size_t))
// {
    
// }
