/*
 *	POSIX Standard: 7.1-2 General Terminal Interface	<termios.h>
 */
#include <termios.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#if defined(SOLARIS)
#include <stropts.h>
#endif

#include "ch18.h"

// shell: 规范模式
// vi:非规范模式


/***************************************************************
 * Macro
 ***************************************************************/
#ifdef LINUX
#define OPTSTR  "+d:einv"
#else
#define OPTSTR  "d:einv"
#endif

struct devdir {
    struct devdir *d_next;
    char *d_name;
};
static struct termios save_termios; // save previous terminal settings
static int ttysavefd = -1;
static enum {
    RESET, 
    RAW, 
    CBREAK
} ttystate = RESET;
/***************************************************************
 * Public Function
 ***************************************************************/
extern int tcgetattr(int fd, struct termios *termptr);
extern int tcsetattr(int fd, int opt, const struct termios *termptr);

extern speed_t cfgetispeed(const struct termios *termptr);
extern speed_t cfgetospeed(const struct termios *termptr);
extern int cfsetispeed(struct termios *termptr, speed_t speed);
extern int cfsetospeed(struct termios *termptr, speed_t speed);

// 行控制函数
extern int tcdrain(int fd);
extern int fcflow(int fd, int action);
extern int fcflush(int fd, int queue);
extern int tcsendbreak(int fd, int duration);

extern char *ctermid(char *ptr);
extern int isatty(int fd);
extern char *ttyname(int fd);

extern int posix_openpt(int oflag); // 打开一个伪终端主设备
extern int grantpt(int fd); // 更改主设备 fd 对应的从设备的权限
extern int unlockpt(int fd); // 允许打开主设备 fd 对应的从设备
extern char *ptsname(int fd); // 返回从设备名字
/***************************************************************
 * Private Function Declaration
 ***************************************************************/
static char *ctermid_m(char *str);
static int isatty_m(int fd);
static char *ttyname_m(int fd);
static char *getpass_m(const char *prompt);

static void add(char *dirname);
static void cleanup(void);
static char *searchdir(char *dirname, struct stat *fdstatp);
static void sig_catch(int signo);
static void sig_winchange_hander(int signo);
static void print_window_size(int fd);
static void sig_term_handler(int signo);

static void set_noecho(int); // at the end of this file
void do_driver(char *); // int the file driver.c
void loop(int, int); // in the file loop.c
/***************************************************************
 * Public Variables
 ***************************************************************/

/***************************************************************
 * Private Variables
 ***************************************************************/
// 保存终端名字
static char ctermid_name[L_ctermid];
static struct devdir *head = NULL;
static struct devdir *tail = NULL;
static char pathname[_POSIX_PATH_MAX + 14];
static volatile sig_atomic_t sigcaught;
/**
 * @fun: 禁用中断字符 ^C, ^?
 *       文件结束符号 ^D 改为 ^B
 * 运行这个可执行程序后的效果:1. terminal 无法退出:^D 失效; 2. ^C 失效
*/
void change_term_stat(void)
{
    struct termios term;
    long vdisable; // 特殊字符处理功能是否可以被关闭

    if (isatty(STDIN_FILENO) == 0)
        err_quit("standard input is not a terminal device");
    
    // _PC_VDISABLE:returns nonzero if special character processing can be disabled
    if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0)
        err_quit("fpathcof erroror _POSIX_VDISABLE not in effect");
    
    if (tcgetattr(STDIN_FILENO, &term) < 0)
        err_sys("tcgetattr error");
    
    printf("previout:\n");
    printf("[%d]:%X\n", VINTR, term.c_cc[VINTR]); // 3
    printf("[%d]:%X\n", VEOF, term.c_cc[VEOF]); // 4
    term.c_cc[VINTR] = vdisable; // 0:disable INTR character
    term.c_cc[VEOF] = 2; // EOF is ^B
    printf("after:\n");
    printf("[%d]:%X\n", VINTR, term.c_cc[VINTR]); // 0
    printf("[%d]:%X\n", VEOF, term.c_cc[VEOF]); // 2
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0)
        err_sys("tcsetattr error");
    
    exit(0);
}

// O_TTY_INIT
// CSIZE
// stty 命令也可以检查和更改终端选项
void term_state_flag(void)
{
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) < 0)
        err_sys("tcgetattr error");
    
    switch (term.c_cflag & CSIZE) {
        case CS5:
            printf("5 bits/byte\n");
        break;
        case CS6:
            printf("6 bits/byte\n");
        break;
        case CS7:
            printf("7 bits/byte\n");
        break;
        case CS8:
            printf("8 bits/byte\n");
        break;
        default:
            printf("unknown bits/byte\n");
        break;
    }
}

void term_get_name(void)
{
    printf("terminal name:%s\n", ctermid(NULL));
}

void tty_test(void)
{
    printf("terminal name:%s\n", ttyname(STDIN_FILENO));
    // printf("fd 0:%s\n", isatty(0) ? "tty" : "not a tty");
    // printf("fd 1:%s\n", isatty(1) ? "tty" : "not a tty");
    // printf("fd 2:%s\n", isatty(2) ? "tty" : "not a tty");
    printf("fd 0:%s\n", isatty_m(0) ? "tty" : "not a tty");
    printf("fd 1:%s\n", isatty_m(1) ? "tty" : "not a tty");
    printf("fd 2:%s\n", isatty_m(2) ? "tty" : "not a tty");

    // printf("terminal name:%s\n", ctermid(NULL));

    // printf("_POSIX_PATH_MAX:%d\n", _POSIX_PATH_MAX);
}

/**
 * @fun: 获取终端的名字
 * 可能的实现
*/
static char *ctermid_m(char *str)
{
    if (str == NULL);
        str = ctermid_name;
    return (stpcpy(str, "/dev/tty"));
}

/**
 * @fun 判断一个描述符是否是终端
*/
static int isatty_m(int fd)
{
    struct termios ts;

    return (tcgetattr(fd, &ts) != -1);
}

/**
 * @fun 获取 ttyname,搜索所有的设备表项 /dev 文件夹, 寻找匹配项
 * @param[in] fd
 * @ret The function ttyname() returns a pointer to a pathname on success.  
 * On error, NULL is returned, and errno is set appropriately.  
 * The function ttyname_r() returns 0 on success,  and  an  error number upon error.
*/
static char *ttyname_m(int fd)
{
    struct stat fdstat;
    struct devdir *ddp; // directory pointer
    char *rval;

    if (isatty(fd) == 0)
        return (NULL);
    if (fstat(fd, &fdstat) < 0) // get fd status
        return (NULL);
    if (S_ISCHR(fdstat.st_mode) == 0)
        return (NULL);

    // 遍历链表
    rval = searchdir("/dev", &fdstat);
    if (rval == NULL) {
        for (ddp = head; ddp != NULL; ddp = ddp->d_next) {
            // 遍历 /dev hulu, 寻找具有相同 i 节点和设备号的表项
            if ((rval = searchdir(ddp->d_name, &fdstat)) != NULL)
                break;
        }
    }

    cleanup();

    return (rval);
}

/**
 * @fun 链表中添加一个节点
 * @param[in] dirname
 * @ret None
*/
static void add(char *dirname)
{
    struct devdir *ddp;
    int len;

    len = strlen(dirname);
    
    // skip ., .., /dev/fd
    if ((dirname[len-1] == '.') && 
        (dirname[len-2] == '/' || dirname[len-2] == '.' && dirname[len-3] == '/'))
            return;
    
    if (strcmp(dirname, "/dev/fd") == 0)
        return;
    if ((ddp = malloc(sizeof(struct devdir))) == NULL)
        return;
    if ((ddp->d_name = strdup(dirname)) == NULL) {
        free(ddp);
        return;
    }

    // linklist operation:插入到尾部
    printf("add a linklist noe:%s\n", dirname);
    ddp->d_next = NULL;
    if (tail == NULL) {
        head = ddp;
        tail = ddp;
    } else {
        tail->d_next = ddp;
        tail = ddp;
    }
}

/**
 * @fun 清空链表
*/
static void cleanup(void)
{
    struct devdir *ddp, *next_ddp;

    ddp = head;
    while (ddp != NULL) {
        next_ddp = ddp->d_next;
        free(ddp->d_name); // allocate by strdup()
        free(ddp);
        ddp = next_ddp;
    }
    head = NULL;
    tail = NULL;
    printf("cleanup\n");
}

/**
 * @fun
 * @param[in] dirname
 * @param[in] fdstatp
 * @ret
*/
static char *searchdir(char *dirname, struct stat *fdstatp)
{
    struct stat devstat;
    DIR *dp;
    int devlen;
    struct dirent *dirp;

    strcpy(pathname, dirname); // include '\0'
    if ((dp = opendir(dirname)) == NULL)
        return (NULL);
    strcat(pathname, "/"); // "/dev" -> "/dev/"
    devlen = strlen(pathname);

    // 遍历目录
    while ((dirp = readdir(dp)) != NULL) {
        strncpy(pathname + devlen, dirp->d_name, _POSIX_PATH_MAX - devlen);
        printf("pathname:%s\n", pathname);
        // skip aliases
        if (strcmp(pathname, "/dev/stdin") == 0 ||
            strcmp(pathname, "/dev/stdout") == 0 ||
            strcmp(pathname, "/dev/stderr") == 0)
                continue;
        if (stat(pathname, &devstat) < 0)
            continue;
        if (S_ISDIR(devstat.st_mode)) {
            add(pathname); // 文件夹继续遍历
            continue;
        }

        // i 节点编号和设备号匹配
        if (devstat.st_ino == fdstatp->st_ino &&
            devstat.st_dev == fdstatp->st_dev) {
            closedir(dp);
            return (pathname);
        }
    }
    closedir(dp);

    return (NULL);
}

// strdup: 拷贝字符串, 并且自动分配内存, 分配的内存需要我们手动释放
// 不使用的时候需要手动 free(), 否则会造成内存泄漏
void strdup_test(void)
{
    char path[10] = "abc";
    char *ptr = strdup(path);

    printf("ptr:%s path:%s\n", ptr, path);
    ptr[0] = 'e';
    printf("ptr:%s path:%s\n", ptr, path);
    
    free(ptr);

    return;
}

void ttyname_test(void)
{
    char *name;
    
    // 0, 1, 2
    for (int fd_ix = 0; fd_ix <= 2; ++fd_ix) {
        if (isatty(fd_ix)) {
            // name = ttyname(fd_ix);
            name = ttyname_m(fd_ix);
            if (name == NULL)
                name = "undefined";
        } else {
            name = "not a tty";
        }
        printf("fd %d name:%s\n",fd_ix, name);
    }
    exit(0);
}

void getpass_test(void)
{
    char *ptr;

    // if ((ptr = getpass("Enter a passwd:")) == NULL) {
    if ((ptr = getpass_m("Enter a passwd:")) == NULL) {
        err_sys("get passwd failed");;
    }

    // now usse passwork(probably encrypt it)
    // 最好不要保存明文密码,要进行加密操作,防止被他人窃取

    printf("passwd:%s\n", ptr);
    while (*ptr != 0) {
        *ptr++ = 0;
    }

    exit(0);
}

/**
 * @fun 规范模式(按行返回,对特殊字符进行处理):getpass() 可能的实现
 * 1.关闭回显,
 * 2.忽略 SIGINT, SIGSTOP, 所有 UNIX 版本的实现都没有忽略 SIGQUIT
 * 3.设置为无缓冲的
 * 4.读完以后恢复信号
*/
static char *getpass_m(const char *prompt)
{
#define MAX_PASS_LEN        8   // 密码最大长度
    static char buf[MAX_PASS_LEN + 1]; // '\0' at end
    char *ptr;
    sigset_t sig, old_sig;
    struct termios ts, old_ts;
    FILE *fp;
    int c;
    char *term_name = ctermid(NULL);

    printf("open terminal:%s\n", term_name);
    if ((fp = fopen(term_name, "r+")) == NULL) // 打开一个终端
        return (NULL);
    setbuf(fp, NULL); // 设置无缓冲

    sigemptyset(&sig);
    sigaddset(&sig, SIGINT);
    sigaddset(&sig, SIGTSTP);
    sigprocmask(SIG_BLOCK, &sig, &old_sig); // mask SIGINT, SIGTSTP

    tcgetattr(fileno(fp), &ts);
    old_ts = ts;
    ts.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    tcsetattr(fileno(fp), TCSAFLUSH, &ts); // 关闭回显
    fputs(prompt, fp); // 打印提示内容

    ptr = buf;
    while ((c = getc(fp)) != EOF && c != '\n') { // block
        printf("c:%c\n", c);
        if (ptr < &buf[MAX_PASS_LEN]) // 规范输入模式:锁请求的字节数已到时;读到 NL 时; 读返回
            *ptr++ = c;
        /**************** 这里为什么无论输入多少个字符,必须按完回车后终端才会显示 log, 和行缓冲有关系???*********************/
        // gdb 调试:程序是已经返回了,但是终端的运行 log 一直没有打印,这个和终端的其他有关系?
        // shell:<规范模式>,行打印或者读到指定的字节数返回
        else {
            printf("bnreak\n");
            break;
        }
    }
    *ptr = 0;
    putc('\n', fp); // we echo a newline

    tcsetattr(fileno(fp), TCSAFLUSH, &old_ts); // restore TTY state
    sigprocmask(SIG_SETMASK, &old_sig, NULL); // restore signal mask
    fclose(fp);

    printf("exit\n");
    return (buf);
}

/**
 * @fun cbreak 模式:
 * 1.非规范模式, 调用者应该捕获信号,否则可能会导致程序由于信号推出后(在信号处理函数中恢复终端设置), 终端一直处于 cbreak 模式
 * 2.关闭回显
 * 3.每次输入一个字节(VMIN设置为1即可)
*/
int tty_cbreak(int fd)
{
    int err;
    struct termios buf;

    if (ttystate != RESET) {
        errno = EINVAL;
        return (-1);
    }

    if (tcgetattr(fd, &buf) < 0)
        return (-1);
    save_termios = buf; // save previous terminal setting

    buf.c_lflag &= ~(ECHO | ICANON); // Echo off, canonical mode off
    // case B:1 byte at a time, no timer
    buf.c_cc[VMIN] = 1; // 最小字节数
    buf.c_cc[VTIME] = 0; // 没有超时时间, 如果接收到指定字节数就无限期阻塞
    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0) {
        return (-1);
    }

    // 设置完后确认一下是否设置成功
    if (tcgetattr(fd, &buf) < 0) {
        err = errno;
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = err;
        return (-1);
    }
    if ((buf.c_lflag & (ECHO | ICANON)) || 
            buf.c_cc[VMIN] != 1 ||
            buf.c_cc[VTIME] != 0) {
        // only some of changes were make, resore the original settings
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = EINVAL;
        return (-1);
    }

    ttystate = CBREAK; // save global terminal flag
    ttysavefd = fd;

    return (0);
}

/**
 * @fun 切换到 tty_raw 模式
 * 1. 非规范模式
 * 2. 关闭回显
 * 3. 禁止 CR 到 NL 的映射(ICRNL), 输入奇偶校验(INPCK), 剥离 8 bytes 的第 8 位(ISTRIP)以及输出流控制(IXON)
 * 4. 进制所有输出处理(OPOSE)
 * 5. 每次输入一个自己(MIN=1, TIME=0)
*/
int tty_raw(int fd)
{
    int err;
    struct termios buf;

    if (ttystate != RESET) {
        errno = EINVAL;
        return (-1);
    }

    if (tcgetattr(fd, &buf) < 0) {
        return (-1);
    }

    save_termios = buf; // structure copy

    buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // 关闭回显等
    buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // 其他的禁止项
    buf.c_cflag &= ~(CSIZE | PARENB); // clear size bits, parity checking off
    buf.c_cflag |= CS8; // set 8 bits per char
    buf.c_oflag &= ~(OPOST); // output processing off
    buf.c_cc[VMIN] = 1; // 1bytes
    buf.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
        return (-1);
    
    // 确认配置是否生效
    if (tcgetattr(fd, &buf) < 0) {
        err = errno;
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno =err;
        return (-1);
    }
    if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
            (buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON)) ||
            (buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 ||
            (buf.c_oflag & (OPOST)) ||
            (buf.c_cc[VMIN] != 1 || buf.c_cc[VTIME] != 0)) {
        tcsetattr(fd, TCSAFLUSH, &save_termios);
        errno = EINVAL;
        return (-1);
    }

    ttystate = RAW;
    ttysavefd = fd;

    return (0);
}

/**
 * @fun reset terminal settings
*/
int tty_reset(int fd)
{
    if (ttystate == RESET)
        return (0);
    if (tcsetattr(fd, TCSAFLUSH, &save_termios) < 0)
        return (-1);
    ttystate = RESET;

    return (0);
}

/**
 * @fun 设置退出程序(退出的时候会调用,一般用与恢复终端原始设置)
*/
void tty_atexit(void)
{
    if (ttysavefd >= 0)
        tty_reset(ttysavefd);
}


// 非规范模式, 模式切换方法:tty_cbreak() -> tty_reset() -> tty_raw()
//  * tty_raw() -> tty_reset() -> tty_cbreak()
void unnormal_terminal_test(void)
{
    int i; 
    char c;

    if (signal(SIGINT, sig_catch) == SIG_ERR)
        err_sys("register signal handler failed");
    if (signal(SIGQUIT, sig_catch) == SIG_ERR)
        err_sys("register signal handler failed");
    if (signal(SIGTERM, sig_catch) == SIG_ERR)
        err_sys("register signal handler failed");    

    if (tty_raw(STDIN_FILENO) < 0)
        err_ret("tty_raw error");
    printf("enter raw mode character, Terminate with \"DELETE\"\n");
    while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
        // if ((c &= 0xFF) == 0177) // 0177 = ASCII Delete
        if ((c &= 0xFF) == 0x04) // ^C = ASCII Delete
            break;
        printf("%o(%c)\n", c, c);
    }

    // change to cbreak mode
    if (tty_reset(STDIN_FILENO) < 0)
        err_ret("tty_reset failed");
    if (i <= 0)
        err_ret("read error");
    if (tty_cbreak(STDIN_FILENO) < 0)
        err_sys("tty_cbreak failed");
    printf("Enter cbreak mode, exit with ^C(SIGINT)\n");
    while ((i = read(STDIN_FILENO, &c, 1)) == 1) {
        c &= 0xFF;
        printf("%o(%c)\n", c, c);
    }
    if (tty_reset(STDIN_FILENO) < 0)
        err_ret("tty_reset failed");
    if (i <= 0)
        err_ret("read error");

    exit(0);
}

static void sig_catch(int signo)
{
    printf("signal caught\n");
    tty_reset(STDIN_FILENO);
    exit(0);
}

void winsize_test(void)
{
    if (isatty(STDIN_FILENO) == 0)
        exit(1);
    if (signal(SIGWINCH, sig_winchange_hander) == SIG_ERR)
        err_ret("register signal failed");
    print_window_size(STDIN_FILENO);
    for (;;)
        pause();
}

static void sig_winchange_hander(int signo)
{
    printf("signal  window size change\n");
    print_window_size(STDIN_FILENO);
}

static void print_window_size(int fd)
{
    struct winsize size;

    if (ioctl(fd, TIOCGWINSZ, (char*)&size) < 0)
        err_sys("TIOCGWINSZ error");
    printf("%d rows, %d colums\n", size.ws_row, size.ws_col);
}

/**
 * Psedo terminal - 伪终端
*/
/**
 * @fun 打开一个伪终端并且返回从设备名称
 * 由于 posix_openpt()返回的名称是保存在静态内存里的,所以需要马上拷贝出去, 否则会被覆盖
 * @param[out] pts_name 保存从设备名称
 * @param[in] pts_namesz
 * @ret 返回主设备 fd
*/
int ptym_open(char *pts_name, int pts_namesz)
{
    char *ptr;
    int fdm, err;

    if ((fdm = posix_openpt(O_RDWR)) < 0)
        return (-1);
    if (grantpt(fdm) < 0) // grant access to slave
        goto errout;
    if (unlockpt(fdm) < 0) // clear slave's lock flag
        goto errout;
    if ((ptr = ptsname(fdm)) == NULL)
        goto errout;
    
    // copy name of slave
    strncpy(pts_name, ptr, pts_namesz);
    pts_name[pts_namesz - 1] = '\0';
    return (fdm);
errout:
    err = errno;
    close(fdm);
    errno = err;
    return (-1);
}

/**
 * @fun 打开从设备
 *      从设备必须在 unlockpt() 之后才可以打开
 * @param[in] pts_name
*/
int ptys_open(char *pts_name)
{
    int fds;
#if defined(SOLARIS)
    int err, setup;
#endif

    if ((fds = open(pts_name, O_RDWR)) < 0)
        return (-1);
#if defined(SOLARIS)
    // check if stream is already setup by auto push facility
    if ((setup = ioctl(fds, I_FIND, "ldterm")) < 0)
        goto errout;

    // SOLARIS 需要手动添加一些 stream 模块
    if (setup == 0) {
        if (ioctl(fds, I_PUSH, "ptem") < 0)
            goto errout;
        if (ioctl(fds, I_PUSH, "ldterm") < 0)
            goto errout;
        if (ioctl(fds, I_PUSH, "ttcompat") < 0)
            goto errout;    
    }

errout:
    err = errno;
    close(fds);
    errno = err;
    return (-1);
#endif

    return (0);
}

/**
 * @fun fork() 调用打开主设备和从设备, 创建作为会话首进程的子进程 并使其具有控制终端
 * @param[out] ptrfdm 主设备描述符
 * @param[out] slave_name
 * @param[in] slave_namesz
 * @param[in] slave_termios 指定一些终端的属性
 * @param[in] slave_winsize 指定终端大小
 * @ret pid
*/
pid_t pty_fork(int *ptrfdm, char *slave_name, int slave_namesz,
                const struct termios *slave_termios, const struct termios *slave_winsize)
{
    int fdm, fds;
    pid_t pid;
    char pts_name[20];

    if ((fdm = ptym_open(pts_name, sizeof(pts_name))) < 0)
        err_sys("Can't open master pty:%s err:%d\n", pts_name, fdm);
    if (slave_name != NULL) {
        strncpy(slave_name, pts_name, slave_namesz);
        slave_name[slave_namesz - 1] = '\0';
    }

    if ((pid = fork()) < 0) {
        return (-1);
    } else if (pid == 0) { // child
        if (setsid() < 0) // 创建子进程作为会话首进程
            err_sys("setsid error");

        // 控制终端, 打开从设备
        if ((fds = ptys_open(pts_name)) < 0)
            err_sys("can't open slave");
        close(fdm); // all donw with master in child
#if defined(BSD)
        // TIOCSCTTY is the BSD way to acquire a controlling terminal
        if (ioctl(fds, TIOCSCTTY, (char*)0) < 0)
            err_sys("TIOCSCTTY error");
#endif
        
        // set slave's termios and window size
        if (slave_termios != NULL) {
            if (tcsetattr(fds, TCSANOW, &slave_termios) < 0)
                err_sys("tcssetattr error on slave pty");
        }
        if (slave_winsize != NULL) {
            if (ioctl(fds, TIOCSWINSZ, slave_winsize) < 0)
                err_sys("TIOCSWINSZ error on slave pty");
        }

        // slave becomes stdin/stdout/stderr of child
        if (dup2(fds, STDIN_FILENO) != STDIN_FILENO)
            err_sys("dup2 error to stdin");
        if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO)
            err_sys("dup2 error to stdin");
        if (dup2(fds, STDERR_FILENO) != STDERR_FILENO)
            err_sys("dup2 error to stdin");
        if (fds != STDIN_FILENO && fds != STDOUT_FILENO && fds != STDERR_FILENO) {
            printf("close fds:%d\n", fds);
            close(fds);
        }
        return (0); // child return 0 just like fork() return
    } else { // parent
        *ptrfdm = fdm;
        return (pid); // parent returns pid of child
    }
}

/**
 * @fun 使用 pty 程序可以用 pty prog arg1 arg2 代替 prog arg1 arg2
 * 使用 pty 来执行程序
*/
int pty_test(int argc, char *argv[])
{
    int fdm;
    int c, ignoreeof, interactive, noecho, verbose;
    pid_t pid;
    char *driver;
    char slave_name[20];
    struct termios orig_termios;
    struct winsize size;

    interactive = isatty(STDIN_FILENO);
    ignoreeof = 0; // 一些选项的初始化
    noecho = 0;
    verbose = 0;
    driver = NULL;
    
    opterr = 0;
    while ((c = getopt(argc, argv, OPTSTR)) != EOF) {
        switch (c) {
            case 'd': // driver for stdin/stdout
                driver = optarg;
                break;
            case 'e': // no echo for slave pty's line discipline
                noecho = 1;
                break;
            case 'i': // ignore EOF on standard input
                ignoreeof = 1;
                break;
            case 'n': // not interactive
                interactive = 0;
                break;
            case 'v': // verbose
                verbose = 1;
                break;
            case '?':
                err_quit("unrecognized option:-%c", optopt);
        }
    }
    if (optind >= argc)
        err_quit("usage:pty [-d driver -eint] program [arg...]");
    
    if (interactive) { // fetch current termios and window size
        if (tcgetattr(STDIN_FILENO, &orig_termios) < 0)
            err_quit("tcgetattr() error on stdin");
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char*)&size) < 0)
            err_quit("get window size failed");
        pid = pty_fork(&fdm, slave_name, sizeof(slave_name), &orig_termios, &size); // 带参数
    } else {
        pid = pty_fork(&fdm, slave_name, sizeof(slave_name), NULL, NULL);
    }

    if (pid < 0) {
        err_sys("fork error");
    } else if (pid == 0) { // child
        if (noecho)
            set_noecho(STDIN_FILENO);
        if (execvp(argv[optind], &argv[optind]) < 0)
            err_sys("can't execute:%s", argv[optind]);
    }
    // else { // parent
    // }
    
    // 对不同的参数进行设置
    fprintf(stderr, "parent running\n");
    if (verbose) {
        fprintf(stderr, "slave name = %s\n", slave_name);
        if (driver != NULL)
            fprintf(stderr, "driver = %s\n", driver);
    }

    if (interactive && driver == NULL) {
        if (tty_raw(STDIN_FILENO) < 0) // 非规范模式
            err_sys("tty_raw error");
        if (atexit(tty_atexit) < 0)
            err_sys("atexit error");
    }
    if (driver)
        do_driver(driver); // changes our stdin/stdout
    loop(fdm, ignoreeof); // copies stdin->ptym, ptym->stdout

    exit(0);
}

/**
 * @fun 关闭从设备回显
 * @param[in] fd
 * @ret
*/
static void set_noecho(int fd)
{
    struct termios stermios;
    
    if (tcgetattr(fd, &stermios) < 0)
        err_sys("tcgetattr error");
    stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    stermios.c_oflag &= ~(ONLCR); // turn off NL to CR/NL mapping on output
    if (tcsetattr(fd, TCSANOW, &stermios) < 0)
        err_sys("tcsetattr error");
}

void do_driver(char *driver)
{
    fprintf(stderr, "driver:%s\n", driver);
}

/**
 * @fun 从标准输入接收所有内容复制到 pty 主设备,并将 pty 主设备接收到的所有数据复制到标准输出
 * 可以使用 select()/poll()实现, 这里使用两个进程实现
*/
void loop(int ptym, int ignoreeof)
{
    pid_t child;
    int nread;
    char buf[BUFFERSIZE];

    if ((child = fork()) < 0)
        err_sys("fork error");
    else if (child == 0) { // child copy stdin to ptym
        for (;;) {
            if ((nread = read(STDIN_FILENO, buf, BUFFERSIZE)) < 0)
                err_sys("read error from stdin");
            else if (nread == 0) // EOF on stdin means we're done
                break;
            if (writen(ptym, buf, nread) != nread)
                err_sys("writen faield");
        }

        // 通知父进程
        if (ignoreeof == 0)
            kill(getppid(), SIGTERM);
        exit(0); // terminate; child process can't return
    }
    
    // else { // parent
    // }
    // parent copies ptym to stdout
    if (signal(SIGTERM, sig_term_handler) == SIG_ERR)
        err_sys("signal_inr error on SIGTERM");
    for (;;) {
        if ((nread = read(ptym, buf, BUFFERSIZE)) <= 0)
            break;
        if (writen(STDOUT_FILENO, buf, nread) != nread)
            err_sys("writen failed");
    }

    // 1. sig_term_handler() 捕获到 SIG_TERM
    // 2. pty master 读到了 EOF
    // 3. error occur
    if (sigcaught == 0) // tell child if it didn't send us the signal 
        kill(child, SIGTERM);
    // parent return to caller
}

static void sig_term_handler(int signo)
{
    sigcaught = 1;
}