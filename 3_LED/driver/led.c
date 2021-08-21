#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define LED_MAJOR       200
#define LED_NAME        "led"

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

//开灯 / 关灯
#define LEDON   1
#define LEDOFF  0

//打开 / 关闭LED
void led_switch( u8 sta )
{
    u32 val = 0;
    if( sta == LEDON ){
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_GDIR_BASE);
    }
    else if(sta == LEDOFF){
        val = readl(GPIO1_DR);
        val &= (1 << 3);
        writel(val, GPIO1_GDIR_BASE);
    }
    else{
        printk(KERN_INFO "Error input\n%s\n", __func__);
    }
    return;
}

//打开
static int led_open(struct inode *inode, struct file *file)
{
    return 0;
}

//关闭
static int led_release(struct inode *inode, struct file *file)
{
    //关闭LED
    // int val = 0;    
    // val = readl(GPIO1_DR);
    // val &= (1 << 3);
    // writel(val, GPIO1_GDIR_BASE);

    return 0;
}

//写驱动设备
static ssize_t led_write(struct file *filp, const char __user *buf,
			 size_t count, loff_t *ppos)
{
    int retval;
    unsigned char databuf[1];   //数据缓存

    //从用户程序获得需要下发的数据
    //从用户空间拷贝数据(用户程序)
    retval = copy_from_user(databuf, buf, count);
    if(retval < 0){
        printk(KERN_INFO, "kernel write failed.\n");
        return -EFAULT;
    }

    //然后通过驱动下发到设备
    //判断开灯 / 关灯
    led_switch( databuf[0] );

    return 0;
}
//字符设备操作集合
static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,     
    //.read             //操作LED，所以不写读的操作方法
};

//驱动入口
static int __init led_init(void)
{
    int ret = 0;
    unsigned int val = 0;    //一个临时变量
    //step1: 初始化LED， 地址映射
    //4字节寄存器，32位的寄存器
    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
    GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

    //操作内存中的数据：读 -> 改 -> 写
    //step2: 初始化
    //之所以使用readl(), 因为这些寄存器都是32位的，4字节
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);      //先将这两位的清零
    val |= 3 << 26;         //再置1
    writel(val, IMX6U_CCM_CCGR1);

    writel(0x10B0, SW_MUX_GPIO1_IO03);

    val = readl(GPIO1_GDIR_BASE);
    val |= 1<<3;
    writel(val, GPIO1_GDIR_BASE);

    val = readl(GPIO1_DR);
    val &= ~(1 << 3);
    writel(val, GPIO1_GDIR_BASE);

    //注册字符设备
    ret = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
    if(ret < 0){
        printk(KERN_INFO "register chardeveice failed.\n");
        
        //return -1;
        return -EIO;
    }
    printk("load driver.\n%s was called.\n", __func__);

    return 0;
}

//卸载驱动
static void __exit led_exit(void)
{
    //如果不加这段代码，卸载驱动时,LED仍然是点亮状态
    //关闭LED
    unsigned int val = 0;    
    val = readl(GPIO1_DR);
    val &= (1 << 3);
    writel(val, GPIO1_GDIR_BASE);

    //驱动出口取消地址映射
    //申请 和 释放一定要一起写， 否则会忘掉
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    //注销字符驱动
    unregister_chrdev(LED_MAJOR, LED_NAME);

    printk("unload driver.\n%s was called.\n", __func__);
    return;
}

//注册驱动
module_init(led_init);
module_exit(led_exit);

//驱动证书
MODULE_LICENSE("GPL");
MODULE_AUTHOR("qz");