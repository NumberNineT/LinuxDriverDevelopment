#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "common/apue.h"
#include "ch7_12_process/ch11.h"
#include "ch13_io/ch13.h"


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
    mmap_io_test(argc, argv);
    
    if (errno != 0)
	    printf("Hello, world end with error:%d\n\n", errno);
    else
        printf("Hello, world end\n\n");

    return 0;
}