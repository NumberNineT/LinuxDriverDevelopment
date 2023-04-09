#include "../common/apue.h"

#include <stdio.h>
#include <stdlib.h>

/*
 *	POSIX Standard: 2.10 Symbolic Constants		<unistd.h>
 */
#include <unistd.h>

/*
 *	ISO C99 Standard: 7.13 Nonlocal jumps	<setjmp.h>
 */
#include <setjmp.h>

#include <sys/resource.h>

// 进程终止
void exit(int status); // status 进程退出的终止状态; 退出时关闭标准 IO
void _Exit(int status);
void _exit(int status);
int atexit(void (*func)(void));
// memory
void *malloc(size_t size);
void *calloc(size_t nobj, size_t size);
void *realloc(void *ptr, size_t newsize);
void free(void *ptr);
// enviroment variable - environ 变量
char *getenv(const char *name);
int putenv(char *str);
int setenv(const char *name, const char *value, int rewrite);
int unsetenv(const char *name);
// Nonlocal jump
int setjump(jmp_buf env);
void longjmp(jmp_buf env, int val);
// process resource
int getrlimit(int resource, struct rlimit *rlptr);
int setrlimit(int resource, const struct rlimit *rlptr);


/*****************
 * DECLARATION
 *****************/
static void my_exit1(void);
static void my_exit2(void);

void exit_test(int argc, char *argv[])
{
    // exit() 调用顺序与注册顺序相反: 栈
    if (atexit(my_exit1) != 0) {
        err_ret("Can't register exit handler");
    }
    if (atexit(my_exit2) != 0) {
        err_ret("Can't register exit handler");
    }

    // 命令行参数, NULL 结尾
    for (int ix = 0; argv[ix] != NULL; ++ix) {
        printf("argv[%d]:%s\n", ix, argv[ix]);
    }


}

static void my_exit1(void)
{
    printf("first exit handler:%s\n", __func__);
}

static void my_exit2(void)
{
    printf("second exit handler:%s\n", __func__);
}

void env_test()
{
    char *key = "PATH";
    char *value;

    if ((value = getenv("PATH")) == NULL) {
        err_ret("getenv failed");
    }
    printf("%s:%s\n", key, value);

    if (putenv("TEST_ENV_PATH=/user/include") != 0) {
        err_ret("putenv failed");
    }

    key = (char*)malloc(200);
    strcpy(key, "TEST_EVN_PATH2");

    value = (char*)malloc(200);
    strcpy(value, "/user/include2");

    if (setenv(key, value, 0) != 0) {
        err_ret("set env failed");
    }

    // check
    if ((value = getenv("TEST_ENV_PATH")) == NULL) {
        err_ret("getenv failed");
    }
    printf("TEST_ENV_PATH:%s\n", value);
    // 这内存被添加到环境变量表使用, 不可以释放??
    // free(value);

    if ((value = getenv(key)) == NULL) {
        err_ret("getenv failed");
    }
    printf("%s:%s\n", key, value);  

    // free(key);

    return;
}

// 输入命令然后进解析框架
#define TOKEN_ADD   5
static void do_line(char *ptr);
static void cmd_add(void);
static int get_token(void);
char *tok_ptr;
jmp_buf jmpbuffer;
void jump_test()
{
    char line[BUFFERSIZE];

    if (setjmp(jmpbuffer) != 0) {
        err_ret("setjmp failed");
    }

    while (fgets(line, BUFFERSIZE, stdin) != NULL) {
        do_line(line);
    }
    exit(0);
}

// process one line of input
static void do_line(char *ptr)
{
    int cmd;

    tok_ptr = ptr;
    printf("ptr:%s\n", ptr);
    while ((cmd = get_token()) > 0) {
        switch (cmd) {
        case TOKEN_ADD: {
            cmd_add();
            break;
        }
        }
    }
}

static void cmd_add(void)
{
    int token;

    token = get_token();
    if (token < 0) {
        printf("jump->\n\n");
        longjmp(jmpbuffer, 1);
    }
}

static int get_token(void)
{
    return -1;
}

// 回滚时:自动变量, 寄存器变量的值是不确定的
// volatile 的值不变
// 全局变量/静态局部变量值不变
// static jmp_buf jmpbuffer;
static int globval;
static void display_var(int g, int a, int r, int v, int s);
static void f1(int a, int r, int v, int s);
void jump_test2()
{
    int autoval;
    register int regival;
    volatile int volaval;
    static int statval;

    globval = 1;
    autoval = 2;
    regival = 3;
    volaval = 4;
    statval = 5;
    if (setjmp(jmpbuffer) != 0) {
        printf("after long jump:\n");
        display_var(globval, autoval, regival, volaval, statval);
        exit(0);
    }

    // change variable before longjump
    globval = 101;
    autoval = 102;
    regival = 103;
    volaval = 104;
    statval = 105;
    f1(autoval, regival, volaval, statval);
    exit(0);
}

static void display_var(int g, int a, int r, int v, int s)
{
    printf("globval:%d\n", g);
    printf("autoval:%d\n", a);
    printf("regival:%d\n", r);
    printf("volaval:%d\n", v);
    printf("statval:%d\n", s);
}

static void f1(int a, int r, int v, int s)
{
    printf("f1():\n");
    display_var(globval, a, r, v, s);
    longjmp(jmpbuffer, 1);
}

#define doit(name) pr_limits(#name, name)
static void pr_limits(char *name, int resource);
void limits_test()
{
#ifdef RLIMIT_AS
    doit(RLIMIT_AS);
#endif
    doit(RLIMIT_CORE);
    doit(RLIMIT_CPU);
    doit(RLIMIT_DATA);
    doit(RLIMIT_FSIZE);
#ifdef RLIMIT_MEMLOCK
    doit(RLIMIT_MEMLOCK);
#endif
}

static void pr_limits(char *name, int resource)
{
    struct rlimit limit;
    unsigned long long lim;

    if (getrlimit(resource, &limit)) {
        err_ret("getrlimit() error");
    }
    printf("%-14s ", name);

    if (limit.rlim_cur == RLIM_INFINITY) {
        printf("(infinite)  ");
    } else {
        lim = limit.rlim_cur;
        printf("%20d  ", lim);
    }

    if (limit.rlim_max == RLIM_INFINITY) {
        printf("(infinite)");
    } else {
        lim = limit.rlim_max;
        printf("%20d", lim);
    }

    putchar((int)'\n');

    return;
}
