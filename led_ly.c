
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>


#define DEVICE_NAME     "leds"  /* ����ģʽ��ִ�С�cat /proc/devices����������豸���� */
#define LED_MAJOR       231     /* ���豸�� */

static struct class *leds_class;
static struct class_device	*leds_class_devs[4];

#define ON 1
#define OFF 0

static unsigned long led_table[]={
	S3C2410_GPF4,
	S3C2410_GPF5,
	S3C2410_GPF6,
};//gpfdat   

static unsigned int led_cfg_table[]={
	S3C2410_GPF4_OUTP,
	S3C2410_GPF5_OUTP,
	S3C2410_GPF6_OUTP,
};//������Ϣ

static int s3c24ly_leds_open(struct inode *inode, struct file *file)
{
	int i;
	for(i=0;i<4;i++){
		s3c2410_gpio_cfgpin(led_table[i],led_cfg_table[i]);
		return 0;
	}
}



ssize_t s3c24ly_leds_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned char led_vals[2];
	int ret;
	unsigned char value = 0,led_num;  //led_num Ҫ���Ƶĵ�  value  0  /  1  0 ��  1 ��
	if ( size != 2)//
			return -EINVAL;
 
	ret = copy_from_user(led_vals, buf, size);
	if(ret << 0){
		return -EINVAL;
	}

	
	value = led_vals[1] ? 0: 1;
	led_num = led_vals[0];    //��һ����Ҫ���Ƶĵ�  �ڶ���������

	
	if(led_num == 3){//ͬʱ��������led
		s3c2410_gpio_setpin(led_table[0],value);
		s3c2410_gpio_setpin(led_table[1],value);
		s3c2410_gpio_setpin(led_table[2],value);
	}else{//�������Ƶ���led
		s3c2410_gpio_setpin(led_table[0+led_num],value);
	}
	return 0;
}


static struct file_operations s3c24ly_leds_fops = {
    .owner  =   THIS_MODULE,    /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
    .open   =   s3c24ly_leds_open,     
	.write	=	s3c24ly_leds_write,
};



static int s3c24ly_leds_init(void)
//static int __init init_module(void)

{
    int ret;
	int minor = 0;

    /* ע���ַ��豸
     * ����Ϊ���豸�š��豸���֡�file_operations�ṹ��
     * ���������豸�žͺ;����file_operations�ṹ��ϵ�����ˣ�
     * �������豸ΪLED_MAJOR���豸�ļ�ʱ���ͻ����s3c24xx_leds_fops�е���س�Ա����
     * LED_MAJOR������Ϊ0����ʾ���ں��Զ��������豸��
     */
    ret = register_chrdev(LED_MAJOR, DEVICE_NAME, &s3c24ly_leds_fops);
    if (ret < 0) {
      printk(DEVICE_NAME " can't register major number\n");
      return ret;
    }

	leds_class = class_create(THIS_MODULE, "leds");
	if (IS_ERR(leds_class))
		return PTR_ERR(leds_class);
    


	leds_class_devs[0] = class_device_create(leds_class, NULL, MKDEV(LED_MAJOR, 0), NULL, "leds"); /* /dev/leds */
	
	for (minor = 1; minor < 4; minor++)  /* /dev/led1,2,3 */
	{
		leds_class_devs[minor] = class_device_create(leds_class, NULL, MKDEV(LED_MAJOR, minor), NULL, "led%d", minor);
		if (unlikely(IS_ERR(leds_class_devs[minor])))
			return PTR_ERR(leds_class_devs[minor]);
	}
        
    printk(DEVICE_NAME " initialized\n");
    return 0;
}

/*
 * ִ��rmmod����ʱ�ͻ����������� 
 */
static void s3c24ly_leds_exit(void)
{
	int minor;
    /* ж���������� */
    unregister_chrdev(LED_MAJOR, DEVICE_NAME);

	for (minor = 0; minor < 4; minor++)
	{
		class_device_unregister(leds_class_devs[minor]);
	}
	class_destroy(leds_class);
}

module_init(s3c24ly_leds_init);
module_exit(s3c24ly_leds_exit);

MODULE_LICENSE("GPL");




