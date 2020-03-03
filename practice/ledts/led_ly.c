
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


#define DEVICE_NAME     "leds"  /* 加载模式后，执行”cat /proc/devices”命令看到的设备名称 */
#define LED_MAJOR       231     /* 主设备号 */

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
};//配置信息

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
	unsigned char value = 0,led_num;  //led_num 要控制的灯  value  0  /  1  0 亮  1 灭
	if ( size != 2)//
			return -EINVAL;
 
	ret = copy_from_user(led_vals, buf, size);
	if(ret << 0){
		return -EINVAL;
	}

	
	value = led_vals[1] ? 0: 1;
	led_num = led_vals[0];    //第一个是要控制的灯  第二个是命令

	
	if(led_num == 3){//同时控制所有led
		s3c2410_gpio_setpin(led_table[0],value);
		s3c2410_gpio_setpin(led_table[1],value);
		s3c2410_gpio_setpin(led_table[2],value);
	}else{//单独控制单个led
		s3c2410_gpio_setpin(led_table[0+led_num],value);
	}
	return 0;
}


static struct file_operations s3c24ly_leds_fops = {
    .owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open   =   s3c24ly_leds_open,     
	.write	=	s3c24ly_leds_write,
};



static int s3c24ly_leds_init(void)
//static int __init init_module(void)

{
    int ret;
	int minor = 0;

    /* 注册字符设备
     * 参数为主设备号、设备名字、file_operations结构；
     * 这样，主设备号就和具体的file_operations结构联系起来了，
     * 操作主设备为LED_MAJOR的设备文件时，就会调用s3c24xx_leds_fops中的相关成员函数
     * LED_MAJOR可以设为0，表示由内核自动分配主设备号
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
 * 执行rmmod命令时就会调用这个函数 
 */
static void s3c24ly_leds_exit(void)
{
	int minor;
    /* 卸载驱动程序 */
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




