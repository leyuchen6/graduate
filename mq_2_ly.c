#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/cdev.h>

#include <asm/gpio.h>



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

#include <asm/arch/regs-adc.h>
#include <asm/arch/map.h>
#include <linux/compiler.h>
#include <linux/interrupt.h>






#define MQ_2_CNT 3


static struct cdev mq_2_ly_cdev;
static struct class *mq_2_ly_class;
static int major;
static const char* mq_2_name = "mq_2_dev";

static void __iomem *adc_base;//用于保存映射后的虚拟地址
	
#define ADCCON (adc_base + S3C2410_ADCCON) //ADC control
	
#define ADCTSC (adc_base + S3C2410_ADCTSC) //ADC touch screen control
	
#define ADCDLY (adc_base + S3C2410_ADCDLY) //ADC start or Interval Delay
	
#define ADCDAT0 (adc_base + S3C2410_ADCDAT0) //ADC conversion data 0
	
#define ADCDAT1 (adc_base + S3C2410_ADCDAT1) //ADC conversion data 1
	
#define ADCUPDN (adc_base + 0x14) //Stylus Up/Down interrupt status

#define DEVICE_NAME "mq_2_dev"


/* adc时钟 */
static struct clk *adc_clk;



typedef struct {
	wait_queue_head_t adc_waitq; //adc工作队列
	int channel;
	int prescale;
} ADC_DEV;

static ADC_DEV adc_dev;

static volatile int ev_adc = 0; //adc状态标记，1表示数据可读

static int adc_data;


//DECLARE_MUTEX(ADC_LOCK);


/* 设置输入频道为ch，预分频系数为prescale，并使能ADC转换 */
static void start_adc(int ch, int prescale)
{ 
	unsigned int tmp;

	tmp = S3C2410_ADCCON_PRSCEN | S3C2410_ADCCON_PRSCVL(prescale) | S3C2410_ADCCON_SELMUX(ch);
	writel(tmp, ADCCON);

	tmp = readl(ADCCON);
	tmp |= S3C2410_ADCCON_ENABLE_START;
	writel(tmp, ADCCON);
}

/* adc中断处理函数 */
static irqreturn_t adc_irq(int irq, void *dev_id)
{

	if (!ev_adc) {
		/* 读取adc转换结果，手册上可以得出转换结果位于9：0位 */
		adc_data = readl(adc_base + S3C2410_ADCDAT0) & 0x3ff;

		ev_adc = 1; //标记数据可读
		wake_up_interruptible(&adc_dev.adc_waitq);
	}

	return IRQ_HANDLED;
}

/*****************************************************************************  
/ 代码解释：利用ADC采样 DMA 
/ 引脚选择: J3上的AIN0
/ ADC 通道选择：AIN0
/ ADCCON   0x5800000
/	ECFLG 			[15] 	转换标志结束	 	0 在转换 		1 结束转换
/	PRSCEN 			[14] 	A/D转换预分频器 	0 禁止 			1 使能
/	PRSCVL 			[13:6] 	A/D转换预分频器值	Data value: 0 ~ 255 			注：值需要大于5
/	SEL_MUX 		[5:3]	通道选择			000 = AIN 0
/	STDBM 			[2]		模式选择			0 正常操作模式 	读写  ADCDAT0
/	READ_ START 	[1]		A/D转换可以被读		0 禁止 1 使能
/	ENABLE_START	[0]		使能开始转换操作	0 不需要操作	1 A/D转换启动后清除此位
/ 
/
/
/
/
/	ADCDLY	 	0x5800008	设置延时 不能为0
/	ADCDAT0 	0x580000C	读数据
******************************************************************************/


static int mq_2_ly_open(struct inode *inode, struct file *flips)
{
	printk("--------------%s--------------\n",__FUNCTION__);
		 int ret;
	/* 初始化adc等待队列 */
	init_waitqueue_head(&(adc_dev.adc_waitq));

	ret = request_irq(IRQ_ADC,adc_irq, IRQF_SHARED, DEVICE_NAME, &adc_dev);
	if (ret) {
		printk(KERN_ERR "IRQ: %d error %d \n", IRQ_ADC, ret);
		return -EINVAL;

	}
	return 0;
}

static int mq_2_ly_release(struct inode *inode, struct file *flip)
{
	printk("--------------%s--------------\n",__FUNCTION__);
	free_irq(IRQ_ADC,&adc_dev);

	return 0;
}


static ssize_t mq_2_ly_read(struct file *file, char __user *buf,
		    size_t nbytes, loff_t *ppos)
{	
	//read copy_to_user   write copy_from_user
	//copy_to_user(void __user *to, const void *from, unsigned long n)
	//读出AIN0通道的值  
//	/* 获取信号量 */
//	if (down_trylock(&ADC_LOCK)) {
//	return -EBUSY;
//	}
//	ev_adc = 1;
	if (!ev_adc) {
		/* 当数据不可读时，非阻塞方式读取直接返回 */
		adc_irq(IRQ_ADC,&adc_dev);
		return -EAGAIN;
		}
	
	else {
		/* 启动adc */
		start_adc(adc_dev.channel, adc_dev.prescale);
		/* 阻塞进程，等待adc转换完成 */
	
		wait_event_interruptible(adc_dev.adc_waitq, ev_adc);
	
		}
	/* 进程唤醒后，标记adc不可读 */
	ev_adc = 0;

	copy_to_user(buf, (char *) &adc_data, sizeof(adc_data));


//	/* 释放信号量 */
//	up(&ADC_LOCK);

	return sizeof(adc_data);

}
	


static int __init adc_init(void)
{ 
	int ret;

	/* 设置adc频道和预分频系数 */
	adc_dev.channel = 0;
	adc_dev.prescale = 0xff;

	/* 获取ADC时钟，关于时钟框架我目前还不甚了解，
	* 过几天开始分析 */
	adc_clk = clk_get(NULL, "mq_2_dev");
	if (!adc_clk) {
		printk(KERN_ERR "failed to find adc clock sourcen");
		return -ENOENT;
	}

	/* 使能时钟 */
	clk_enable(adc_clk);

	/* 映射IO内存 */
	adc_base = ioremap(S3C2410_PA_ADC, S3C24XX_SZ_ADC);//一共20个端口，故为0x14
	if (adc_base == NULL) {
		printk(KERN_ERR "Failed to remap register blockn");
		ret = -EINVAL;
		goto err_noclk;
	}
	
err_noclk:
	clk_disable(adc_clk);
//	clk_put(adc_clk);


	return ret;
}


static struct file_operations mq_2_ly_fops = {
    .owner   =   THIS_MODULE,    /* 杩欐槸涓�涓畯锛屾帹鍚戠紪璇戞ā鍧楁椂鑷姩鍒涘缓鐨刜_this_module鍙橀噺 */
    .open    =   mq_2_ly_open,     
	.read    =   mq_2_ly_read,
	.release =   mq_2_ly_release,
};


static int mq_2_ly_init(void)
{
	dev_t devid;
	int ret;
//	int i = 0;


	
	/* 注册驱动 */
	alloc_chrdev_region(&devid, 0, MQ_2_CNT, mq_2_name); /* (major,0~1) 对应 mq_2_ly_fops, (major, 2~255)都不对应mq_2_ly_fops */
	major = MAJOR(devid);                     

	cdev_init(&mq_2_ly_cdev, &mq_2_ly_fops);
	cdev_add(&mq_2_ly_cdev, devid, MQ_2_CNT);

	mq_2_ly_class = class_create(THIS_MODULE, mq_2_name);
	class_device_create(mq_2_ly_class, NULL, MKDEV(major, 0), NULL, "mq_2_dev0"); /* /dev/mq_2_dev0  */
		
	adc_init();
		
	
	if( IS_ERR(mq_2_ly_class))
	{
		printk(KERN_ERR "fail create class\n");
		ret = PTR_ERR(mq_2_ly_class);
		
		goto err_1;
	}



//	i = mq_2_ly_Init();

//	if(! i ) // mq_2_ly_Init锛堬級杩欎釜鍑芥暟鎴愬姛杩斿洖 0  澶辫触杩斿洖 1
		printk("mq_2 Initing...\n");

	
	return 0;
 
err_1:
		class_destroy(mq_2_ly_class);
		iounmap(adc_base);
		return -1;
	
}

static void mq_2_ly_exit(void)
{
	class_device_destroy(mq_2_ly_class, MKDEV(major, 0));
	class_device_destroy(mq_2_ly_class, MKDEV(major, 1));
	class_destroy(mq_2_ly_class);

	cdev_del(&mq_2_ly_cdev);
	unregister_chrdev_region(MKDEV(major, 0), 	MQ_2_CNT);
	iounmap(adc_base);

}

module_init(mq_2_ly_init);
module_exit(mq_2_ly_exit);

MODULE_LICENSE("GPL");



