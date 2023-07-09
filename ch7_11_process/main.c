#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "test.h"


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
    // sig_usr_test();
    // sig_alarm_test();
    sig_child_test();
    // sig_test4();
    // sig_test5();
    // sig_alarm_test1();
    // sig_test6();
    // sig_test_m();


    if (errno != 0)
	    printf("Hello, world end with error:%d\n\n", errno);
    else
        printf("Hello, world end\n\n");

    return 0;
}