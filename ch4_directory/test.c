#include "test.h"

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	POSIX Standard: 5.6 File Characteristics	<sys/stat.h>
 */
#include <sys/stat.h>

/*
 *	POSIX Standard: 6.5 File Control Operations	<fcntl.h>
 */
#include <fcntl.h>

#include <sys/ioctl.h>

#include "../inc/apue.h"

/*************
 * MACRO 
 *************/
#define RWRWRW      (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

/*************
 * DECLARATION 
 *************/
// int stat(const char *restrict pathname, struct stat *restrict buf);
// int fstat(int fd, struct stat *buf);
// int lstat(const char *restrict pathname, struct stat *restrict buf);
// int fstat(int fd, const char *restrict pathname, struct stat * restrict buf, int flag);
// 测试文件按实际用户 ID 和 实际组 ID 进行访问权限测试:Write, Read and execute 权限
int access(const char *pathname, int mode);
int faccessat(int fd, const char *pathname, int mode, int flag);
// 屏蔽部分权限位
mode_t unmask(mode_t cmask);
// 修改文件标志位
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag); // fd:AT_FDCWD // flag:AT_SYMLINK_NOFOLLOW
// 改变文件所属用户 ID 以及用户组 ID
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int fd, const char *pathname, uid_t owner, gid_t group, int flag); // fd:AT_FDCWD(use current working directory) 
                                                                                // flag:AT_SYMLINK_NOFOLLOW
int lchown(const char *pathname, uid_t owner, gid_t group);
// 改变文件长度
int truncate(const char *pathname, off_t length);
int ftruncate(int fd, off_t length);
// link(), linkat()...
int link(const char *existingpath, const char *newpath); // 创建一个指向现有文件的链接
int linkat(int efd, const char *existingpath, int nfd, const char *newpath, int flag);
int unlink(const char *pathname);
int unlinkat(int fd, const char *pathname, int flag);
int remove(const char *pathname);
// rename
int rename(const char *oldname, const char *newname);
int renameat(int oldfd, const char * oldname, int newfd, const char *newname);
// symbol link
int symlink(const char *actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);
ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsize);
ssize_t readlinkat(int fd, const char *restrict pathname, char * restrict buf, size_t bufsize);


// struct stat
//   {
//     __dev_t st_dev;		/* Device.  */
// #ifndef __x86_64__
//     unsigned short int __pad1;
// #endif
// #if defined __x86_64__ || !defined __USE_FILE_OFFSET64
//     __ino_t st_ino;		/* File serial number.	*/
// #else
//     __ino_t __st_ino;			/* 32bit file serial number.	*/
// #endif
// #ifndef __x86_64__
//     __mode_t st_mode;			/* File mode.  */
//     __nlink_t st_nlink;			/* Link count.  */
// #else
//     __nlink_t st_nlink;		/* Link count.  */
//     __mode_t st_mode;		/* File mode.  */
// #endif
//     __uid_t st_uid;		/* User ID of the file's owner.	*/
//     __gid_t st_gid;		/* Group ID of the file's group.*/
// #ifdef __x86_64__
//     int __pad0;
// #endif
//     __dev_t st_rdev;		/* Device number, if device.  */
// #ifndef __x86_64__
//     unsigned short int __pad2;
// #endif
// #if defined __x86_64__ || !defined __USE_FILE_OFFSET64
//     __off_t st_size;			/* Size of file, in bytes.  */
// #else
//     __off64_t st_size;			/* Size of file, in bytes.  */
// #endif
//     __blksize_t st_blksize;	/* Optimal block size for I/O.  */
// #if defined __x86_64__  || !defined __USE_FILE_OFFSET64
//     __blkcnt_t st_blocks;		/* Number 512-byte blocks allocated. */
// #else
//     __blkcnt64_t st_blocks;		/* Number 512-byte blocks allocated. */
// #endif
// #ifdef __USE_XOPEN2K8
//     /* Nanosecond resolution timestamps are stored in a format
//        equivalent to 'struct timespec'.  This is the type used
//        whenever possible but the Unix namespace rules do not allow the
//        identifier 'timespec' to appear in the <sys/stat.h> header.
//        Therefore we have to handle the use of this header in strictly
//        standard-compliant sources special.  */
//     struct timespec st_atim;		/* Time of last access.  */
//     struct timespec st_mtim;		/* Time of last modification.  */
//     struct timespec st_ctim;		/* Time of last status change.  */
// # define st_atime st_atim.tv_sec	/* Backward compatibility.  */
// # define st_mtime st_mtim.tv_sec
// # define st_ctime st_ctim.tv_sec
// #else
//     __time_t st_atime;			/* Time of last access.  */
//     __syscall_ulong_t st_atimensec;	/* Nscecs of last access.  */
//     __time_t st_mtime;			/* Time of last modification.  */
//     __syscall_ulong_t st_mtimensec;	/* Nsecs of last modification.  */
//     __time_t st_ctime;			/* Time of last status change.  */
//     __syscall_ulong_t st_ctimensec;	/* Nsecs of last status change.  */
// #endif
// #ifdef __x86_64__
//     __syscall_slong_t __glibc_reserved[3];
// #else
// # ifndef __USE_FILE_OFFSET64
//     unsigned long int __glibc_reserved4;
//     unsigned long int __glibc_reserved5;
// # else
//     __ino64_t st_ino;			/* File serial number.	*/
// # endif
// #endif
//   };

/**
 * @fun: dir or file type test
*/
void dir_test(int argc, char *argv[])
{
    int i;
    struct stat buf;
    char *ptr;

    for(i = 1; i < argc; ++i) {
        printf("%s:", argv[i]);

        if(lstat(argv[i], &buf) < 0) {
            err_sys("error lstat");
            continue;
        }

        if(S_ISREG(buf.st_mode)) // S_ISREG() 在 stat.h 中定义
            ptr = "regular";
        else if(S_ISDIR(buf.st_mode))
            ptr = "directory";
        else if(S_ISCHR(buf.st_mode))
            ptr = "character";
        else if(S_ISBLK(buf.st_mode))
            ptr = "block";
        else if(S_ISFIFO(buf.st_mode))
            ptr = "FIFO";
        else if(S_ISLNK(buf.st_mode))
            ptr = "Link";
        else if(S_ISSOCK(buf.st_mode))
            ptr = "socket";
        else
            ptr = "unknown device type:%d\r\n", buf.st_mode;
        
        printf("%s\n", ptr);
    }

    exit(0);
}

/**
 * @fun: 文件访问权限测试
*/
void access_test(int argc, char *argv[])
{
    if(argc != 2) {
        err_quit("usage:./a.out <pathname>");
    }

    if(access(argv[1], R_OK) < 0) {
        err_ret("read access error for %s", argv[1]);
    } else {
        printf("read access success for %s\r\n", argv[1]);
    }

    if(open(argv[1], O_RDONLY) < 0) {
        err_ret("open %s failed", argv[1]);
    } else {
        printf("open file %s success\r\n", argv[1]);
    }

    exit(0);
}

/**
 * @fun: 创建文件时改变文件访问权限
*/
void file_access_test2()
{
    const char *path1 = "./foo";
    const char *path2 = "./bar";
    // const char *path3 = "./bar2";

    umask(0);
    if(creat(path1, RWRWRW) < 0) {
        err_sys("create file %s failed", path1);
    } else {
        printf("create file %s success\r\n", path1);
    }

    umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if(creat(path2, RWRWRW) < 0) {
        err_sys("create file %s failed", path2);
    } else {
        printf("create file %s success\r\n", path2);
    }

    // if(creat(path3, RWRWRW) < 0) {
    //     err_sys("create file %s failed", path3);
    // } else {
    //     printf("create file %s success\r\n", path3);
    // }
    // 必须要手动恢复, 否则后续创建的文件都没有这些权限
    umask(0);

    exit(0);
}

/**
 * @fun: change file mode
*/
void change_file_mode_test()
{
    struct stat statbuf;
    const char *path1 = "./foo";
    const char *path2 = "./bar";
    const char *path3 = "./bar2";

    if(stat(path1, &statbuf) < 0) {
        err_sys("get file:%s stat failed", path1);
    }

    // turn on set-group-ID and turn off group-execute
    if(chmod(path1, (statbuf.st_mode & ~S_IXGRP)) < 0) {
        err_sys("change file:%s mode failed", path1);
    }

    // rw-r--r--
    if(chmod(path2, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0) {
        err_sys("change file:%s mode failed", path2);
    }

    exit(0);
}

void unlink_file_test()
{
    const char *path = "./tempfile.txt";
    const char *newpath = "./newtempfile.txt";
    int fd;

    if((fd = open(path, O_RDWR | O_CREAT)) < 0) {
        err_sys("open file %s failed", path);
    } else {
        printf("open file %s success\r\n", path);
    }

    // if(unlink(path) < 0) {
    //     err_sys("unlink file %s failed", path);
    // } else {
    //     printf("unlink file %s success\r\n", path);
    // }

    // sleep(15);

    // if(rename(path, newpath) < 0) {
    //     err_sys("rename file %s failed", path);
    // } else {
    //     printf("rename file %s success\r\n", path);
    // }
    
    close(fd);

    if(link(path, newpath)) {
        err_sys("link file %s failed", path);
    } else {
        printf("link file %s -> %s success\r\n", path, newpath);
    }

    printf("done!\r\n");

    exit(0);
}

void symbol_link_test()
{
    const char *path = "./tempfile.txt";
    const char *symbolLinkPath = "./newtempfile.txt";
    int fd;

    // create file
    if((fd = open(path, O_RDWR | O_CREAT)) < 0) {
        err_sys("open file %s failed", path);
    } else {
        printf("open file %s success\r\n", path);
    }

    // close file
    if(close(fd) < 0) {
        err_sys("close file %s failed", path);
    } else {
        printf("close file %s success\r\n", path);
    }
    
    // create symbol link
    if(symlink(path, symbolLinkPath) < 0) {
        err_sys("symlink file %s failed", path);
    } else {
        printf("symlink file %s -> %s success\r\n", path, symbolLinkPath);
    }


}