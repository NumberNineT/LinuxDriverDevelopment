#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
//#include <linux/slab.h>
#include <linux/uaccess.h>


#define CHRDEVBASE_MAJOR 200
#define CHRDEVBASE_NAME  "chrdevbase"

//定义缓冲区
static char readbuf[100], writebuf[100];
static char kerneldata[100] = { "kernel data." };

//打开驱动
static int chrdevbase_open(struct inode *inode, struct file *file)
{
    printk( KERN_EMERG "open chrdevbase driver, %s was called.\n", __func__ );
    return 0;
}

//关闭驱动
static int chrdevbase_release(struct inode *inode, struct file *file)
{
    printk( KERN_EMERG "close chrdevbase driver, %s was called.\n", __func__ );
    return 0;
}

//读取
static ssize_t chrdevbase_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
    //printk( KERN_EMERG "read chrdevbase driver, %s was called.\n", __func__ );
    //不可以直接将buf指向要访问的数据
    /***
     * 驱动给应用传递数据的时候 
     * 应用程序不可以直接访问内核的数据，需要使用特定的函数***/
    int ret = 0;
    
    memcpy( readbuf, kerneldata, sizeof(kerneldata) );
    
    //static inline long copy_to_user(void __user *to, const void *front, unsigned long n)
    ret = copy_to_user(buf, readbuf, count);
    if( ret == 0 ){

    }else{

    }
    return 0;
}

//写入到内核
static ssize_t chrdevbase_write(struct file *file, const char __user *buf,
			size_t count, loff_t *ppos)
{
    //printk( KERN_EMERG "write chrdevbase driver, %s was called.\n", __func__ );
    int ret = 0;
    ret = copy_from_user( writebuf, buf, count );
    if( ret == 0 ){
        printk("kernel received:%s\n", writebuf);
    }else{
        printk("kernel receive data failed.\n");
    }

    return 0;
}

//file_operations类型的一个结构体
static struct file_operations chrdevbase_fops={
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .release = chrdevbase_release,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
};

/*
* @fun:模块入口
*/
static int __init chardevbase_init(void)
{
    printk( KERN_INFO "chardevbase driver started.%s was called.\n", __func__ );

    //注册字符设备驱动
    //int register_chrdev( unsigned int major, const char *name, const struct file_operations *fops );
    register_chrdev( CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops );

    return 0;
}
/*
* @fun:模块入口
*/ 
static void __exit chrdevbase_exit(void)
{
    printk( KERN_INFO "chardevbase driver close.%s was called.\n", __func__ );

    //卸载字符设备驱动
    //unregister_chrdev(major, VPE_MODULE_NAME);
    unregister_chrdev( CHRDEVBASE_MAJOR, CHRDEVBASE_NAME );

    return;
}
//将入口和出口函数进行折册
module_init(chardevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("qizhen");
