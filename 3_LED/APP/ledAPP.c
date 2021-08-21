#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
* ./ledAPP <filename> <0:1>
* 使用方法: ./ledAPP /dev/led 0
*/
int main(int argc, char **argv)
{
    int fd;     //文件描述符
    int retval;
    char *filename = argv[1];
    unsigned char databuf[1];       //用户的数据缓冲区

    if(argc != 3){
        printf("Error Usage.\n");
        return -1;
    }

    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("Unable to open file:%s\n", filename);
        return -1;
    }

    //char to int
    databuf[0] = atoi( argv[2] ); 

    /*
    * @param1: 要写的文件
    * @param2: 要写入数据的首地址
    * @param3: 要写入数据的大小
    */
    retval = write(fd, databuf, sizeof(databuf));
    if( retval < 0){
        printf("Unable to write file.");
        close(fd);      //异常处理
        return -1;
    }

    //关闭文件
    close(fd);

    
    return 0;
}