#ifndef __TEST_H__
#define __TEST_H__

void exit_test(int argc, char *argv[]);
void env_test();
void jump_test();
void jump_test2();
void limits_test();

void process_test1();
void process_test2();
void process_test3();
void process_test4();
void rise_condition_test();

void exec_test1();
void shell_test();
void tsys_test(int argc, char *argv[]);
void system_test(void);
void uid_test(void);
void process_counter_test(void);
void get_process_info(int argc, char *argv[]);
void get_user_info(void);
void process_schedule_test(int argc, char *argv[]);
void process_times_test(int argc, char *argv[]);
void  process_times_test(int argc, char *argv[]);
int test_main(void);
void sig_test(void);
void sig_test2(void);
void sig_test3(void);
void sig_test4(void);
int sig_test5(void);
unsigned int sleep1(unsigned int seconds);
unsigned int sleep2(unsigned int seconds);
void sig_alarm_test1(void);
void sig_test6(void);
void sig_test_m(void);
void sig_test_m(void);

#endif