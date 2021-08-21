#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/cdev.h>

//与LED控制有关的寄存器的物理地址
#define CCM_CCGR1_BASE                  (0X020C406C)        //时钟使能寄存器
#define SW_MUX_GPIO1_IO03_BASE          (0X020E0068)        //IO口复用
#define SW_PAD_GPIO1_IO03_BASE          (0X020E02F4)        //设置GPIO的电气属性
#define GPIO1_DR_BASE                   (0X0209C000)        //默认IO口的输出电平
#define GPIO1_GDIR_BASE                 (0X0209C004)

//地址映射后的虚拟地址指针
//__iomem是ioremap()的返回值类型, arch/arm/include/asm/io.h
//void __iomem *ioremap(resource_size_t res_cookie, size_t size);
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LEDON       1
#define LEDOFF      0
#define NEWCHRLED_NAME      "newchrled"     //设备的名字


//LED设备结构体
//通过一个结构体来保存设备的信息
struct newchrled_dev{
    dev_t devid;
    int major;
    int minor;
};

//初始化一个设备newchrled
struct newchrled_dev newchrled;

//模块入口
static int __init newchrled_init(void)
{
    int ret = 0;

    printk(KERN_INFO "init new char device\n");
    //step1.初始化LED

    //step2.注册字符设备
    //给定了主设备号
    if(newchrled.major != 0){
        //创建一个设备
        newchrled.devid = MKDEV(newchrled.major, 0);
        //int register_chrdev_region(dev_t from, unsigned count, const char *name);
        //注册设备的数量一个
        ret = register_chrdev_region(newchrled.devid, 1, NEWCHRLED_NAME);
    }
    else{   //没有指定主设备号
        //int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name);
        //次设备号从0 开始， 申请一个
        ret = alloc_chrdev_region( &newchrled.devid, 0, 1, NEWCHRLED_NAME );
        //将获得的主设备号 和 次设备号进行赋值
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
    }

    //处理返回值
    if(ret < 0){
        printk( KERN_INFO "%s chrdev_region error!\n", NEWCHRLED_NAME );
        return -1;
    }
    else{
        printk("%s register success!\nmajor=%d !\nminor=%d !\n", NEWCHRLED_NAME);
    }

    //step3: 注册字符设备的操作集合
    return 0;
}

//模块出口
static void __exit newchrled_exit(void)
{
    printk(KERN_INFO "exit new char device\n");

    //卸载
    //void unregister_chrdev_region(dev_t from, unsigned count);
    unregister_chrdev_region(newchrled.devid, 1);

    return;
}

module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_AUTHOR("QiZhen");
MODULE_LICENSE("Dual BSD/GPL");