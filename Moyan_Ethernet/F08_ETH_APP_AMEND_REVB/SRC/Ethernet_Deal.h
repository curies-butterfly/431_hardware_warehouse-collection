/*
 *  Ethernet_Deal.*
 *
 *  Created on: 2021年5月22日
 *  Author: ZLL
 */
#ifndef _Ethernet_Deal_H_
#define _Ethernet_Deal_H_
#define u8 unsigned char
#define MaxBuffSize  128
#define true 1
#define false 0
/***********************数据包宏定义**********************/
/*#define TYP_CNT     143   //用电器识别包字节数
#define ARC_CNT     20    //电弧包字节数

#define TYP_FLAG_H    0xAA  //用电器识别包头
#define TYP_FLAG_L    0xBB  //用电器识别包头
#define ARC_FLAG_H    0xBB  //电弧包头
#define ARC_FLAG_L    0xAA  //电弧包头
#define END_TAG_H     0xEE  //包尾
#define END_TAG_L     0xFF  //包尾*/

#define SOCK_TCPS			0
/*************************************
 *描述:LED定义
 *	 		红	绿	| 蓝
 *电弧 		闪	灭	| 亮		 连接成功
 *超载 		亮	灭	| 闪或灭	 无连接
 *故障 		亮	闪	|
 ************************************/
/***********LED&Beep宏定义************************************/
#define Green_Led_OFF GpioDataRegs.GPASET.bit.GPIO10 = 1;
#define Green_Led_ON GpioDataRegs.GPACLEAR.bit.GPIO10 = 1;
#define Red_Led_OFF GpioDataRegs.GPASET.bit.GPIO7 = 1;
#define Red_Led_ON GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;
#define Blue_Led_OFF GpioDataRegs.GPASET.bit.GPIO8 = 1;
#define Blue_Led_ON GpioDataRegs.GPACLEAR.bit.GPIO8 = 1;
#define BEEP_OFF GpioDataRegs.GPACLEAR.bit.GPIO12 = 1;
#define BEEP_ON GpioDataRegs.GPASET.bit.GPIO12 = 1;
#define Blue_LED_toggle GpioDataRegs.GPATOGGLE.bit.GPIO8 = 1
#define Red_Led_toggle GpioDataRegs.GPATOGGLE.bit.GPIO7 = 1
#define Green_Led_toggle GpioDataRegs.GPATOGGLE.bit.GPIO10 = 1
/***********Relay宏定义*********************************/
#define	Realy1_OFF	GpioDataRegs.GPACLEAR.bit.GPIO13 = 1;
#define	Realy1_ON	GpioDataRegs.GPASET.bit.GPIO13 = 1;
#define	Realy2_OFF	GpioDataRegs.GPACLEAR.bit.GPIO23 = 1;
#define	Realy2_ON	GpioDataRegs.GPASET.bit.GPIO23 = 1;
#define	Realy3_OFF	GpioDataRegs.GPACLEAR.bit.GPIO27 = 1;
#define	Realy3_ON	GpioDataRegs.GPASET.bit.GPIO27 = 1;
/******************输入定义******************************/
#define KEY1_IN  GpioDataRegs.GPADAT.bit.GPIO30
#define KEY2_IN  GpioDataRegs.GPADAT.bit.GPIO31
/************数据buff定义*******************************/
extern u8 OnOff_1; //继电器状态
extern u8 OnOff_2;
extern u8 OnOff_3;
extern u8 uploadFlag; //发送标志
extern u8 SigTobuff[4];
extern u8 WizDealBuffer[MaxBuffSize];//Dsp数据接收缓冲区
extern int pBuffLen; //缓冲区现存数据长度
extern char key1_push;//自动还是手动翻页
/***********函数声明***************/
void Deal_Recv(u8 *gDATABUF);
u8 ErrDec(u8 ID);



void gpio_config(void);
void spi_init(void);
void Reset_W5500(void);
void InitSpiGpio(void);
void delay_loop(long k);
int Deal_Recvnew(void);
int KEY_Scan(void);

#endif	/*Ethernet_Del*/
