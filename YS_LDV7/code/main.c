/***************************�����Ƶ���****************************
**  �������ƣ�YS-V0.7����ʶ��ģ����������
**	CPU: STC11L08XE
**	����22.1184MHZ
**	�����ʣ�9600 bit/S
**	���ײ�Ʒ��Ϣ��YS-V0.7����ʶ�𿪷���
**                http://yuesheng001.taobao.com
**  ���ߣ�zdings
**  ��ϵ��751956552@qq.com
**  �޸����ڣ�2013.9.13
**  ˵��������ģʽ+IO���ƣ� ��ÿ��ʶ��ʱ����Ҫ˵��С�ܡ�������� �����ܹ�������һ����ʶ��
/***************************�����Ƶ���******************************/
#include "config.h"
/************************************************************************************/
//	nAsrStatus ������main�������б�ʾ�������е�״̬������LD3320оƬ�ڲ���״̬�Ĵ���
//	LD_ASR_NONE:		��ʾû������ASRʶ��
//	LD_ASR_RUNING��		��ʾLD3320������ASRʶ����
//	LD_ASR_FOUNDOK:		��ʾһ��ʶ�����̽�������һ��ʶ����
//	LD_ASR_FOUNDZERO:	��ʾһ��ʶ�����̽�����û��ʶ����
//	LD_ASR_ERROR:		��ʾһ��ʶ��������LD3320оƬ�ڲ����ֲ���ȷ��״̬
/***********************************************************************************/
uint8 idata nAsrStatus=0;	
void MCU_init(); 
void ProcessInt0(); //ʶ��������
void  delay(unsigned long uldata);
void 	User_handle(uint8 dat);//�û�ִ�в�������
void Led_test(void);//��Ƭ������ָʾ
void Delay200ms();
uint8_t G0_flag=DISABLE;//���б�־��ENABLE:���С�DISABLE:��ֹ���� 
sbit LED=P4^2;//�ź�ָʾ��
//Ӧ��IO�ڶ��� ��ģ���ע P2��
sbit PA1=P1^0; //��Ӧ���ϱ�� P1.0
sbit PA2=P1^1;  //��Ӧ���ϱ�� P1.1
sbit PA3=P1^2;  //.....
sbit PA4=P1^3;  //.....
sbit PA5=P1^4;  //.....
sbit PA6=P1^5;  //.....
sbit PA7=P1^6;  //��Ӧ���ϱ�� P1.6
sbit PA8=P1^7;  //��Ӧ���ϱ�� P1.7



/***********************************************************
* ��    �ƣ� void  main(void)
* ��    �ܣ� ������	�������
* ��ڲ�����  
* ���ڲ�����
* ˵    ���� 					 
* ���÷����� 
**********************************************************/ 
void  main(void)
{
	uint8 idata nAsrRes;
	uint8 i=0;
	Led_test();
	MCU_init();
	LD_Reset();
	UartIni(); /*���ڳ�ʼ��*/
	nAsrStatus = LD_ASR_NONE;		//	��ʼ״̬��û������ASR
	
	#ifdef TEST	
	    PrintCom("һ�����С��\r\n"); /*text.....*/
		PrintCom("�������1������������\r\n"); /*text.....*/
		PrintCom("	2����ʪ�ȼ��\r\n"); /*text.....*/
		PrintCom("	3������\r\n"); /*text.....*/
		PrintCom("	4���ص�\r\n"); /*text.....*/
		PrintCom("  5����������\r\n"); /*text.....*/
		PrintCom("	6�����Ŷ���\r\n"); /*text.....*/
		PrintCom("	7����ʱ\r\n"); /*text.....*/
	#endif

	while(1)
	{
		switch(nAsrStatus)
		{
			case LD_ASR_RUNING:
			case LD_ASR_ERROR:		
				break;
			case LD_ASR_NONE:
			{
				nAsrStatus=LD_ASR_RUNING;
				if (RunASR()==0)	/*	����һ��ASRʶ�����̣�ASR��ʼ����ASR���ӹؼ��������ASR����*/
				{
					nAsrStatus = LD_ASR_ERROR;
				}
				break;
			}
			case LD_ASR_FOUNDOK: /*	һ��ASRʶ�����̽�����ȥȡASRʶ����*/
			{				
				nAsrRes = LD_GetResult();		/*��ȡ���*/
				User_handle(nAsrRes);//�û�ִ�к��� 
				nAsrStatus = LD_ASR_NONE;
				break;
			}
			case LD_ASR_FOUNDZERO:
			default:
			{
				nAsrStatus = LD_ASR_NONE;
				break;
			}
		}// switch	 			
	}// while

}
/***********************************************************
* ��    �ƣ� 	 LED�Ʋ���
* ��    �ܣ� ��Ƭ���Ƿ���ָʾ
* ��ڲ����� �� 
* ���ڲ�������
* ˵    ���� 					 
**********************************************************/
void Led_test(void)
{
	LED=~ LED;
	Delay200ms();
	LED=~ LED;
	Delay200ms();
	LED=~ LED;
	Delay200ms();
	LED=~ LED;
	Delay200ms();
	LED=~ LED;
	Delay200ms();
	LED=~ LED;
}
/***********************************************************
* ��    �ƣ� void MCU_init()
* ��    �ܣ� ��Ƭ����ʼ��
* ��ڲ�����  
* ���ڲ�����
* ˵    ���� 					 
* ���÷����� 
**********************************************************/ 
void MCU_init()
{
	P0 = 0xff;
	P1 = 0x00;
	P2 = 0xff;
	P3 = 0xff;
	P4 = 0xff;

	P1M0=0XFF;	//P1�˿�����Ϊ����������ܣ������IO�������������������̵���ģ�鹤��
	P1M1=0X00;

	LD_MODE = 0;		//	����MD�ܽ�Ϊ�ͣ�����ģʽ��д
	IE0=1;
	EX0=1;
	EA=1;
}
/***********************************************************
* ��    �ƣ�	��ʱ����
* ��    �ܣ�
* ��ڲ�����  
* ���ڲ�����
* ˵    ���� 					 
* ���÷����� 
**********************************************************/ 
void Delay200us()		//@22.1184MHz
{
	unsigned char i, j;
	_nop_();
	_nop_();
	i = 5;
	j = 73;
	do
	{
		while (--j);
	} while (--i);
}

void  delay(unsigned long uldata)
{
	unsigned int j  =  0;
	unsigned int g  =  0;
	while(uldata--)
	Delay200us();
}

void Delay200ms()		//@22.1184MHz
{
	unsigned char i, j, k;

	i = 17;
	j = 208;
	k = 27;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

/***********************************************************
* ��    �ƣ� �жϴ�������
* ��    �ܣ�
* ��ڲ�����  
* ���ڲ�����
* ˵    ���� 					 
* ���÷����� 
**********************************************************/ 
void ExtInt0Handler(void) interrupt 0  
{ 	
	ProcessInt0();				/*	LD3320 �ͳ��ж��źţ�����ASR�Ͳ���MP3���жϣ���Ҫ���жϴ��������зֱ���*/
}
/***********************************************************
* ��    �ƣ��û�ִ�к��� 
* ��    �ܣ�ʶ��ɹ���ִ�ж������ڴ˽����޸� 
* ��ڲ����� �� 
* ���ڲ�������
* ˵    ���� ͨ������PAx�˿ڵĸߵ͵�ƽ���Ӷ������ⲿ�̵�����ͨ��					 
**********************************************************/
void 	User_handle(uint8 dat)
{
     //UARTSendByte(dat);//����ʶ���루ʮ�����ƣ�
		 if(0==dat)
		 {
		  	G0_flag=ENABLE;
				LED=0;
		 }
		 else if(ENABLE==G0_flag)
		 {	
		 		G0_flag=DISABLE;
				LED=1;
			 switch(dat)		   /*�Խ��ִ����ز���,�ͻ��޸�*/
			  {
				  case CODE_FHZJM:			/*������ԡ�*/
						PrintCom("�����������桱����ʶ��ɹ�\r\n"); //���������ʾ��Ϣ����ɾ����
						PA1=1;//��PA1�˿�Ϊ�ߵ�ƽ
						Delay200ms();
						PA1 = 0	;
													 break;
					case CODE_WSDJC:	 /*���ȫ����*/
						PrintCom("����ʪ�ȼ�⡱����ʶ��ɹ�\r\n");//���������ʾ��Ϣ����ɾ����		//�鿴��ʪ��
						PA2=1;//��PA2�˿�Ϊ�ߵ�ƽ
						Delay200ms();
						PA2 = 0	 ;

													 break;
					case CODE_KD:		/*�����λ��*/				
						PrintCom("�����ơ�����ʶ��ɹ�\r\n"); //���������ʾ��Ϣ����ɾ����
						PA3=1;//��PA3�˿�Ϊ�ߵ�ƽ 
						Delay200ms();
						PA3 = 0	  ;

													break;
					case CODE_GD:		/*�����λ��*/				
						PrintCom("���صơ�����ʶ��ɹ�\r\n"); //���������ʾ��Ϣ����ɾ����
						PA4=1;//��PA4�˿�Ϊ�͵�ƽ
						Delay200ms();
						PA4 = 0	;

													break;
					case CODE_BFYY:		/*�����λ��*/				
						PrintCom("���������֡�����ʶ��ɹ�\r\n"); //���������ʾ��Ϣ����ɾ����
						PA5=1;//��PA5�˿�Ϊ�ߵ�ƽ
						Delay200ms();
						PA5 = 0	 ;

													break;
					case CODE_BFDH:		/*�����λ��*/				
						PrintCom("�����Ŷ���������ʶ��ɹ�\r\n"); //���������ʾ��Ϣ����ɾ����
						PA6=1;//��PA6�˿�Ϊ�ߵ�ƽ
						Delay200ms();
						PA6 = 0	  ;

													break;
					case CODE_BS:		/*�����λ��*/				
						PrintCom("����ʱ������ʶ��ɹ�\r\n"); //���������ʾ��Ϣ����ɾ����
						PA7=1;//��PA7�˿�Ϊ�ߵ�ƽ
						Delay200ms();
						PA7 = 0	   ;

													break;																											
							default:PrintCom("������ʶ�𷢿���\r\n"); //���������ʾ��Ϣ����ɾ����
													break;
				}	
			}	
			else 	
			{
				PrintCom("��˵��һ������\r\n"); //���������ʾ��Ϣ����ɾ����	
			}
}	 