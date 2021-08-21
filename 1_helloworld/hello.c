#include <linux/init.h>
#include <linux/module.h>

//模块初始化
static int __init hello_init(void)
{
    //KERN_INFO，表示了要打印的消息的等级，KERN_ERR, KERN_NOTICE, KERN_等
    printk(KERN_INFO "Enter %s\n", __func__);	//__func__代表函数名称
    return 0;	//linux中，return 0 表示正常返回，返回其他数字表示异常
}

//模块退出
static void __exit hello_exit(void)
{
    printk( KERN_INFO "Enter:%s\n", __func__ );
}


module_init(hello_init);	//模块插入的时候会自动调用hello_init这个函数(回调函数，函数指针)
module_exit(hello_exit);

MODULE_AUTHOR("QiZhen");
MODULE_LICENSE("Dual BSD/GPL");
