#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "common/apue.h"
#include "ch7_12_process/ch11.h"
#include "ch13_io/ch13.h"
#include "ch16_socket/ch16.h"
#include "ch17_IPC/ch17.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

    // exit_test(argc, argv);
    // env_test();
    // jump_test();
    // jump_test2();
    // limits_test();

    // process_test1();
    // process_test2();
    // process_test3();
    // process_test4();
    // rise_condition_test();

    // exec_test1();
    // shell_test();
    // tsys_test(argc, argv);
    // system_test();
    // uid_test();
    // process_counter_test();
    // get_process_info(argc, argv);
    // get_user_info();
    // process_schedule_test(argc, argv);
    // process_times_test(argc, argv);
    // process_times_test(argc, argv);
    // test_main();
    // sig_hup_test();
    // sig_usr_test();
    // sig_alarm_test();
    // sig_child_test();
    // sig_test4();
    // sig_test5();
    // sig_alarm_test1();
    // sig_test6();
    // my_sig_test();
    // sig_proc_mask_test1();
    // sig_test7();
    // sig_setjmp_test2();
    // sig_suspend_test1();
    // sig_suspend_test2();
    // abort_test1();
    // system_test1();
    // signal_test9();
    // sleep_test1();

    // mq_test1();

    // thread_test1();
    // thread_exit_test2();
    // thread_exit_test3();
    // thread_cleanup_test1();
    // thread_lock_test2();
    // lock_test1();
    // thread_lock_test3();
    // cond_test1();
    // barrier_test();
    // sysconf_test2();
    // thread_property_test1();
    // recursive_mutex_test1();
    // getenv_test();
    // getenv_test3();
    // thread_signal_test1();
    // fork_handler_test1();

    // daemon_process_test();
    // alread_runing();
    // io_test1();
    // file_record_test1();
    // file_lock_test(argc, argv);
    // char_process_test(argc, argv);
    // aio_test1(argc, argv);
    // aio_read_test1();
    // aio_suspend_test();
    // aio_lio_listio_test();
    // aio_signal_test();
    // aio_signal_thread_test();
    // mmap_io_test(argc, argv);
    // pipe_test1();
    // pipe_test2(argc, argv);
    // parent_child_sync_test1();
    // pager_test2(argc, argv);
    // popen_test1();
    // coprocess_test1();
    // network_ipc_test1();
    // get_addr_info_test1(argc, argv);
    // socketpair_send_test(argc, argv);
    // socketpair_recv_test();
    // unix_socket_test1();
    // unix_socket_server_test2();
    unix_socket_client_test2();
    // buf_args(NULL, NULL);
    // start_open_server_v1(NULL, NULL);
    // start_open_client_v1(NULL, NULL);


    // change_term_stat();
    // term_state_flag();
    // term_get_name();
    // tty_test();
    // strdup_test();
    // ttyname_test();
    // getpass_test();
    // unnormal_terminal_test();
    // winsize_test();
    // pty_test(argc, argv);
    // tty_test();

    if (errno != 0)
	    printf("Hello, world end with error:%d\n\n", errno);
    else
        printf("Hello, world end\n\n");

    return 0;
}