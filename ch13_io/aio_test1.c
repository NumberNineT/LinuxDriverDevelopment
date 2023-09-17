#include "../common/apue.h"
#include <aio.h>
#include <ctype.h>
#include <fcntl.h>
#include <aio.h> // -lrt
#include <sys/types.h>
#include <sys/stat.h>

#define BSZ     4096
#define NBUF    8

static unsigned char translate(unsigned char c);


enum rwop {
    UNUSED = 0,
    READ_PENDING = 1,
    WRITE_PENDING = 2
};

struct buf {
    enum rwop op;
    int last; // 当前是否是最后一包数据
    struct aiocb aiocb;
    unsigned char data[BSZ];
};

unsigned char buf[BSZ]; // translate_buffer
struct buf bufs[NBUF]; // 异步 IO 使用的 buffer, 相当于是个异步 IO 队列

//
void aio_test1(int argc, char *argv[])
{
    int ifd, ofd, i, j, n, err, numop;
    struct stat sbuf;
    const struct aiocb* aiolist[NBUF];
    off_t off = 0;

    if (argc != 3)
        err_ret("usage:test infileName outFileName");
    if ((ifd = open(argv[1], O_RDONLY)) < 0)
        err_ret("open file:%s failed", argv[1]);
    if ((ofd = open(argv[2], O_CREAT | O_RDWR | O_TRUNC, FILE_MODE)) < 0)
        err_ret("open file:%s failed", argv[2]);
    if (fstat(ifd, &sbuf) < 0)
        err_sys("fstat failed");
    
    // intialize the aio operation buffers
    for (i = 0; i < NBUF; ++i) {
        bufs[i].op = UNUSED;
        bufs[i].aiocb.aio_buf = bufs[i].data;
        bufs[i].aiocb.aio_sigevent.sigev_notify = SIGEV_NONE;
        aiolist[i] = NULL;
    }

    numop = 0;
    for (;;) {
        for (i = 0; i < NBUF; ++i) { // 遍历异步 IO 队列
            switch(bufs[i].op) {
                case UNUSED: {
                    // Read from the input file if more data remains unread
                    // 如果还有文件可以读就进入 read_pending 状态
                    if (off < sbuf.st_size) {
                        bufs[i].op = READ_PENDING;
                        bufs[i].aiocb.aio_fildes = ifd;
                        bufs[i].aiocb.aio_offset = off;
                        off += BSZ;
                        if (off >= sbuf.st_size)
                            bufs[i].last = 1;
                        bufs[i].aiocb.aio_nbytes = BSZ;
                        // aiocb 的其他成员已经在上面初始化
                        if (aio_read(&bufs[i].aiocb) < 0)
                            err_sys("aio read failed");
                        aiolist[i] = &bufs[i].aiocb;
                        ++numop;
                    }
                    break;
                }
                case READ_PENDING: {
                    // 获取异步操作的结果
                    if ((err = aio_error(&bufs[i].aiocb)) == EINPROGRESS) // aio_error() 获取异步操作的完成状态
                        continue;
                    if (err != 0) {
                        if (err == -1)
                            err_sys("aio_error failed");
                        else
                            err_exit("reaad failed");
                    }

                    // a read is complete, translete the buffer and write it to file
                    // 只有异步操作成功才可以调用 aio_return() 来获取异步操作的返回值
                    if ((n = aio_return(&bufs[i].aiocb)) < 0)
                        err_sys("aio return failed");
                    if (n != BSZ && !bufs[i].last) // 读失败
                        err_quit("short read (%d/%d)", n, BSZ);

                    // 读成功, translate
                    for (j = 0; j < n; ++j)
                        bufs[i].data[j] = translate(bufs[i].data[j]);
                    printf("aio trans:%d\n", n);

                    // 进入 write_pending 状态
                    bufs[i].op = WRITE_PENDING;
                    bufs[i].aiocb.aio_fildes = ofd;
                    bufs[i].aiocb.aio_nbytes = n;
                    if (aio_write(&bufs[i].aiocb) < 0)
                        err_sys("aio_write failed");
                    break;
                }
                case WRITE_PENDING: {
                    // 获取异步操作结果
                    if ((err = aio_error(&bufs[i].aiocb)) == EINPROGRESS)
                        continue;
                    if (err != 0) {
                        if (err == -1)
                            err_sys("aio_erro failed");
                        else
                            err_exit("write failed");
                    }
                    // write is complete
                    if ((n = aio_return(&bufs[i].aiocb)) < 0)
                        err_sys("aio_return failed");
                    if (n != bufs[i].aiocb.aio_nbytes)
                        err_quit("short write (%d/%ld)", n, bufs[i].aiocb.aio_nbytes);
                    aiolist[i] = NULL;
                    bufs[i].op = UNUSED;
                    --numop;
                    break;
                }
            }
        }
        if (numop == 0) {
            // 文件转换结束
            if (off >= sbuf.st_size)
                break;
        } else {
            // 阻塞进程
            if (aio_suspend(aiolist, NBUF, NULL) < 0)
                err_sys("aio_suspend failed");
        }
    }

    // 不等待, 强制写入存储
    bufs[0].aiocb.aio_fildes = ofd;
    if (aio_fsync(O_SYNC, &bufs[0].aiocb) < 0)
        err_sys("aio_fsync failed");

    return;
}

// 模糊化处理字符
static unsigned char translate(unsigned char c)
{
    if (isalpha(c)) {
        if (c >= 'n')
            c -= 13;
        else if (c >= 'a')
            c += 13;
        else if (c >= 'N')
            c -= 13;
        else
            c += 13;
    }

    return (c);
}

void char_process_test(int argc, char *argv[])
{
    int ifd, ofd, i, n, nw;

    if (argc != 3)
        err_ret("usage:test infileName outFileName");
    if ((ifd = open(argv[1], O_RDONLY)) < 0)
        err_ret("open file:%s failed", argv[1]);
    if ((ofd = open(argv[2], O_CREAT | O_RDWR | O_TRUNC, FILE_MODE)) < 0)
        err_ret("open file:%s failed", argv[2]);

    while ((n = read(ifd, buf, BSZ)) > 0) {
        for (i = 0; i < n; ++i)
            buf[i] = translate(buf[i]);
        if ((nw = write(ofd, buf, n)) != n) {
            if (nw < 0)
                err_sys("write failed");
            else
                err_quit("short write (%d/%d)", nw, n)
        }
        printf("trans:%d\n", n);
    }

    fsync(ofd);
    return;
}
