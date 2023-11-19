#include "../common/apue.h"
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>

/***************************************************************
 * Macro
 ***************************************************************/
#define NQ          3       // number of queues
#define MAXMSZ      512      // maximum message size
#define KEY         123   // key of first message queue
#define UNIX_DOMAIN_SOCKET_PATH     "/var/tmp/foo.test"

extern int socketpair(int domain, int type, int protocol, int sockfd[2]);

struct threadinfo {
    int qid;
    int fd;
};

struct msg {
    long msg_type;
    char data[MAXMSZ];
};

int fd_pipe(int fd[2])
{
    return (socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
}

// 消息队列不能配合 poll()/select() 使用, 因为他们是监听文件描述符的
// 套接字是和文件描述符相关的, 
// 对每个消息队列使用一个线程, 线程会在 msgrecv() 的地方阻塞, 收到数据后写入套接字, 这样就可以配合 poll()/select()使用
// 缺点:(1) 额外创建一个线程; (2) 数据拷贝两次;

// 线程入口函数
static void *helper(void *arg)
{
    int n;
    struct msg m;
    struct threadinfo *tip = arg;

    for (;;) {
        memset(&m, 0, sizeof(m));
        if ((n = msgrcv(tip->qid, &m, MAXMSZ, 0, MSG_NOERROR)) < 0)
            err_sys("msgrcv error");
        if (write(tip->fd, m.data, n) < 0)
            err_sys("write error");
    }
}

// 接收函数
void socketpair_recv_test(void)
{
    int n, i, err;
    int fd[2];
    int qid[NQ];
    struct pollfd pfd[NQ];
    struct threadinfo ti[NQ];
    pthread_t tid[NQ];
    char buf[MAXMSZ];

    for (i = 0; i < NQ; ++i) {
        if ((qid[i] = msgget((KEY + i), IPC_CREAT|0666)) < 0)
            err_ret("msgget failed");
        printf("queue ID %d is %d\n", i, qid[i]);
        // 使用数据报套接字而不是流套接字,保持消息边界,保证一次只能读取一条消息
        if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fd) < 0)
            err_ret("create socketpair failed");
        pfd[i].fd = fd[0]; // 读端
        pfd[i].events = POLLIN;
        ti[i].qid = qid[i];
        ti[i].fd = fd[1]; // 写端
        if ((err = pthread_create(&tid[i], NULL, helper, &ti[i])) != 0)
            err_ret("pthread_create error");
    }

    for (;;) {
        if (poll(pfd, NQ, -1) < 0) // 一直阻塞
            err_ret("poll error");
        for (i = 0; i < NQ; ++i) {
            if (pfd[i].revents & POLLIN) {
                if ((n = read(pfd[i].fd, buf, sizeof(buf))) < 0)
                    err_sys("read failed");
                buf[n] = 0;
                printf("queue id:%d message:%s\n", qid[i], buf);
            }
        }
    }
    exit(0);
}

void socketpair_send_test(int argc, char *argv[])
{
    key_t key;
    long qid;
    size_t nbytes;
    struct msg m;

    if (argc != 3)
        err_quit("usage:sendtest <KEY> <message>");
    key = strtol(argv[1], NULL, 0);
    if ((qid = msgget(key, 0)) < 0)
        err_quit("can't open message queue:%s\n", argv[1]);
    
    memset(&m, 0, sizeof(m));
    strncpy(m.data, argv[2], MAXMSZ-1);
    nbytes = strlen(m.data);
    m.msg_type = 1;
    if (msgsnd(qid, &m, nbytes, 0) < 0)
        err_sys("can't send message");
    exit(0);
}
 
// 将一个地址绑定到 UNIX 域套接字(命名UNIX域套接字, 这里是:UNIX_DOMAIN_SOCKET_PATH)
/* 同一台电脑通过 unix 域套接字进程间通信的方式 */
void unix_socket_test1(void)
{
    int fd, addr_len;
    struct sockaddr_un un; // unix 套接字地址结构, 与 internet 套接字地址结构不同

    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, "foo.socket");
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        err_exit("socket failed");
    addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if (bind(fd, (struct sockaddr *)&un, addr_len) < 0)
        err_exit("bind failed");
    printf("unix domain socket bound\n");
    exit(0);
}

/* 同一台电脑通过 unix 域套接字进程间通信的方式 */
// 创建 socket() -> bind() -> listen() -> accept()
void unix_socket_server_test2(void)
{
    int listen_fd, fd;
    uid_t user_id;
    int n;
    char buff[512]  = {0};

    if ((listen_fd = serv_listen(UNIX_DOMAIN_SOCKET_PATH)) < 0) 
        err_ret("listen failed:%d\n", listen_fd);

    if ((fd = serv_accept(listen_fd, &user_id)) < 0)
        err_ret("accept failed:%d\n", fd);
    
    while (1) {
        n = read(fd, buff, 512);
        buff[n] = '\0';
        printf("read len:%d data:%s\n", n, buff);
    }
}

// 命名 UNIX 域套接字
// 创建 socket() -> bind() -> connect()
void unix_socket_client_test2(void)
{
    int fd;
    char buff[512] = {0};

    if ((fd = cli_conn(UNIX_DOMAIN_SOCKET_PATH)) < 0)
        err_ret("connect failed\n");

    // 死循环:因为 uint8_t:0-255
    for (uint8_t ix = 0; ix < 256; ++ix) {
        snprintf(buff, 512, "data ix:%d\n", ix);
        printf("client send:%s\n", buff);
        write(fd, buff, strlen(buff));
        sleep(2);
    }
    exit(0);
    
}

// 服务器发送文件描述符
void send_fd_server_test1(void)
{

}

// 客户端接收文件描述符
void recv_fd_client_test1(void)
{

}
