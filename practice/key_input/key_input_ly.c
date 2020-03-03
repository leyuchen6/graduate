/*
����S3C2440����������ϵͳ��������
�ο�linux-2.6.22.6\drivers\input\keyboard\gpio_keys.c 
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

//���尴����Ϣ�ṹ��
struct key_info{              
    unsigned int eint;
    unsigned int pins;
    unsigned int pin_val;
    char name[20];
};

struct key_info *key_info_id=NULL;


//���������ʼ��
struct key_info mykey[4]=
{
    {IRQ_EINT0, S3C2410_GPF0, KEY_L,        "S2"},
    {IRQ_EINT2, S3C2410_GPF2, KEY_S,        "S3"},
    {IRQ_EINT11,S3C2410_GPG3, KEY_ENTER,    "S4"},
    {IRQ_EINT19,S3C2410_GPG11,KEY_LEFTSHIFT,"S5"},
};

//�����жϴ�����
static irqreturn_t button_irq(int irq,void *dev_id)
{
    key_info_id=(struct key_info *)dev_id  ;           //�ȴ����п�����������
    mod_timer(&button_timer,jiffies+HZ/100);				//���¶�ʱ��ֵ10ms
    return IRQ_HANDLED;
}

//��ʱ����ʱ������
static void button_timer_func(unsigned long data)
{
    struct key_info *cdev_keyinfo = key_info_id;
    unsigned int val ;                               //�洢������ƽ

	
    if(!key_info_id)                      //���˵�һ����ʱ���ж�
        return;
    val=s3c2410_gpio_getpin(cdev_keyinfo->pins);   //���ܽŵ�ƽ /* val ��ӡһֱΪ0 */
    if(val)
    {
        /* ����û�а��£���0��ʾ */
        input_event(buttons_dev,EV_KEY,cdev_keyinfo->pin_val,0);
        //�¼��ϱ���Ϻ󣬷����¼�ͬ���¼�
        input_sync(buttons_dev);
    }else
    {
        /* �������£���1��ʾ */
        input_event(buttons_dev,EV_KEY,cdev_keyinfo->pin_val,1);
        //�¼��ϱ���Ϻ󣬷����¼�ͬ���¼�
        input_sync(buttons_dev);
    }

}




static  int __init button_init(void)
{
    unsigned char count=0;
	int err;
    /* 1������һ��input_dev�ṹ�� */
    buttons_dev=input_allocate_device();
	buttons_dev->name = "button_dev";

    /* 2�����ýṹ�� */
    /* 2.1���ܲ����������¼� */
    set_bit(EV_KEY,buttons_dev->evbit);
//	set_bit(EV_SYN,buttons_dev->evbit);

    /* 2.2�������ܲ�������������Щ�¼�  ����ĸ�����֡����ŵȵ�*/
    /* ���������ĸ���������涨Ϊ L��S��ENTER��LEFTSHIFT��ߵ�shift */
    set_bit(KEY_L,buttons_dev->keybit);
    set_bit(KEY_S,buttons_dev->keybit);
    set_bit(KEY_ENTER,buttons_dev->keybit);
    set_bit(KEY_LEFTSHIFT,buttons_dev->keybit);

	
    /* 3��ע��*/
    input_register_device(buttons_dev);



    /* 4��Ӳ����ز��� */
    /* 4.1����ʱ����ʼ��*/
    init_timer(&button_timer);
    button_timer.expires=0;
    button_timer.function=button_timer_func;
    add_timer(&button_timer);

    /* 4.2��ע���ж� */
    for(count=0;count<4;count++)
    {
		err = request_irq(mykey[count].eint,button_irq,IRQ_TYPE_EDGE_BOTH,mykey[count].name,&mykey[count]);
		if(err)
			break;
    }

	/* 5���ϱ��¼� */

	
	
    return 0;
}
static  void __exit button_exit(void)
{
    unsigned char count=0;
    /* ɾ����ʱ�� */
    del_timer(&button_timer);

    /* ע���豸 */
    input_unregister_device(buttons_dev);

    /* �ͷ��ж� */
    for(count=0;count<4;count++)
    {
        free_irq(mykey[count].eint,&mykey[count]);
    }   
    /* �ͷ�����Ŀռ� */
    input_free_device(buttons_dev);
}

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");






