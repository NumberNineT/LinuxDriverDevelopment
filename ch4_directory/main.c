#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "test.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

    // dir_test(argc, argv);
    // access_test(argc, argv);
    // file_access_test2();
    // change_file_mode_test();
    // unlink_file_test();
    symbol_link_test();


	printf("Hello, world end with error:%d\n", errno);

    return 0;
}