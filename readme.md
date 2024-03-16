《UNIX环境高级编程》

- bookmark

- TODO:
1. gdb command line usage.
2. 学习 miwear bluetooth stack 与 bluetooth app 应用层的异步部分, 他们是怎么让异步操作正常跑起来的?

---

##### Unix Advanced Programing

##### 1. gdb configuration
编译不同目录下的 main.c 时, 只需要修改 task.json 和 launch.json 中的文件路径即可
https://zhuanlan.zhihu.com/p/92175757
.vscode 文件夹下有三个配置文件：
task.json (compiler build settings), 负责编译
launch.json (debugger settings), 负责调试
c_cpp_properties.json(compiler path and intelliSense settings),负责更改路径等设置

我分别针对不同的情况进行了配置，分别是
1.单一源文件，点击就能运行and debug
2.可执行文件，主要针对make后得到的可执行文件进行debug
3.cmake项目

##### 2.编译方法
```shell
gcc main.c common/apue.c ch1/test.c -lpthread -lrt -w -o test
# -w 忽略警告
# -lpthread 链接 thread 库
# -g debug