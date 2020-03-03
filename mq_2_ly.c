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

static void __iomem *adc_base;//���ڱ���ӳ���������ַ
	
#define ADCCON (adc_base + S3C2410_ADCCON) //ADC control
	
#define ADCTSC (adc_base + S3C2410_ADCTSC) //ADC touch screen control
	
#define ADCDLY (adc_base + S3C2410_ADCDLY) //ADC start or Interval Delay
	
#define ADCDAT0 (adc_base + S3C2410_ADCDAT0) //ADC conversion data 0
	
#define ADCDAT1 (adc_base + S3C2410_ADCDAT1) //ADC conversion data 1
	
#define ADCUPDN (adc_base + 0x14) //Stylus Up/Down interrupt status

#define DEVICE_NAME "mq_2_dev"


/* adcʱ�� */
static struct clk *adc_clk;



typedef struct {
	wait_queue_head_t adc_waitq; //adc��������
	int channel;
	int prescale;
} ADC_DEV;

static ADC_DEV adc_dev;

static volatile int ev_adc = 0; //adc״̬��ǣ�1��ʾ���ݿɶ�

static int adc_data;


//DECLARE_MUTEX(ADC_LOCK);


/* ��������Ƶ��Ϊch��Ԥ��Ƶϵ��Ϊprescale����ʹ��ADCת�� */
static void start_adc(int ch, int prescale)
{ 
	unsigned int tmp;

	tmp = S3C2410_ADCCON_PRSCEN | S3C2410_ADCCON_PRSCVL(prescale) | S3C2410_ADCCON_SELMUX(ch);
	writel(tmp, ADCCON);

	tmp = readl(ADCCON);
	tmp |= S3C2410_ADCCON_ENABLE_START;
	writel(tmp, ADCCON);
}

/* adc�жϴ����� */
static irqreturn_t adc_irq(int irq, void *dev_id)
{

	if (!ev_adc) {
		/* ��ȡadcת��������ֲ��Ͽ��Եó�ת�����λ��9��0λ */
		adc_data = readl(adc_base + S3C2410_ADCDAT0) & 0x3ff;

		ev_adc = 1; //������ݿɶ�
		wake_up_interruptible(&adc_dev.adc_waitq);
	}

	return IRQ_HANDLED;
}

/*****************************************************************************  
/ ������ͣ�����ADC���� DMA 
/ ����ѡ��: J3�ϵ�AIN0
/ ADC ͨ��ѡ��AIN0
/ ADCCON   0x5800000
/	ECFLG 			[15] 	ת����־����	 	0 ��ת�� 		1 ����ת��
/	PRSCEN 			[14] 	A/Dת��Ԥ��Ƶ�� 	0 ��ֹ 			1 ʹ��
/	PRSCVL 			[13:6] 	A/Dת��Ԥ��Ƶ��ֵ	Data value: 0 ~ 255 			ע��ֵ��Ҫ����5
/	SEL_MUX 		[5:3]	ͨ��ѡ��			000 = AIN 0
/	STDBM 			[2]		ģʽѡ��			0 ��������ģʽ 	��д  ADCDAT0
/	READ_ START 	[1]		A/Dת�����Ա���		0 ��ֹ 1 ʹ��
/	ENABLE_START	[0]		ʹ�ܿ�ʼת������	0 ����Ҫ����	1 A/Dת�������������λ
/ 
/
/
/
/
/	ADCDLY	 	0x5800008	������ʱ ����Ϊ0
/	ADCDAT0 	0x580000C	������
******************************************************************************/


static int mq_2_ly_open(struct inode *inode, struct file *flips)
{
	printk("--------------%s--------------\n",__FUNCTION__);
		 int ret;
	/* ��ʼ��adc�ȴ����� */
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
	//����AIN0ͨ����ֵ  
//	/* ��ȡ�ź��� */
//	if (down_trylock(&ADC_LOCK)) {
//	return -EBUSY;
//	}
//	ev_adc = 1;
	if (!ev_adc) {
		/* �����ݲ��ɶ�ʱ����������ʽ��ȡֱ�ӷ��� */
		adc_irq(IRQ_ADC,&adc_dev);
		return -EAGAIN;
		}
	
	else {
		/* ����adc */
		start_adc(adc_dev.channel, adc_dev.prescale);
		/* �������̣��ȴ�adcת����� */
	
		wait_event_interruptible(adc_dev.adc_waitq, ev_adc);
	
		}
	/* ���̻��Ѻ󣬱��adc���ɶ� */
	ev_adc = 0;

	copy_to_user(buf, (char *) &adc_data, sizeof(adc_data));


//	/* �ͷ��ź��� */
//	up(&ADC_LOCK);

	return sizeof(adc_data);

}
	


static int __init adc_init(void)
{ 
	int ret;

	/* ����adcƵ����Ԥ��Ƶϵ�� */
	adc_dev.channel = 0;
	adc_dev.prescale = 0xff;

	/* ��ȡADCʱ�ӣ�����ʱ�ӿ����Ŀǰ�������˽⣬
	* �����쿪ʼ���� */
	adc_clk = clk_get(NULL, "mq_2_dev");
	if (!adc_clk) {
		printk(KERN_ERR "failed to find adc clock sourcen");
		return -ENOENT;
	}

	/* ʹ��ʱ�� */
	clk_enable(adc_clk);

	/* ӳ��IO�ڴ� */
	adc_base = ioremap(S3C2410_PA_ADC, S3C24XX_SZ_ADC);//һ��20���˿ڣ���Ϊ0x14
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
    .owner   =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open    =   mq_2_ly_open,     
	.read    =   mq_2_ly_read,
	.release =   mq_2_ly_release,
};


static int mq_2_ly_init(void)
{
	dev_t devid;
	int ret;
//	int i = 0;


	
	/* ע������ */
	alloc_chrdev_region(&devid, 0, MQ_2_CNT, mq_2_name); /* (major,0~1) ��Ӧ mq_2_ly_fops, (major, 2~255)������Ӧmq_2_ly_fops */
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

//	if(! i ) // mq_2_ly_Init（）这个函数成功返回 0  失败返回 1
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



