#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>

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
#define NEWCHRLED_COUNT     1           //要申请的次设备号的个数

//LED设备结构体
//通过一个结构体来保存设备的信息
struct newchrled_dev{
    struct cdev c_dev;  //字符设备
    dev_t devid;        //设备号
    struct class *class;    //类这个结构体
    struct device *device;  //设备，指针
    int major;          //主设备号
    int minor;          //次设备号
};

//初始化一个设备newchrled
struct newchrled_dev newchrled;

//打开 / 关闭LED的函数
void led_switch( u8 stat )
{
    u32 val = 0;
    if( stat == LEDON ){
        val = readl(GPIO1_DR);  //读取虚拟地址
        val &= ~(1 << 3);       
        writel( val, GPIO1_GDIR_BASE );
    }
    else if( stat == LEDOFF ){
        val = readl(GPIO1_DR);     //寄存器为32位寄存器，所以使用readl,)
        val &= (1 << 3);
        writel(val, GPIO1_GDIR_BASE);
    }
    else{
        printk(KERN_INFO "Error input\n%s\n", __func__);
    }
    return;
}

//实现file_operations对象中的函数
//open
static int newchrled_open( struct inode *inode, struct file *filep)
{
    //私有数据
    //将 newchrled 进行私有化处理
    filep->private_data = &newchrled;

    return 0;
}

//close
static int newchrled_release(struct inode *inode, struct file *filep)
{
    //私有化处理数据
    struct newchrled_dev *dev = (struct newchrled_dev*)filep->private_data;
    //就可以使用这种方式使用私有数据，
    // dev->class;
    // dev->major;
    // dev->minor;

    return 0;
} 

//write
//应用程序写到驱动 -> HAL
static ssize_t newchrled_write(struct file *filp, const char __user *buf,
			 size_t count, loff_t *ppos)
{
    int retval;
    unsigned char databuf[1];   //一个字符的数据缓冲区

    //从用户数据空间拷贝数据
    retval = copy_from_user(databuf, buf, count);
    if(retval < 0){
        printk(KERN_INFO, "kernel write failed.\n");
        return -EFAULT;
    }

    //根据数据来 修改相应的寄存器的值
    led_switch(databuf[0]);

    return 0;
}

//read
static ssize_t newchrled_read(struct file *filp, __user char *buf, size_t count,
			loff_t *ppos)
{
    return 0;
}

//初始化一个file_operations
static const struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    //open, close, read, write,...
    .open = newchrled_open,
    .release = newchrled_release,
    .read = newchrled_read,
    .write = newchrled_write,
};

//模块入口
//为了增强代码的鲁棒性，使用goto语句处理异常的返回值，而不是直接返回
//直接返回有很多安全隐患
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
    newchrled.c_dev.owner = THIS_MODULE;
    //cdev的初始化
    //void cdev_init(struct cdev *, const struct file_operations *);
    cdev_init(&newchrled.c_dev, &newchrled_fops);
    //将cdev添加到linux驱动中
    //int cdev_add(struct cdev *, dev_t, unsigned);
    ret = cdev_add(&newchrled.c_dev, newchrled.devid, NEWCHRLED_COUNT);
    if(ret < 0){
        goto fail_cdev;     //
    }
    //step4: 自动创建设备节点
    //cmm_class = class_create(THIS_MODULE, "cardman_4000");
    newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if (IS_ERR(newchrled.class)){
        ret = PTR_ERR(newchrled.class);
        goto fail_class;    //
    }

    newchrled.device = device_create(newchrled.class, NULL,
                    newchrled.devid, NULL, NEWCHRLED_NAME);
    if( IS_ERR(newchrled.device) ){
        ret = PTR_ERR(newchrled.device);
        goto fail_device;       //
    }
    
    return 0;


/***这里是异常返回值处理部分**/
fail_device:
    class_destroy(newchrled.class);
    //goto执行到这里以后，仍然会执行后面的fail_class, fail_cdev, fail_devid
fail_class:
    dev_del(&newchrled.c_dev);
    //执行fail_cdev, fail_devid
fail_cdev:
    unregister_chrdev_region(newchrled.devid, 1);
    //仍然会执行fail_devid
fail_devid:
    return ret;
}

//模块出口
static void __exit newchrled_exit(void)
{
    printk(KERN_INFO "exit new char device\n");
    //删除字符设备
    //void cdev_del(struct cdev *);
    cdev_del(&newchrled.c_dev);

    //卸载
    //void unregister_chrdev_region(dev_t from, unsigned count);
    unregister_chrdev_region(newchrled.devid, 1);

    //摧毁设备
    //void device_destroy(struct class *cls, dev_t devt);
    device_destroy(newchrled.class, newchrled.devid);

    //摧毁创建的类
    //class_destroy(cmm_class);
    class_destroy(newchrled.class);

    return;
}

module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_AUTHOR("QiZhen");
MODULE_LICENSE("Dual BSD/GPL");