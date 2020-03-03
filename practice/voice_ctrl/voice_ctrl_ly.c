
/* 语音模块通过命令控制IO口电平变化
 * 通过中断来完成所需要控制的命令
 * J3 20 口  是一个 EINT2 
 * J1 3  口  是EINT20	GPG12
 * 按键  S2    	S3   	S4     	S5
 * 		 EINT0 	EINT2 	EINT11 	EINT19
 * 		 GPF0  	GPF2	GPG3	GPG11
 * GPG0 - 15    EINT8 - 23
 * GPF0 - 7 	EINT0 - 7 
 */
#include <linux/poll.h>
#include <linux/cdev.h>
	 
	 
	 
	 
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>
	 
#include <asm/arch/map.h>
#include <linux/compiler.h>
	 

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <linux/gpio_keys.h>
#include <asm/gpio.h>

/* 毕业设计实现的功能
 * 温湿度
 * 火灾报警一直开着
 * 
 */


/* 语音控制  	
 * 开关灯  		查看温湿度  
 * 播放音乐		播放动画	返回主界面
 */

#define IRQ_LY_CNT 7


static struct cdev irq_ly_cdev;
static struct class *irq_ly_class;
static int major;
static const char* irq_ly_name = "irq_ly_dev";



static struct timer_list irq_ly_timer;


//定义中断信息结构体
struct irq_ly_info{              
    unsigned int eint;
    unsigned int pins;
    unsigned int val;
    char name[20];
};
#define KD 	0
#define GD 	2
#define BFYY	11
#define CKWSD	19
#define FHZJM	20

//中断数组初始化
struct irq_ly_info myirq[5]=
{
    {IRQ_EINT0, 	S3C2410_GPF0, 	FHZJM,        	"FHZJM"},
    {IRQ_EINT2, 	S3C2410_GPF2, 	GD,        		"GD"},
    {IRQ_EINT11,	S3C2410_GPG3, 	BFYY,   	 	"BFYY"},
    {IRQ_EINT19,	S3C2410_GPG11,	CKWSD,			"CKWSD"},
	{IRQ_EINT20,	S3C2410_GPG12,	KD,				"KD"},
};


struct irq_ly_info *ieq_ly_info_id=NULL;


static int pins_val(int count)
{
	int val;
	val=s3c2410_gpio_getpin(myirq[count].pins);   //读管脚电平 
	return val;
}
static irqreturn_t voice_irq_ly(int irq,void *dev_id)
{
	/* 判断一下是哪个中断
	 * 再对应进行处理
	 */
    ieq_ly_info_id=(struct irq_ly_info *)dev_id  ;           //等待队列控制条件置真
    mod_timer(&irq_ly_timer,jiffies+HZ/100);				//更新定时器值10ms
	return IRQ_HANDLED;
}

//定时器超时处理函数
static void irq_ly_timer_func(unsigned long data)
{
	struct irq_ly_info *cdev_irqlyinfo = ieq_ly_info_id;
	s3c2410_gpio_setpin(S3C2410_GPG12,0);
    if(!ieq_ly_info_id)                      //过滤第一个定时器中断
        return;
	switch(cdev_irqlyinfo->val){
		case KD :
						s3c2410_gpio_cfgpin(S3C2410_GPF4,S3C2410_GPF4_OUTP);
						s3c2410_gpio_cfgpin(S3C2410_GPF5,S3C2410_GPF5_OUTP);
						s3c2410_gpio_cfgpin(S3C2410_GPF6,S3C2410_GPF6_OUTP);
						s3c2410_gpio_setpin(S3C2410_GPF4,0);
						s3c2410_gpio_setpin(S3C2410_GPF5,0);
						s3c2410_gpio_setpin(S3C2410_GPF6,0);
						break;
		case GD  	:	s3c2410_gpio_cfgpin(S3C2410_GPF4,S3C2410_GPF4_OUTP);
						s3c2410_gpio_cfgpin(S3C2410_GPF5,S3C2410_GPF5_OUTP);
						s3c2410_gpio_cfgpin(S3C2410_GPF6,S3C2410_GPF6_OUTP);
						s3c2410_gpio_setpin(S3C2410_GPF4,1);
						s3c2410_gpio_setpin(S3C2410_GPF5,1);
						s3c2410_gpio_setpin(S3C2410_GPF6,1);
						break;
		case BFYY  : break;
		case CKWSD : break;
		case FHZJM : break;

		
	}
}

static int irq_ly_open(struct inode *inode, struct file *file)
{
	printk("--------------%s--------------\n",__FUNCTION__);

	return 0;
}

ssize_t irq_ly_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	printk("irq_ly write...\n");
	return 0;
}

static int irq_ly_release(struct inode *inode, struct file *flip)
{
	printk("--------------%s--------------\n",__FUNCTION__);

	return 0;
}


static struct file_operations irq_ly_fops = {
    .owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open   =   irq_ly_open,     
	.write	=	irq_ly_write,
	.release = 	irq_ly_release
};



static  int __init voice_irq_ly_init(void)
{


	dev_t devid;
	int ret;
	unsigned char count=0;
	int err;
	/* 注册驱动 */
	alloc_chrdev_region(&devid, 0, IRQ_LY_CNT, irq_ly_name); /* (major,0~1) 对应 mq_2_ly_fops, (major, 2~255)都不对应mq_2_ly_fops */
	major = MAJOR(devid);					  

	cdev_init(&irq_ly_cdev, &irq_ly_fops);
	cdev_add(&irq_ly_cdev, devid, IRQ_LY_CNT);

	irq_ly_class = class_create(THIS_MODULE, irq_ly_name);
	class_device_create(irq_ly_class, NULL, MKDEV(major, 0), NULL, "irq_ly_dev0"); /* /dev/irq_ly_dev0  */

	if( IS_ERR(irq_ly_class))
	{
		printk(KERN_ERR "fail create class\n");
		ret = PTR_ERR(irq_ly_class);
		
		goto err_1;
	}
	
    /* 4.1、定时器初始化*/
    init_timer(&irq_ly_timer);
	
	
    irq_ly_timer.expires=0;
    irq_ly_timer.function=irq_ly_timer_func;
    add_timer(&irq_ly_timer);

	
    /* 4.2、注册中断 */
    for(count=0;count<5;count++)
    {
	   /* err = request_irq(mykey[count].eint,button_irq,IRQ_TYPE_EDGE_BOTH,mykey[count].name,&mykey[count])
	    * int request_irq(unsigned int irq,		irq_handler_t handler,	unsigned long flags,	const char *devname,	void *dev_id)  
	 	*/
	    err = request_irq(myirq[count].eint,voice_irq_ly,IRQ_TYPE_EDGE_RISING,myirq[count].name,&myirq[count]);
		if(err){
			break;
		}
    }
	
	
	s3c2410_gpio_setpin(S3C2410_GPG12,0);
	
	printk("irq_ly Initing...\n");
	
		
	return 0;
 
err_1:
		class_destroy(irq_ly_class);
		return -1;

    return 0;
}


  
static  void __exit voice_irq_ly_exit(void)
{
	unsigned char count=0;

	class_device_destroy(irq_ly_class, MKDEV(major, 0));
	class_device_destroy(irq_ly_class, MKDEV(major, 1));
	class_destroy(irq_ly_class);

	cdev_del(&irq_ly_cdev);
	unregister_chrdev_region(MKDEV(major, 0), 	IRQ_LY_CNT);

    /* 删除定时器 */
    del_timer(&irq_ly_timer);


	/* 释放中断 */
	for(count=0;count<5;count++)
	{
		free_irq(myirq[count].eint,&myirq[count]);
	}	

}


module_init(voice_irq_ly_init);
module_exit(voice_irq_ly_exit);
MODULE_LICENSE("GPL");



