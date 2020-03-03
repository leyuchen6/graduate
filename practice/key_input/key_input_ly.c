/*
基于S3C2440按键输入子系统驱动代码
参考linux-2.6.22.6\drivers\input\keyboard\gpio_keys.c 
*/


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





static struct input_dev *buttons_dev;
static struct timer_list button_timer;

//定义按键信息结构体
struct key_info{              
    unsigned int eint;
    unsigned int pins;
    unsigned int pin_val;
    char name[20];
};

struct key_info *key_info_id=NULL;


//按键数组初始化
struct key_info mykey[4]=
{
    {IRQ_EINT0, S3C2410_GPF0, KEY_L,        "S2"},
    {IRQ_EINT2, S3C2410_GPF2, KEY_S,        "S3"},
    {IRQ_EINT11,S3C2410_GPG3, KEY_ENTER,    "S4"},
    {IRQ_EINT19,S3C2410_GPG11,KEY_LEFTSHIFT,"S5"},
};

//按键中断处理函数
static irqreturn_t button_irq(int irq,void *dev_id)
{
    key_info_id=(struct key_info *)dev_id  ;           //等待队列控制条件置真
    mod_timer(&button_timer,jiffies+HZ/100);				//更新定时器值10ms
    return IRQ_HANDLED;
}

//定时器超时处理函数
static void button_timer_func(unsigned long data)
{
    struct key_info *cdev_keyinfo = key_info_id;
    unsigned int val ;                               //存储按键电平

	
    if(!key_info_id)                      //过滤第一个定时器中断
        return;
    val=s3c2410_gpio_getpin(cdev_keyinfo->pins);   //读管脚电平 /* val 打印一直为0 */
    if(val)
    {
        /* 按键没有按下，用0表示 */
        input_event(buttons_dev,EV_KEY,cdev_keyinfo->pin_val,0);
        //事件上报完毕后，发送事件同步事件
        input_sync(buttons_dev);
    }else
    {
        /* 按键按下，用1表示 */
        input_event(buttons_dev,EV_KEY,cdev_keyinfo->pin_val,1);
        //事件上报完毕后，发送事件同步事件
        input_sync(buttons_dev);
    }

}




static  int __init button_init(void)
{
    unsigned char count=0;
	int err;
    /* 1、分配一个input_dev结构体 */
    buttons_dev=input_allocate_device();
	buttons_dev->name = "button_dev";

    /* 2、设置结构体 */
    /* 2.1、能产生按键类事件 */
    set_bit(EV_KEY,buttons_dev->evbit);
//	set_bit(EV_SYN,buttons_dev->evbit);

    /* 2.2、设置能产生按键类中哪些事件  ：字母、数字、符号等等*/
    /* 开发板有四个按键这里规定为 L、S、ENTER、LEFTSHIFT左边的shift */
    set_bit(KEY_L,buttons_dev->keybit);
    set_bit(KEY_S,buttons_dev->keybit);
    set_bit(KEY_ENTER,buttons_dev->keybit);
    set_bit(KEY_LEFTSHIFT,buttons_dev->keybit);

	
    /* 3、注册*/
    input_register_device(buttons_dev);



    /* 4、硬件相关操作 */
    /* 4.1、定时器初始化*/
    init_timer(&button_timer);
    button_timer.expires=0;
    button_timer.function=button_timer_func;
    add_timer(&button_timer);

    /* 4.2、注册中断 */
    for(count=0;count<4;count++)
    {
		err = request_irq(mykey[count].eint,button_irq,IRQ_TYPE_EDGE_BOTH,mykey[count].name,&mykey[count]);
		if(err)
			break;
    }

	/* 5、上报事件 */

	
	
    return 0;
}
static  void __exit button_exit(void)
{
    unsigned char count=0;
    /* 删除定时器 */
    del_timer(&button_timer);

    /* 注销设备 */
    input_unregister_device(buttons_dev);

    /* 释放中断 */
    for(count=0;count<4;count++)
    {
        free_irq(mykey[count].eint,&mykey[count]);
    }   
    /* 释放申请的空间 */
    input_free_device(buttons_dev);
}

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");






