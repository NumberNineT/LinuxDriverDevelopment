#ifndef __APUE_H__
#define __APUE_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


/*************
 * MACRO 
 *************/
#define UNUSED_PARAM(p)       ((void)p)

#define err_quit(format, ...) { printf(format"\r\n", ##__VA_ARGS__); \
                                exit(-1);                            \
                               }
#define err_sys(format, ...)  ( printf(format"\r\n", ##__VA_ARGS__))
#define err_ret(format, ...)  { printf(format"\r\n", ##__VA_ARGS__); \
                                return;                              \
                               }
#define err_dump(format, ...)  { printf(format"\r\n", ##__VA_ARGS__); \
                                 exit(-1);                             \
                                }

#define BUFFERSIZE            4096
#define MAXLINE               1


#ifdef PATH_MAX
static long pathmax = PATH_MAX;
#else
static long pathmax = 0;
#endif

#define PATH_MAX_GUESS          1024

#ifdef OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif

#define OPEN_MAX_GUESS          256

#define USER_TIME_TEST          0
#define SYSTEM_TIME_TEST        0
#define CHILD_TIME_TEST         1


/*************
 * VARIABLE 
 *************/
static long posix_version = 0;
static long xsi_version;

#endif
