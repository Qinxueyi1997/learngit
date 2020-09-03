/*---------------------------------*/
#ifndef _ad7705_h
#define _ad7705_h

#define	SCLK	19
#define	DIN	   23
#define	DOUT1	25
#define	DRDY1	21
#define SCLK0 gpio_set_level(SCLK, 0)
#define SCLK1 gpio_set_level(SCLK, 1)
#define DIN0 gpio_set_level(DIN, 0)
#define DIN1 gpio_set_level(DIN, 1)
#define DOUT gpio_get_level(DOUT1)
#define DRDY gpio_get_level(DRDY1)
void Delay_Us(unsigned int us);
void Delay_Ms(unsigned int ms);
void SPISendByte(unsigned char dat);
unsigned char SPIRecvByte(void);
void reset_AD7705(void);
void AD7705_init1(void);
void AD7705_init2(void);
unsigned int Read_datareg1(void);
unsigned int Read_datareg2(void);
//unsigned char Read_tongxinreg(void);
unsigned int SPIRecv16bit(void);
#endif


