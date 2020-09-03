#include"ad7705.h"
#include "sys/unistd.h"
#include "driver/gpio.h"
void Delay_Us(unsigned int us)
{
	for(;us>0;us--);
}
void Delay_Ms(unsigned int ms)
{
	for(;ms>0;ms--)
		Delay_Us(300);
}

void SPISendByte(unsigned char dat)
{
 unsigned char a;
 SCLK1 ;
 for (a=0; a<8; a++)
 {
  SCLK0;
  if(dat & 0x80)
  DIN1;
  else
  DIN0;
usleep(10);
usleep(10);
  SCLK1;    
usleep(10);
usleep(10);
  dat<<=1;
 }
 DIN1;
}

unsigned int SPIRecv16bit(void)	 //SPI����
{
	unsigned int dat = 0;
	unsigned char i;
//	CS = 0;
	
	for(i=0;i<16;i++)  //8λ���˾�����
	{
		dat = dat<<1;//���յ��ķ������λ�����Ƶ���λ���ճ���һλ

		SCLK1;
		SCLK1;
		SCLK0;		//�͵�ƽ��ʱ���ȡ����
		SCLK0;
 
		if(DOUT)
			dat = dat | 0x0001;//SO����1��д1��dat���λ
		else
			dat = dat & 0xFFFE;//SO����0��д0��dat���λ������λҪ�Ƴ��Ĳ��� 
	}

	SCLK1; 
		return dat;
}

void reset_AD7705(void)
{
      unsigned char i;
      SPISendByte(0x01);
      DIN1;
      for(i=0; i<36; i++)
      {
          SCLK0;
		  SCLK0;
          SCLK1;
      }     
      Delay_Us(160);//500US
}


void AD7705_init1(void)
{
	 reset_AD7705();
	// Delay_Ms(1);
	 SPISendByte(0x20);
	 SPISendByte(0x04); 
	 SPISendByte(0x10);
	 SPISendByte(0x44);	
	 while(DRDY);
}

void AD7705_init2(void)
{
	 reset_AD7705();
//	 Delay_Ms(1);
	 SPISendByte(0x21);
	 SPISendByte(0x06); 
	 SPISendByte(0x11);
	 SPISendByte(0x44);
	 while(DRDY);
}


unsigned int Read_datareg1(void)
{
	unsigned int n=0;
//	 while(DRDY);
	 SPISendByte(0x38);
	 while(DRDY);
	 n = SPIRecv16bit();
	 return n;
}

unsigned int Read_datareg2(void)
{
	unsigned int n=0;
//	 while(DRDY);
	 SPISendByte(0x39);//0X39��һ����ͨ��2���ݼĴ�����0x38��ͨ��1����
	 while(DRDY);
	 n = SPIRecv16bit();
	 return n;
}


