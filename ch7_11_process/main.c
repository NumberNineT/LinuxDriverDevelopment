#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "test.h"


int main(int argc, char *argv[])
{
	printf("Hello, world start\n");

    // exit_test(argc, argv);
    // env_test();
    jump_test();

    if (errno != 0)
	    printf("Hello, world end with error:%d\n\n", errno);
    else
        printf("Hello, world end\n\n");

    return 0;
}