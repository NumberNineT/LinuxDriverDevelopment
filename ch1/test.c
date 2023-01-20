#include "test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> // calculate file operation time

/*
 *	POSIX Standard: 5.6 File Characteristics	<sys/stat.h>
 */
#include <sys/stat.h>

/*
 *	POSIX Standard: 5.1.2 Directory Operations	<dirent.h>
 */
#include <dirent.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>


/*************
 * MACRO 
 *************/
#define UNUSED_PARAM(p)       ((void)p)

#define err_quit(format, ...) printf(format", log output.", ##__VA_ARGS__)
#define err_sys(format, ...)  printf(format", log output.", ##__VA_ARGS__)

#define BUFFERSIZE            4096


void print_hello(void *param)
{
    UNUSED_PARAM(param);
    printf("%s:%d %s", __FILE__, __LINE__, __func__);
    return;
}

/**
 * @fun: file operation function test
 *       // 
     
*/
void file_operation_test()
{
    // file system operation function
    // stat()
    const char *path = "/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin";
    FILE *fp = fopen(path, "rw");
    char arr[10] = {1, 2, 3};

    if(!fp) {
        printf("open %s failed\r\n", path);
        return;
    }

    fprintf(fp, "%s", __FILE__);
    fwrite(arr, sizeof(char), sizeof(arr), fp);
    printf("write len:%ld\r\n", sizeof(arr));
    fclose(fp);
    fp = NULL;

    return;
}

/**
 * @fun: stat system call
 *       int fstat(int filedes, struct stat *buf);
 *       int stat(const char *path, struct stat *buf);
 *       int lstat(const char *path, struct stat *buf);
 *       difference:
 *       fstat() need open file to get the file pointer filedes; 
*/
void file_stat_test()
{
    struct stat st = {0};
    char *pArr = NULL;
    FILE *fp = NULL;
    const char *path = "/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin";
    clock_t clkStart, clkEnd;

    stat("/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin", &st);
    printf("%s size:%ld\r\n", path, st.st_size);
    pArr = (char*)malloc(st.st_size);

    // does not check return value temporarily
    fp = fopen("/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin", "rb");
    clkStart = clock();
    fread(pArr, sizeof(char), st.st_size, fp);
    clkEnd = clock();
    printf("read test consume time:%lu\r\n", clkEnd - clkStart);
    fclose(fp);

    fp = fopen("/home/qz/code/LinuxDriverDevelopment/ch1/test_file.bin", "rb");
    clkStart = clock();
    fwrite(pArr, sizeof(char), st.st_size, fp);
    clkEnd = clock();
    printf("read test consume time:%lu\r\n", clkEnd - clkStart);
    fclose(fp);

    return;
}


/**
 * @fun: list file int the directory
 * 
 * @param
 * @param
 * @ret:0-success; 1-255:error occur
*/
int directory_op_test(int argc, char **argv)
{
    DIR *dp;
    struct dirent *dirp;

    if(argc != 2) {
        err_quit("usage: ls directory_name\r\n");
        return 1;
    }

    if((dp = opendir(argv[1])) == NULL) {
        err_sys("Can't open %s\r\n", argv[1]);
        return 2;
    }

    while((dirp = readdir(dp)) != NULL) {
        printf("%s\r\n", dirp->d_name);
    }

    closedir(dp);

    // program normally exit
    exit(0);

    return 0;
}

/**
 * @fun:将标准输入拷贝到标准输出
 *      可以用于复制任意 UNIX 普通文件
 *      CTRL + D 退出
*/
int input_output_test()
{
    int n;
    char buf[BUFFERSIZE];

    // 每次运行一个程序, 所有的 shell 都会为其打开三个文件表述符:stdin, stdout, stderr
    // 所以不需要打开文件可以直接操作
    while(n = read(STDIN_FILENO, buf, 10)) { // 为什么会阻塞在这里????????????????????
        printf("read len:%d\r\n", n);
        if(write(STDOUT_FILENO, buf, n) != n) {
            err_sys("write error\r\n");
        }
        printf("write len:%d\r\n", n);
    }
    if(n < 0) {
        err_sys("read error\r\n");
    }

    exit(0);

    return 0;
}

int input_output_test2()
{
    int c;

    while((c = getc(stdin)) != EOF) {
        printf("read:%c\r\n", c);
        if(putc(c, stdout) == EOF) {
            err_sys("write error\r\n");
        }
        printf("write:%c\r\n", c);
    }

    exit(0);

    return 0;
}

