#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @desc简单写一个应用程序调用写好的驱动程序
 * @argv: ./chrdevbaseAPP filename <1, 2> 
 * @desc:1表示从驱动读取数据， 2表示写到驱动
*/
int main( int argc, char **argv )
{
    int ret = 0;
    int fd = 0;
    char *filename = argv[1];
    char readbuf[50], writebuf[50];
    char usrdata[] = {"usr data!"};

    if(argc != 3){
        printf("Error usage.\n");
        return -1;
    }

    fd = open( filename, O_RDWR );

    if(fd < 0){
        printf("open file:%s failed.\n", filename);
        return -1;
    }

    if( atoi(argv[2]) == 1 ){
        //从设备读内容
        //ssize_t read(int fd, void *buf, size_t count);
        ret = read(fd, &readbuf, 50);
        if( ret < 0 ){
            printf("read file %s failed.\n", filename);
            return -1;
        }
        else{
            printf("APP read data : %s.\n", readbuf);
        }
    }
    else if( atoi(argv[2]) == 2 ){
        //写内容
        memcpy(writebuf, usrdata, sizeof(usrdata));
        //ssize_t write(int fd, const void *buf, size_t count);
        ret = write(fd, &writebuf, 50);
        if( ret < 0 ){
            printf("Can't write %s.\n", filename);
            return -1;
        }
        else{
            printf("APP write data : %s.\n", writebuf);
        }
    }
    else{
        printf("Error input argv[2]\n");
    }
    
    //最后关闭设备
    //int close(int fd);
    ret = close(fd);
    if( ret < 0 ){
        printf("Can't close.\n");
        return -1;
    }
    else{
        printf("close %s success.\n", filename);
    }

    return 0;
}