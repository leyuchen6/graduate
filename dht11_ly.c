
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

#define DHT11_CNT   2


static struct cdev dht11_ly_cdev;
static struct class *dht11_ly_class;
static int major;
static const char* dht11_name = "dht11_dev";


#define delay_us(x)		udelay(x)
#define delay_ms(x)		mdelay(x)



int dht11_ly_open(struct inode *inode, struct file *flips)
{
	printk("--------------%s--------------\n",__FUNCTION__);
	return 0;
}
void data_out(int data)
{
    s3c2410_gpio_cfgpin(S3C2410_GPE15, S3C2410_GPE15_OUTP);
    s3c2410_gpio_setpin(S3C2410_GPE15, data);
}

int data_in(void)
{
	s3c2410_gpio_cfgpin(S3C2410_GPE15 , S3C2410_GPE15_INP);
	return  s3c2410_gpio_getpin(S3C2410_GPE15);
}

unsigned char DHT11_Read_Bit(void) 			 
{
 	unsigned char retry=0;
	while((data_in()) && (retry<100))//�ȴ���Ϊ�͵�ƽ
	{
		retry++;
		udelay(1);
	}
	retry=0;
	while((!data_in()) && (retry<100))//�ȴ���Ϊ�ߵ�ƽ
	{
		retry++;
		udelay(1);
	}
	
	udelay(40);//  ��ʱ40 us
	if(data_in())
	    return 1;
	else 
	    return 0;		   
}

//��DHT11��ȡһ���ֽ�
//����ֵ������������
static u8 DHT11_Read_Byte(void)    
{        
    u8 i,dat;
    dat=0;
	for (i=0;i<8;i++) 
	{
		dat<<=1; 
		dat|=DHT11_Read_Bit();
	}    
	return dat;

}



int ly_read_data(unsigned char *envir)
{
	int i;
	char buf[5];
	u8 retry=0;

	/* ����ռס���� ����ʼ�ź� */
	
	data_out(0);
	mdelay(20);
	
	data_out(1);
	udelay(40);

	/* �ȴ�dht��������Ӧ */
	
/*	if(data_in() == 0) 
	{
		while(data_in() == 0 &&  (retry < 100))   //����  80 us
		{
			retry++;
			udelay(1);
		}; 
		
		if(retry>=100)
		{ 
			return 1;
		}
		else 
			retry=0;
		
		while((!data_in()) && (retry<100))    //����  80  us
			
		{
			retry++;
			udelay(1);
		};
		
		if(retry>=100)
		{ 
			return 1;
		}else 	    
			retry=0;
*/		
		delay_us(160);
		for(i=0;i<5;i++)  //��������
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
		{
			*envir=buf[0];
			*(envir+1)=buf[2];
		}
		return 0;	
//	}
//	return 1;
}
static ssize_t dht11_ly_read(struct file *file, char __user *buf,
		    size_t nbytes, loff_t *ppos)
{	
	unsigned char buf_read[5]={0};	
	int ret = 0;
	int ret1 = 0;
 	ret = ly_read_data(buf_read);
	if(ret == 0 ){
		printk("1\n");
		ret1 = copy_to_user(buf, buf_read, nbytes);
		if(ret1 < 0 )
			return -1;
	}else{
		printk("read error\n");
	}
	
	return 0;
}

static int dht11_ly_release(struct inode *inode, struct file *flip)
{
	printk("--------------%s--------------\n",__FUNCTION__);
	return 0;
}





static struct file_operations dht11_ly_fops = {
    .owner   =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open    =   dht11_ly_open,     
	.read    =   dht11_ly_read,
	.release =   dht11_ly_release,
};


static int dht11_ly_init(void)
{
	dev_t devid;
	int ret;
	int i = 0;

	/* ע������ */
	alloc_chrdev_region(&devid, 0, DHT11_CNT, dht11_name); /* (major,0~1) ��Ӧ dht11_ly_fops, (major, 2~255)������Ӧdht11_ly_fops */
	major = MAJOR(devid);                     

	cdev_init(&dht11_ly_cdev, &dht11_ly_fops);
	cdev_add(&dht11_ly_cdev, devid, DHT11_CNT);

	dht11_ly_class = class_create(THIS_MODULE, dht11_name);
	class_device_create(dht11_ly_class, NULL, MKDEV(major, 0), NULL, "dht11_dev0"); /* /dev/dht11_dev0  */

	
	if( IS_ERR(dht11_ly_class))
	{
		printk(KERN_ERR "fail create class\n");
		ret = PTR_ERR(dht11_ly_class);
		
		goto err_1;
	}



//	i = DHT11_ly_Init();

//	if(! i ) // DHT11_ly_Init（）这个函数成功返回 0  失败返回 1
		printk("dht11 Initing...\n");

	
	return 0;
 
err_1:
		class_destroy(dht11_ly_class);
		return -1;
	
}

static void dht11_ly_exit(void)
{
	class_device_destroy(dht11_ly_class, MKDEV(major, 0));
	class_device_destroy(dht11_ly_class, MKDEV(major, 1));
	class_destroy(dht11_ly_class);

	cdev_del(&dht11_ly_cdev);
	unregister_chrdev_region(MKDEV(major, 0), DHT11_CNT);


}

module_init(dht11_ly_init);
module_exit(dht11_ly_exit);

MODULE_LICENSE("GPL");

