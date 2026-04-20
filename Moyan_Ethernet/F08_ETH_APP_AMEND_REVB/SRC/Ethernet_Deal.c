#include "Ethernet_Deal.h"
#include "DSP2833x_Device.h"     // DSP2833x 头文件
#include "DSP2833x_Examples.h"   // DSP2833x 例子相关头文件
#include "FFT.h"
#include "string.h"
#include "stdio.h"
#include "socket.h"
#include "NTC.h"
#include "I2cEeprom.h"
#include "SCI.h"
#include "Data_Frame.h"
#include "BootLoader.h"
#include "main.h"
#include "LCD.h"
/*********************数据定义**************************/
//继电器通断状态，总闸开关(1-继电器闭合，后级电路断开    0-继电器断开，后级电路闭合)
u8 OnOff_1=0;
u8 OnOff_2=0;
u8 OnOff_3=0;
u8 uploadFlag = 0; //发送标志
u8 SigTobuff[4] = {0,0,0,0};
/***********************END****************************/
u8 WizDealBuffer[MaxBuffSize];//Dsp数据接收缓冲区
int pBuffLen = 0; //缓冲区现存数据长度
/**********************************
 * 描述：计算故障码
 * 输入：传感器ID
 * 返回：无
 ***********************************/
u8 ErrDec(u8 ID)
{
	u8 Code = 0;
	switch(ID)
	{
		case 0:
			//提qu7654210位
			Code = ((Err_Code & (uint32)0x0000F400) >> 8) + (Err_Code & 0x03);
			break;
		case 1:
			Code = ((Err_Code & (uint32)0x00F40000) >> 16) + (Err_Code & 0x03);
			break;
		case 2:
			Code = ((Err_Code & (uint32)0xF4000000) >> 24) + (Err_Code & 0x03);
			break;
		default:
			break;
	}
	return Code;
}
void gpio_config(void)
{
	EALLOW;
	/**************W5500相关引脚初始化*********************/
	GpioCtrlRegs.GPADIR.bit.GPIO19 = 1;
	GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0;
	GpioCtrlRegs.GPADIR.bit.GPIO20 = 1;
	GpioCtrlRegs.GPADIR.bit.GPIO18 = 0;

	/***************将LED灯管脚配置成GPIO*****************/
	GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 0;  // Red LED
	GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 0;  //Blue LED
	GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 0;  // Green LED
	//配置方向为输出
	GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;
	GpioCtrlRegs.GPADIR.bit.GPIO8 = 1;
	GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;
	//默认高电平
	GpioDataRegs.GPASET.bit.GPIO7 = 1;
	GpioDataRegs.GPASET.bit.GPIO8 = 1;
	GpioDataRegs.GPASET.bit.GPIO10 = 1;

	/***************继电器的IO初始化***********************/
	//配置为GPIO
	GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 0;  // GPIO13  Relay1
	GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 0;  // GPIO23  Relay2
	GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 0;  // GPIO27  Relay3
	//配置为输出
	GpioCtrlRegs.GPADIR.bit.GPIO13 = 1;
	GpioCtrlRegs.GPADIR.bit.GPIO23 = 1;
	GpioCtrlRegs.GPADIR.bit.GPIO27 = 1;
	//默认低电平，继电器断开
	GpioDataRegs.GPACLEAR.bit.GPIO13 = 1;
	GpioDataRegs.GPACLEAR.bit.GPIO23 = 1;
	GpioDataRegs.GPACLEAR.bit.GPIO27 = 1;

	/****************蜂鸣器初始化**************************/
	GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0;
	GpioCtrlRegs.GPADIR.bit.GPIO12 = 1;
	GpioDataRegs.GPACLEAR.bit.GPIO12 = 1;

	/**********输入按钮*************/
	//
	GpioCtrlRegs.GPAMUX2.bit.GPIO31=0;//普通IO模式
	GpioCtrlRegs.GPAPUD.bit.GPIO31=0;//使能内部上拉
	GpioCtrlRegs.GPADIR.bit.GPIO31=0;//配置成输入
	//
	GpioCtrlRegs.GPAMUX2.bit.GPIO30=0;//普通IO模式
	GpioCtrlRegs.GPAPUD.bit.GPIO30=0;//使能内部上拉
	GpioCtrlRegs.GPADIR.bit.GPIO30=0;//配置成输入
	 EDIS;
}
#define KEY_TIME_MAX 100
char key1_flag = -1;
char key2_flag = -1;
char key1_push = 0;//表示按键1是否按过；1表示停止自动翻页，0表示继续自动翻页
Uint16 key_time_cnt1 = 0;
Uint16 key_time_cnt2 = 0;
int KEY_Scan(void)
{
	//停止翻页
	if(!KEY1_IN)
	{
		key1_flag = 0;
		key_time_cnt1 = 0;
	}
	else if(key1_flag==0)
	{
		if(key1_push == 1)//在手动模式下
		{
			key_time_cnt1++;//延时计数器开始延时计数
			if(key_time_cnt1>KEY_TIME_MAX)//达到最大阀值(认为是按键真的被按下了)
			{
				key_time_cnt1=0;//计数器清零
				key1_flag=1;//按键标志置1，防止一直触发，
				LCD_Index++;
				if(LCD_Index == 100)
					LCD_Index-=60;
				LCD_Task(LCD_Index%4 + 1);
			}
		}
	}
	//在停止时翻页反，应该去除有bug
	if(!KEY2_IN)
	{
		key2_flag = 0;
		key_time_cnt2 = 0;
	}
	else if(key2_flag==0)
	{
		key_time_cnt2++;//延时计数器开始延时计数
		if(key_time_cnt2>KEY_TIME_MAX)//达到最大阀值(认为是按键真的被按下了)
		{
			key_time_cnt2=0;//计数器清零
			key2_flag=1;//按键标志置1，防止一直触发，
			if(key1_push == 1)//手动变自动
				key1_push = 0;
			else//自动变手动
				key1_push = 1;
//			LCD_Index--;
//			if(LCD_Index == 0)
//				LCD_Index+=60;
//			LCD_Task(LCD_Index%4 + 1);
		}
	}
	return 0;
}


void spi_init()
{
	SpiaRegs.SPICCR.bit.SPISWRESET = 0;//
	SpiaRegs.SPICCR.all = 0x0047;		//The SPI software resets the polarity bit
											//to 1 (sending data along the falling edge),
											//moving in and out of the 8 bit word length each time,
											//and prohibiting the SPI internal loopback (LOOKBACK) function;
	SpiaRegs.SPICTL.all = 0x0006;		// Enable master mode, normal phase, // enable talk, and SPI int disabled.
	SpiaRegs.SPISTS.all = 0x0000;		//溢出中断，禁止SPI中断；
	SpiaRegs.SPIBRR = 0x001F;			//SPI波特率=37.5M/24=1.5MHZ；
	SpiaRegs.SPIPRI.bit.FREE = 1;		//Set so breakpoints don't disturb xmission
	SpiaRegs.SPICCR.bit.SPISWRESET = 1;
}



void delay_loop(long k)
{
    long     i,j;
    for (i = 0; i < 10000; i++)
    {
    	for (j = 0; j < k; j++);
    }
}


void Reset_W5500(void)
{
	GpioDataRegs.GPADAT.bit.GPIO20 = 0;
	DELAY_US(100000);
	GpioDataRegs.GPADAT.bit.GPIO20 = 1;
	DELAY_US(1000000);
}
void InitSpiGpio()
{
   InitSpiaGpio();
}

void InitSpiaGpio()
{

   EALLOW;
/* Enable internal pull-up for the selected pins */
// Pull-ups can be enabled or disabled by the user.
// This will enable the pullups for the specified pins.
// Comment out other unwanted lines.


//F08的引脚
    GpioCtrlRegs.GPAPUD.bit.GPIO16 = 0;   // Enable pull-up on GPIO16 (SPISIMOA)
    GpioCtrlRegs.GPAPUD.bit.GPIO17 = 0;   // Enable pull-up on GPIO17 (SPISOMIA)
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;   // Enable pull-up on GPIO18 (SPICLKA)
    GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0;   // Enable pull-up on GPIO19 (SPISTEA)

//335的引脚
//    GpioCtrlRegs.GPBPUD.bit.GPIO54 = 0;   // Enable pull-up on GPIO54 (SPISIMOA)
//    GpioCtrlRegs.GPBPUD.bit.GPIO55 = 0;   // Enable pull-up on GPIO55 (SPISOMIA)
//    GpioCtrlRegs.GPBPUD.bit.GPIO56 = 0;   // Enable pull-up on GPIO56 (SPICLKA)
//    GpioCtrlRegs.GPBPUD.bit.GPIO57 = 0;   // Enable pull-up on GPIO57 (SPISTEA)


/* Set qualification for selected pins to asynch only */
// This will select asynch (no qualification) for the selected pins.
// Comment out other unwanted lines.

//F08的引脚
    GpioCtrlRegs.GPAQSEL2.bit.GPIO16 = 3; // Asynch input GPIO16 (SPISIMOA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO17 = 3; // Asynch input GPIO17 (SPISOMIA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 3; // Asynch input GPIO18 (SPICLKA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO19 = 3; // Asynch input GPIO19 (SPISTEA)
//335的引脚
//    GpioCtrlRegs.GPBQSEL2.bit.GPIO54 = 3; // Asynch input GPIO54 (SPISIMOA)
//    GpioCtrlRegs.GPBQSEL2.bit.GPIO55 = 3; // Asynch input GPIO55 (SPISOMIA)
//    GpioCtrlRegs.GPBQSEL2.bit.GPIO56 = 3; // Asynch input GPIO56 (SPICLKA)
//    GpioCtrlRegs.GPBQSEL2.bit.GPIO57 = 3; // Asynch input GPIO57 (SPISTEA)


/* Configure SPI-A pins using GPIO regs*/
// This specifies which of the possible GPIO pins will be SPI functional pins.
// Comment out other unwanted lines.
//F08的引脚
    GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1; // Configure GPIO16 as SPISIMOA
    GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 1; // Configure GPIO17 as SPISOMIA
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1; // Configure GPIO18 as SPICLKA
    GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 0; // Configure GPIO19 as SPISTEA
    //335的引脚
//    GpioCtrlRegs.GPBMUX2.bit.GPIO54 = 1; // Configure GPIO54 as SPISIMOA
//    GpioCtrlRegs.GPBMUX2.bit.GPIO55 = 1; // Configure GPIO55 as SPISOMIA
//    GpioCtrlRegs.GPBMUX2.bit.GPIO56 = 1; // Configure GPIO56 as SPICLKA
//    GpioCtrlRegs.GPBMUX2.bit.GPIO57 = 0; // Configure GPIO57 as SPISTEA
    //GpioCtrlRegs.GPBMUX2.bit.GPIO57 = 1;
    //GpioDataRegs.GPBDAT.bit.GPIO56 = 1;

    EDIS;
}


int StringToHex(char *str, unsigned char *out, unsigned int *outlen)
{
    char *p = str;
    char high = 0, low = 0;
    int tmplen = strlen(p), cnt = 0;
    tmplen = strlen(p);
    while(cnt < (tmplen / 2))
    {
        high = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;
		low = (*(++ p) > '9' && ((*p <= 'F') || (*p <= 'f'))) ? *(p) - 48 - 7 : *(p) - 48;
        out[cnt] = ((high & 0x0f) << 4 | (low & 0x0f));
        p ++;
        cnt ++;
    }
    if(tmplen % 2 != 0) out[cnt] = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;

    if(outlen != NULL) *outlen = tmplen / 2 + tmplen % 2;
    return tmplen / 2 + tmplen % 2;
}
int FilterDC(float *ADC,int N)//去除数据中的直流成分，否则直流分量将很大
{
   int i;
   float sum=0;
   printf("read start\n");
   for(i=0;i<N;i++)
    { sum+=ADC[i];
    printf(" %.2f,",ADC[i]);
    }

   sum=sum/N;
   /*for(i=0;i<N;i++)
    { ADC[i]-=sum;}*/
   //sum = fabs(sum*20);
   printf("\n avr=%.2f\n",sum);
   sum = sum*20;
   return (int)sum;
}
void correcting_init_value()
{
	char temp_char = 0;
	Uint16 temp = 0;
	//零点位置
	temp_char = FilterDC(I_Array1,SampCnt);
	SCI_Printf("old ZeroCV1=%d,ZeroCV2=%d ,ZeroCV3=%d\r\n",ZeroCV1,ZeroCV2,ZeroCV3);
	temp_char = ZeroCV1 - temp_char;
	AT24CXX_WriteData(ZeroCV1_Addr,temp_char);
	temp_char = FilterDC(I_Array2,SampCnt);
	temp_char = ZeroCV2 - temp_char;
	AT24CXX_WriteData(ZeroCV2_Addr,temp_char);
	temp_char = FilterDC(I_Array3,SampCnt);
	temp_char = ZeroCV3 - temp_char;
	AT24CXX_WriteData(ZeroCV3_Addr,temp_char);
	//底噪值
	temp = 0;
	AT24CXX_WriteData(Correction_value_Addr1,temp);
	AT24CXX_WriteData(Correction_value_Addr2,temp);
	AT24CXX_WriteData(Correction_value_Addr3,temp);
	//
	AT24CXX_WriteData(CntAPP2Addr,temp);  //进入APP2次数
	AT24CXX_WriteData(isCLR_REBOOT_TIMES_ADDRESS,temp);//
	AT24CXX_WriteData(REBOOT_TIMES_ADDRESS,temp);//断网已重启次数
	//读出值并打印
	ZeroCV1  = AT24CXX_ReadData(ZeroCV1_Addr);
	ZeroCV2 = AT24CXX_ReadData(ZeroCV2_Addr);
	ZeroCV3 = AT24CXX_ReadData(ZeroCV3_Addr);
	Correction_value_1 = AT24CXX_ReadData(Correction_value_Addr1);
	Correction_value_2 = AT24CXX_ReadData(Correction_value_Addr2);
	Correction_value_3 = AT24CXX_ReadData(Correction_value_Addr3);
	SCI_Printf("correct = %d %d %d(0默认)\n",Correction_value_1,Correction_value_2,Correction_value_3);
	SCI_Printf("Zero = %d %d %d(20默认)\n",ZeroCV1,ZeroCV2,ZeroCV3);
	temp = AT24CXX_ReadData(CntAPP2Addr);
	SCI_Printf("APP2 Cnt = %d \n",temp);
	temp = AT24CXX_ReadData(isCLR_REBOOT_TIMES_ADDRESS);
	SCI_Printf("isCLR_REBOOT_TIMES = %d \n",temp);
	temp = AT24CXX_ReadData(REBOOT_TIMES_ADDRESS);
	SCI_Printf("REBOOT_TIMES = %d \n",temp);
}


#define JDQ_CNT 4
#define AC_CNT 3
#define AD_CNT 3
#define RS_CNT 2
#define T1_CNT 2
#define BD_CNT 3
#define VERS_CNT 3
#define RECV_CNT 4
#define CR_CNT 3
#define CRPD_CNT 4
#define DFT_CNT 3
#define SGNT_CNT 12
#define AR_CNT 4
#define APP_CNT 10
int Deal_Recvnew(void)
{
	Uint16 temp = 0;
	char temp_char = 0;
//	u8 temp_u8 = 0;
	char tbuff[8] = {0,0,0,0,0,0,0,0};
	Uint16 Length = 4;
	u8 DealReturnBuff[2];
	DealReturnBuff[0] ='A';
	DealReturnBuff[1] ='E';
	u8 bFindHead = true;
	int i = 0;
	int j = 0;
	if(pBuffLen<=0) //缓冲区无数据
		return 1;
	for(i=0;i<pBuffLen-1;i++) //遍历寻找包头
	{
		bFindHead = false;
		//AB1
		if(WizDealBuffer[i] =='A' && WizDealBuffer[i+1] =='B'&& WizDealBuffer[i+3] =='1') //k继电器
		{
			bFindHead = true;
			if(WizDealBuffer[i+2]=='0')
			{
				printf("AB01\n");
				Realy1_ON;//继电器闭合，接触器断开
				send(SOCK_TCPS,DealReturnBuff,2);
				OnOff_1 = 1;
				AT24CXX_WriteData(Relay1_addr,0x01);
			}
			else if(WizDealBuffer[i+2]=='1')
			{
				printf("AB11\n");
				Realy2_ON;//继电器闭合，接触器断开
				send(SOCK_TCPS,DealReturnBuff,2);
				OnOff_2=1;
				AT24CXX_WriteData(Relay2_addr,0x01);
			}
			else if(WizDealBuffer[i+2]=='2')
			{
				printf("AB21\n");
				Realy3_ON;//继电器闭合，接触器断开
				send(SOCK_TCPS,DealReturnBuff,2);
				OnOff_3 =1;
				AT24CXX_WriteData(Relay3_addr,0x01);
			}
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+JDQ_CNT,pBuffLen-JDQ_CNT);//移除处理完的帧
			pBuffLen-=JDQ_CNT;
			break;
		}
		//AB0
		if((WizDealBuffer[i+0]=='A')&&(WizDealBuffer[i+1]=='B')&&(WizDealBuffer[i+3]=='0'))
		{
			bFindHead = true;
			if(WizDealBuffer[i+2]=='0')
			{
				printf("AB00\n");
				Realy1_OFF;//继电器断开，接触器闭合
				send(SOCK_TCPS,DealReturnBuff,2);
				OnOff_1 = 0;
				AT24CXX_WriteData(Relay1_addr,0x00);
			}
			else if(WizDealBuffer[i+2]=='1')
			{
				printf("AB10\n");
				Realy2_OFF;//继电器断开，接触器闭合
				send(SOCK_TCPS,DealReturnBuff,2);
				OnOff_2 = 0;
				AT24CXX_WriteData(Relay2_addr,0x00);
			}
			else if(WizDealBuffer[i+2]=='2')
			{
				printf("AB20\n");
				Realy3_OFF;//继电器断开，接触器闭合
				send(SOCK_TCPS,DealReturnBuff,2);
				OnOff_3 = 0;
				AT24CXX_WriteData(Relay3_addr,0x00);
			}
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+JDQ_CNT,pBuffLen-JDQ_CNT);//移除处理完的帧
			pBuffLen-=JDQ_CNT;
			break;
		}
		//AC
		if((WizDealBuffer[i+0]=='A')&&(WizDealBuffer[i+1]=='C'))
		{
			bFindHead = true;
			if(WizDealBuffer[i+2]=='0')
			{
				printf("AC0\n");
				send(SOCK_TCPS,DealReturnBuff,2);
				BEEP_OFF;
			}
			else if(WizDealBuffer[i+2]=='1')
			{
				printf("AC1\n");
				send(SOCK_TCPS,DealReturnBuff,2);
				BEEP_ON;
			}
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+AC_CNT,pBuffLen-AC_CNT);//移除处理完的帧
			pBuffLen-=AC_CNT;
			break;
		}
		//AD
		if((WizDealBuffer[i+0]=='A')&&(WizDealBuffer[i+1]=='D'))
		{
			bFindHead = true;
			if(WizDealBuffer[i+2]=='0')//是否上发数据（1发0不发）
			{
				printf("AD0\n");
				uploadFlag = 0;
			}
			else if(WizDealBuffer[i+2]=='1')
			{
				printf("AD1\n");
				SCI_Printf("AD1\n");
				uploadFlag = 1;
			}
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+AD_CNT,pBuffLen-AD_CNT);//移除处理完的帧
			pBuffLen-=AD_CNT;
			break;
		}
		//RS
		if((WizDealBuffer[i+0]=='R')&&(WizDealBuffer[i+1]=='S'))
		{
				printf("RS1\n");
				bFindHead = true;
				SoftwareReset();
		}
		//T1
		if((WizDealBuffer[i+0]=='T')&&(WizDealBuffer[i+1]=='1'))
		{
			/***********注意：CCS6.0自带的printf不可以使用%f********************/
			bFindHead = true;
			SCI_Printf("电压值1 ： %d \r\n",(int)UA1);
			SCI_Printf("电压值2 ： %d \r\n",(int)UA2);
			SCI_Printf("电压值3 ： %d \r\n",(int)UA3);
			SCI_Printf("功率1 ：%d \r\n",(int)P_total1);
			SCI_Printf("功率2 ：%d \r\n",(int)P_total2);
			SCI_Printf("功率3 ：%d \r\n",(int)P_total3);
			SCI_Printf("温度1 ：%d \r\n",ntcfun_1());
			SCI_Printf("温度2 ：%d \r\n",ntcfun_2());
			SCI_Printf("温度3 ：%d \r\n",ntcfun_3());
			SCI_Printf("剩余电流 1 : %d \r\n",LeakgeI1);
			SCI_Printf("剩余电流 2 : %d \r\n",LeakgeI2);
			SCI_Printf("剩余电流 3 : %d \r\n",LeakgeI3);
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+T1_CNT,pBuffLen-T1_CNT);//移除处理完的帧
			pBuffLen-=T1_CNT;
			break;
		}
		//BD2
		if((WizDealBuffer[i+0]=='B')&&(WizDealBuffer[i+1]=='D')&&(WizDealBuffer[i+2]=='2'))
		{
			bFindHead = true;
			SCI_Printf("收到BD2\r\n");
			jumptoBOOT();//跳转到IAP
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+BD_CNT,pBuffLen-BD_CNT);//移除处理完的帧
			pBuffLen-=BD_CNT;
			break;
		}
		//VER
		if((WizDealBuffer[i+0]=='V')&&(WizDealBuffer[i+1]=='E')&&(WizDealBuffer[i+2]=='R'))
		{
			bFindHead = true;
			SCI_Printf("收到VER\r\n");
			LAN_RetVersion();
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+VERS_CNT,pBuffLen-VERS_CNT);//移除处理完的帧
			pBuffLen-=VERS_CNT;
			break;
		}
		//RECV
		if((WizDealBuffer[i+0]=='R')&&(WizDealBuffer[i+1]=='E')&&
			   (WizDealBuffer[i+2]=='C')&&(WizDealBuffer[i+3]=='V'))
		{
			//手动下载了用户程序
			// 发送此命令  表示出厂固件程序已下载
			// 恢复出厂固件命令
			//Set_flash_APP1_flag();//置APP1 flash标志为0
			//NVIC_SystemReset();//复位  恢复出厂固件
			bFindHead = true;
			SCI_Printf("收到RECV\n");
			AT24CXX_WriteData(BL_Status_ADDRESS,BL_APP1);
			SoftwareReset();
		}
		//CR
		/************************************************
		 * CRW:write写入此时的底噪功率
		 * CRR:read读取底噪功率
		 * CRC:clear清除EEPROM中底噪值
		 * CRP:plus拉高零点
		 * CRD:down拉低零点
		 * CRS:set设置零点或手动填入零点
		 * 一般出厂先将波形零点校准到0附近
		 * 后使用前三个命令校准底噪
		 * 注意，EEPROM初值为FF，出厂前要DFT置零
		 */
		if((WizDealBuffer[i+0]=='C')&&(WizDealBuffer[i+1]=='R'))
		{
			bFindHead = true;
			if(WizDealBuffer[i+2] == 'W')//write
			{
				printf("收到CRW\n");
				CRR_Data_Process(1); //写入
				CRR_Data_Process(0); //读取
				printf("correct = %d %d %d\n",Correction_value_1,Correction_value_2,Correction_value_3);
			}
			if(WizDealBuffer[i+2] == 'R')//read
			{
				printf("收到CRR\n");
				CRR_Data_Process(0); //读取
				printf("correct = %d %d %d(0默认)\n",Correction_value_1,Correction_value_2,Correction_value_3);
				printf("Zero = %d %d %d(20默认)\n",ZeroCV1,ZeroCV2,ZeroCV3);
			}
			if(WizDealBuffer[i+2] == 'C')//clear
			{
				printf("收到CRC\n");
				CRR_Data_Process(2);
			}
			if(WizDealBuffer[i+2] == 'P')//拉高零点
			{
				printf("收到CRP\n");
				if(WizDealBuffer[i+3] == '0')
				{
					ZeroCV1++;
				}
				if(WizDealBuffer[i+3] == '1')
				{
					ZeroCV2++;
				}
				if(WizDealBuffer[i+3] == '2')
				{
					ZeroCV3++;
				}
				memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
				pBuffLen-=i;
				memcpy(WizDealBuffer,WizDealBuffer+CRPD_CNT,pBuffLen-CRPD_CNT);//移除处理完的帧
				pBuffLen-=CRPD_CNT;
				break;
			}
			if(WizDealBuffer[i+2] == 'D')//拉低零点
			{
				printf("收到CRD\n");
				if(WizDealBuffer[i+3] == '0')
				{
					if(ZeroCV1 > 0)
						ZeroCV1--;
					else
						printf("Reach the limit\n");
				}
				if(WizDealBuffer[i+3] == '1')
				{
					if(ZeroCV2 > 0)
						ZeroCV2--;
					else
						printf("Reach the limit\n");
				}
				if(WizDealBuffer[i+3] == '2')
				{
					if(ZeroCV3 > 0)
						ZeroCV3--;
					else
						printf("Reach the limit\n");
				}
				memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
				pBuffLen-=i;
				memcpy(WizDealBuffer,WizDealBuffer+CRPD_CNT,pBuffLen-CRPD_CNT);//移除处理完的帧
				pBuffLen-=CRPD_CNT;
				break;
			}
			if(WizDealBuffer[i+2] == 'S')//设置零点
			{
				printf("收到CRS\n");
				AT24CXX_WriteData(ZeroCV1_Addr,ZeroCV1);
				AT24CXX_WriteData(ZeroCV2_Addr,ZeroCV2);
				AT24CXX_WriteData(ZeroCV3_Addr,ZeroCV3);
				printf("Zero = %d %d %d\n",ZeroCV1,ZeroCV2,ZeroCV3);
			}
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+CR_CNT,pBuffLen-CR_CNT);//移除处理完的帧
			pBuffLen-=CR_CNT;
			break;
		}
		//DFT
		if((WizDealBuffer[i+0]=='D')&&(WizDealBuffer[i+1]=='F')&&  (WizDealBuffer[i+2]=='T'))  //恢复EEPROM
		{
			bFindHead = true;
			printf("收到DEFAT\n");
			printf("old ZeroCV1=%d,ZeroCV2=%d ,ZeroCV3=%d\r\n",ZeroCV1,ZeroCV2,ZeroCV3);
			//零点位置
			temp_char = FilterDC(I_Array1,SampCnt);
			temp_char = ZeroCV1 - temp_char;
			AT24CXX_WriteData(ZeroCV1_Addr,temp_char);
			temp_char = FilterDC(I_Array2,SampCnt);
			temp_char = ZeroCV2 - temp_char;
			AT24CXX_WriteData(ZeroCV2_Addr,temp_char);
			temp_char = FilterDC(I_Array3,SampCnt);
			temp_char = ZeroCV3 - temp_char;
			AT24CXX_WriteData(ZeroCV3_Addr,temp_char);
			//底噪值
			temp = 0;
			AT24CXX_WriteData(Correction_value_Addr1,temp);
			AT24CXX_WriteData(Correction_value_Addr2,temp);
			AT24CXX_WriteData(Correction_value_Addr3,temp);
			//
			AT24CXX_WriteData(CntAPP2Addr,temp);  //进入APP2次数
			AT24CXX_WriteData(isCLR_REBOOT_TIMES_ADDRESS,temp);//
			AT24CXX_WriteData(REBOOT_TIMES_ADDRESS,temp);//断网已重启次数
			//读出值并打印
			ZeroCV1  = AT24CXX_ReadData(ZeroCV1_Addr);
			ZeroCV2 = AT24CXX_ReadData(ZeroCV2_Addr);
			ZeroCV3 = AT24CXX_ReadData(ZeroCV3_Addr);
			Correction_value_1 = AT24CXX_ReadData(Correction_value_Addr1);
			Correction_value_2 = AT24CXX_ReadData(Correction_value_Addr2);
			Correction_value_3 = AT24CXX_ReadData(Correction_value_Addr3);
			printf("correct = %d %d %d(0默认)\n",Correction_value_1,Correction_value_2,Correction_value_3);
			printf("Zero = %d %d %d(20默认)\n",ZeroCV1,ZeroCV2,ZeroCV3);
			temp = AT24CXX_ReadData(CntAPP2Addr);
			printf("APP2 Cnt = %d \n",temp);
			temp = AT24CXX_ReadData(isCLR_REBOOT_TIMES_ADDRESS);
			printf("isCLR_REBOOT_TIMES = %d \n",temp);
			temp = AT24CXX_ReadData(REBOOT_TIMES_ADDRESS);
			printf("REBOOT_TIMES = %d \n",temp);
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+DFT_CNT,pBuffLen-DFT_CNT);//移除处理完的帧
			pBuffLen-=DFT_CNT;
			break;
		}
		//SGNT
		if((WizDealBuffer[i+0]=='S')&&(WizDealBuffer[i+1]=='G')&&
			  		   (WizDealBuffer[i+2]=='N')&&(WizDealBuffer[i+3]=='T'))  //信号质量
		{
			bFindHead = true;
			SCI_Printf("SGNT\n");
			RST_Count_Base = 0;//复位计数置零
			for(j=4;j<12;j++)
			{
				tbuff[j-4] = WizDealBuffer[i+j];
			}
			Length = 4;
			StringToHex(tbuff,SigTobuff,&Length);
			LAN_RetSGN();
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+SGNT_CNT,pBuffLen-SGNT_CNT);//移除处理完的帧
			pBuffLen-=SGNT_CNT;
			break;
		}
		//AR
		if((WizDealBuffer[i+0]=='A')&&(WizDealBuffer[i+1]=='R'))  //电弧单周期阈值
		{
			bFindHead = true;
			SCI_Printf("AR\n");
			Length = 1;
			//temp_char = WizDealBuffer[i+3];
			//StringToHex(&temp_char,&temp_u8,&Length);
			if(WizDealBuffer[i+2]=='0')
			{
				//if(temp_u8) SingleArcTH1++;
				//else SingleArcTH1--;
				if(WizDealBuffer[i+3]=='1')
				{
					SingleArcTH1++;
					AT24CXX_WriteData(ArcTH1_Addr,SingleArcTH1);
					SCI_Printf("TH1=%d\n",SingleArcTH1);
				}
				else if(WizDealBuffer[i+3]=='0')
				{
					SingleArcTH1--;
					AT24CXX_WriteData(ArcTH1_Addr,SingleArcTH1);
					SCI_Printf("TH1=%d\n",SingleArcTH1);
				}
				else if(WizDealBuffer[i+3]=='F')
				{
					Err_Code = Err_Code|gBIT12;
					//SCI_Printf("1F%d\n");
				}
				send(SOCK_TCPS,DealReturnBuff,2);
			}
			if(WizDealBuffer[i+2]=='1')
			{
				if(WizDealBuffer[i+3]=='1')
				{
					SingleArcTH2++;
					AT24CXX_WriteData(ArcTH2_Addr,SingleArcTH2);
					SCI_Printf("TH2=%d\n",SingleArcTH2);
				}
				else if(WizDealBuffer[i+3]=='0')
				{
					SingleArcTH2--;
					AT24CXX_WriteData(ArcTH2_Addr,SingleArcTH2);
					SCI_Printf("TH2=%d\n",SingleArcTH2);
				}
				else if(WizDealBuffer[i+3]=='F')
				{
					Err_Code = Err_Code|gBIT20;
					//SCI_Printf("2F%d\n");
				}
				send(SOCK_TCPS,DealReturnBuff,2);
			}
			if(WizDealBuffer[i+2]=='2')
			{
				if(WizDealBuffer[i+3]=='1')
				{
					SingleArcTH3++;
					AT24CXX_WriteData(ArcTH3_Addr,SingleArcTH3);
					SCI_Printf("TH3=%d\n",SingleArcTH3);
				}
				else if(WizDealBuffer[i+3]=='0')
				{
					SingleArcTH3--;
					AT24CXX_WriteData(ArcTH3_Addr,SingleArcTH3);
					SCI_Printf("TH3=%d\n",SingleArcTH3);
				}
				else if(WizDealBuffer[i+3]=='F')
				{
					Err_Code = Err_Code|gBIT28;
					//SCI_Printf("3F%d\n");
				}
				send(SOCK_TCPS,DealReturnBuff,2);
			}
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+AR_CNT,pBuffLen-AR_CNT);//移除处理完的帧
			pBuffLen-=AR_CNT;
			break;
		}
		//APP
		if((WizDealBuffer[i]=='A')&&(WizDealBuffer[i+1]=='P')&&(WizDealBuffer[i+2]=='P'))  //回传用电器信息
		{
			bFindHead = true;
			if(WizDealBuffer[i+3]=='0')
			{
			 // HBCode1 =  将五位字符串编码复制到HBCODE内 LCD中处理即可
			  strncpy(HBCode1,WizDealBuffer+i+4,6); //截取类别至具体用电器
			  isAPP1 = 1;
			  //SCI_Printf("APP1\n");
			}
			if(WizDealBuffer[i+3]=='1')
			{
			  strncpy(HBCode2,WizDealBuffer+i+4,6); //截取类别至具体用电器
			  isAPP2 = 1;
			  //SCI_Printf("APP2\n");
			}
			if(WizDealBuffer[i+3]=='2')
			{
			  strncpy(HBCode3,WizDealBuffer+i+4,6); //截取类别至具体用电器
			  isAPP3 = 1;
			  //SCI_Printf("APP3\n");
			}
			memcpy(WizDealBuffer,WizDealBuffer+i,pBuffLen-i);  //移除搜寻帧
			pBuffLen-=i;
			memcpy(WizDealBuffer,WizDealBuffer+APP_CNT,pBuffLen-APP_CNT);//移除处理完的帧
			pBuffLen-=APP_CNT;
			break;
		}
		if(bFindHead)
		{
			break;
		}
	}
	if(!bFindHead)
	{
	  //删除无效数据
		//memcpy(DSPData_Buffer,DSPData_Buffer+pBuffLen-1,1);
		pBuffLen = 1;
		return -1;
	}
	return 0;
}
/*void Deal_Recv(u8 *gDATABUF)
{
    //int i=0;
	Uint16 temp = 0;
	char temp_char = 0;
	u8 temp_u8 = 0;
	char tbuff[8] = {0,0,0,0,0,0,0,0};
	u8 i = 0;
	Uint16 Length = 4;
    u8 DealReturnBuff[2];
	DealReturnBuff[0] ='A';
	DealReturnBuff[1] ='E';
	if((gDATABUF[0]=='A')&&(gDATABUF[1]=='B')&&(gDATABUF[3]=='1'))
	{

		if(gDATABUF[2]=='0')
		{
			printf("AB01\n");
			Realy1_ON;//继电器闭合，接触器断开
			send(SOCK_TCPS,DealReturnBuff,2);
			OnOff_1 = 1;
			AT24CXX_WriteData(Relay1_addr,0x01);
			//ARC1_Buff[8] = OnOff_1;
		}
		else if(gDATABUF[2]=='1')
		{
			printf("AB11\n");
			Realy2_ON;//继电器闭合，接触器断开
			send(SOCK_TCPS,DealReturnBuff,2);
			OnOff_2=1;
			AT24CXX_WriteData(Relay2_addr,0x01);
			//ARC2_Buff[8] = OnOff_2;
		}
		else if(gDATABUF[2]=='2')
		{
			printf("AB21\n");
			Realy3_ON;//继电器闭合，接触器断开
			send(SOCK_TCPS,DealReturnBuff,2);
			OnOff_3 =1;
			AT24CXX_WriteData(Relay3_addr,0x01);
			//ARC3_Buff[8] = OnOff_3;
		}
	}
    if((gDATABUF[0]=='A')&&(gDATABUF[1]=='B')&&(gDATABUF[3]=='0'))
	{
	    if(gDATABUF[2]=='0')
		{
	    	printf("AB00\n");
			Realy1_OFF;//继电器断开，接触器闭合
			send(SOCK_TCPS,DealReturnBuff,2);
			OnOff_1 = 0;
			AT24CXX_WriteData(Relay1_addr,0x00);
		}
		else if(gDATABUF[2]=='1')
		{
			printf("AB10\n");
			Realy2_OFF;//继电器断开，接触器闭合
			send(SOCK_TCPS,DealReturnBuff,2);
			OnOff_2 = 0;
			AT24CXX_WriteData(Relay2_addr,0x00);
		}
		else if(gDATABUF[2]=='2')
		{
			printf("AB20\n");
			Realy3_OFF;//继电器断开，接触器闭合
			send(SOCK_TCPS,DealReturnBuff,2);
			OnOff_3 = 0;
			AT24CXX_WriteData(Relay3_addr,0x00);
		}
	}
	if((gDATABUF[0]=='A')&&(gDATABUF[1]=='C'))
	{
		if(gDATABUF[2]=='0')
		{
			printf("AC0\n");
			send(SOCK_TCPS,DealReturnBuff,2);
			BEEP_OFF;
		}
		else if(gDATABUF[2]=='1')
		{
			printf("AC1\n");
			send(SOCK_TCPS,DealReturnBuff,2);
			BEEP_ON;
		}
	}
	if((gDATABUF[0]=='A')&&(gDATABUF[1]=='D'))
	{
		if(gDATABUF[2]=='0')//是否上发数据（1发0不发）
		{
			printf("AD0\n");
			uploadFlag = 0;
		}
		else if(gDATABUF[2]=='1')
		{
			printf("AD1\n");
			SCI_Printf("AD1\n");
			uploadFlag = 1;
		}
	}

	if((gDATABUF[0]=='R')&&(gDATABUF[1]=='S'))
	{
			printf("RS1\n");
			SoftwareReset();
			    //NVIC_SystemReset();
	}

	if((gDATABUF[0]=='T')&&(gDATABUF[1]=='1'))
	{
		//注意：CCS6.0自带的printf不可以使用%f
		SCI_Printf("电压值1 ： %d \r\n",(int)UA1);
		SCI_Printf("电压值2 ： %d \r\n",(int)UA2);
		SCI_Printf("电压值3 ： %d \r\n",(int)UA3);
		SCI_Printf("功率1 ：%d \r\n",(int)P_total1);
		SCI_Printf("功率2 ：%d \r\n",(int)P_total2);
		SCI_Printf("功率3 ：%d \r\n",(int)P_total3);
		SCI_Printf("温度1 ：%d \r\n",ntcfun_1());
		SCI_Printf("温度2 ：%d \r\n",ntcfun_2());
		SCI_Printf("温度3 ：%d \r\n",ntcfun_3());

		SCI_Printf("剩余电流 1 : %d \r\n",ReturnLkgI(LeakgeI1));
		SCI_Printf("剩余电流 2 : %d \r\n",ReturnLkgI(LeakgeI2));
		SCI_Printf("剩余电流 3 : %d \r\n",ReturnLkgI(LeakgeI3));

	}

	if((gDATABUF[0]=='B')&&(gDATABUF[1]=='D')&&(gDATABUF[2]=='2'))
	{
		SCI_Printf("收到BD2\r\n");
		jumptoBOOT();//跳转到IAP
	}
	if((gDATABUF[0]=='V')&&(gDATABUF[1]=='E')&&(gDATABUF[2]=='R'))
	{
		SCI_Printf("收到VER\r\n");
		LAN_RetVersion();
	}

  if((gDATABUF[0]=='R')&&(gDATABUF[1]=='E')&&
	   (gDATABUF[2]=='C')&&(gDATABUF[3]=='V'))
	{
	    //手动下载了用户程序
		// 发送此命令  表示出厂固件程序已下载
		// 恢复出厂固件命令
		//Set_flash_APP1_flag();//置APP1 flash标志为0
		//NVIC_SystemReset();//复位  恢复出厂固件
	    SCI_Printf("收到RECV\n");
		AT24CXX_WriteData(BL_Status_ADDRESS,BL_APP1);
		SoftwareReset();
	}*/
/************************************************
 * CRW:write写入此时的底噪功率
 * CRR:read读取底噪功率
 * CRC:clear清除EEPROM中底噪值
 * CRP:plus拉高零点
 * CRD:down拉低零点
 * CRS:set设置零点或手动填入零点
 * 一般出厂先将波形零点校准到0附近
 * 后使用前三个命令校准底噪
 * 注意，EEPROM初值为FF，出厂前要DFT置零
 */
	/*if((gDATABUF[0]=='C')&&(gDATABUF[1]=='R'))
	{
		if(gDATABUF[2] == 'W')//write
		{
			printf("收到CRW\n");
			CRR_Data_Process(1); //写入
			CRR_Data_Process(0); //读取
			printf("correct = %d %d %d\n",Correction_value_1,Correction_value_2,Correction_value_3);
		}
		if(gDATABUF[2] == 'R')//read
		{
			printf("收到CRR\n");
			CRR_Data_Process(0); //读取
			printf("correct = %d %d %d(0默认)\n",Correction_value_1,Correction_value_2,Correction_value_3);
			printf("Zero = %d %d %d(20默认)\n",ZeroCV1,ZeroCV2,ZeroCV3);
		}
		if(gDATABUF[2] == 'C')//clear
		{
			printf("收到CRC\n");
			CRR_Data_Process(2);
		}
		if(gDATABUF[2] == 'P')//拉高零点
		{
			printf("收到CRP\n");
			if(gDATABUF[3] == '0')
			{
				ZeroCV1++;
			}
			if(gDATABUF[3] == '1')
			{
				ZeroCV2++;
			}
			if(gDATABUF[3] == '2')
			{
				ZeroCV3++;
			}
		}
		if(gDATABUF[2] == 'D')//拉低零点
		{
			printf("收到CRD\n");
			if(gDATABUF[3] == '0')
			{
				if(ZeroCV1 > 0)
					ZeroCV1--;
				else
					printf("Reach the limit\n");
			}
			if(gDATABUF[3] == '1')
			{
				if(ZeroCV2 > 0)
					ZeroCV2--;
				else
					printf("Reach the limit\n");
			}
			if(gDATABUF[3] == '2')
			{
				if(ZeroCV3 > 0)
					ZeroCV3--;
				else
					printf("Reach the limit\n");
			}
		}
		if(gDATABUF[2] == 'S')//设置零点
		{
			printf("收到CRS\n");
			//if(gDATABUF[3] == '0')
			//{
				AT24CXX_WriteData(ZeroCV1_Addr,ZeroCV1);
			//}
			//if(gDATABUF[3] == '1')
			//{
				AT24CXX_WriteData(ZeroCV2_Addr,ZeroCV2);
			//}
			//if(gDATABUF[3] == '2')
			//{
				AT24CXX_WriteData(ZeroCV3_Addr,ZeroCV3);
			//}
				printf("Zero = %d %d %d\n",ZeroCV1,ZeroCV2,ZeroCV3);
		}

	}

	  if((gDATABUF[0]=='D')&&(gDATABUF[1]=='F')&&
		   (gDATABUF[2]=='T'))  //恢复EEPROM
	  {
		  printf("收到DEFAT\n");
		  temp = 20;
		  AT24CXX_WriteData(ZeroCV1_Addr,temp);
		  AT24CXX_WriteData(ZeroCV2_Addr,temp);
		  AT24CXX_WriteData(ZeroCV3_Addr,temp);
		  temp = 0;
		  //底噪值
		  AT24CXX_WriteData(Correction_value_Addr1,temp);
		  AT24CXX_WriteData(Correction_value_Addr2,temp);
		  AT24CXX_WriteData(Correction_value_Addr3,temp);
		  //
		  AT24CXX_WriteData(CntAPP2Addr,temp);  //进入APP2次数
		  AT24CXX_WriteData(isCLR_REBOOT_TIMES_ADDRESS,temp);//
		  AT24CXX_WriteData(REBOOT_TIMES_ADDRESS,temp);//断网已重启次数

		  //读出值并打印
		  ZeroCV1  = AT24CXX_ReadData(ZeroCV1_Addr);
		  ZeroCV2 = AT24CXX_ReadData(ZeroCV2_Addr);
		  ZeroCV3 = AT24CXX_ReadData(ZeroCV3_Addr);
		  Correction_value_1 = AT24CXX_ReadData(Correction_value_Addr1);
		  Correction_value_2 = AT24CXX_ReadData(Correction_value_Addr2);
		  Correction_value_3 = AT24CXX_ReadData(Correction_value_Addr3);
		  printf("correct = %d %d %d(0默认)\n",Correction_value_1,Correction_value_2,Correction_value_3);
		  printf("Zero = %d %d %d(20默认)\n",ZeroCV1,ZeroCV2,ZeroCV3);
		  temp = AT24CXX_ReadData(CntAPP2Addr);
		  printf("APP2 Cnt = %d \n",temp);
		  temp = AT24CXX_ReadData(isCLR_REBOOT_TIMES_ADDRESS);
		  printf("isCLR_REBOOT_TIMES = %d \n",temp);
		  temp = AT24CXX_ReadData(REBOOT_TIMES_ADDRESS);
		  printf("REBOOT_TIMES = %d \n",temp);

	  }
	  if((gDATABUF[0]=='S')&&(gDATABUF[1]=='G')&&
	  		   (gDATABUF[2]=='N')&&(gDATABUF[3]=='T'))  //信号质量
	  {
		  //printf("SGNT\n");
		  SCI_Printf("SGNT\n");
		  RST_Count_Base = 0;//复位计数置零
		  for(i=4;i<12;i++)
		  {
			  tbuff[i-4] = gDATABUF[i];
		  }
		  //printf("t = %s ",tbuff);
		  Length = 4;
		  StringToHex(tbuff,SigTobuff,&Length);
		  LAN_RetSGN();
	  }
	  if((gDATABUF[0]=='A')&&(gDATABUF[1]=='R'))  //电弧单周期阈值
	  {
		  SCI_Printf("AR\n");
		  Length = 1;
		  temp_char = gDATABUF[3];
		  StringToHex(&temp_char,&temp_u8,&Length);
		  if(gDATABUF[2]=='0')
		  {
			  if(temp_u8) SingleArcTH1++;
			  else SingleArcTH1--;
			  //SingleArcTH1 = temp_u8 + ArcTHBase;
			  AT24CXX_WriteData(ArcTH1_Addr,SingleArcTH1);
			  SCI_Printf("TH1=%d\n",SingleArcTH1);
			  send(SOCK_TCPS,DealReturnBuff,2);
		  }
		  if(gDATABUF[2]=='1')
		  {
			  if(temp_u8) SingleArcTH2++;
			  else SingleArcTH2--;
			  //SingleArcTH2 = temp_u8 + ArcTHBase;
			  AT24CXX_WriteData(ArcTH2_Addr,SingleArcTH2);
			  SCI_Printf("TH2=%d\n",SingleArcTH2);
			  send(SOCK_TCPS,DealReturnBuff,2);
		  }
		  if(gDATABUF[2]=='2')
		  {
			  if(temp_u8) SingleArcTH3++;
			  else SingleArcTH3--;
			  //SingleArcTH3 = temp_u8 + ArcTHBase;
			  AT24CXX_WriteData(ArcTH3_Addr,SingleArcTH3);
			  SCI_Printf("TH3=%d\n",SingleArcTH3);
			  send(SOCK_TCPS,DealReturnBuff,2);
		  }
	  }
	  //用于多次接收，如APP0010001 APP1010001同时发可以一次处理
	  i = 0;
	  APP_Deal:
	  	  ;
	  if((gDATABUF[i]=='A')&&(gDATABUF[i+1]=='P')&&(gDATABUF[i+2]=='P'))  //回传用电器信息
	  {
		  if(gDATABUF[i+3]=='0')
		  {
			 // HBCode1 =  将五位字符串编码复制到HBCODE内 LCD中处理即可
			  strncpy(HBCode1,gDATABUF+i+4,6); //截取类别至具体用电器
			  SCI_Printf("APP1\n");
		  }
		  if(gDATABUF[i+3]=='1')
		  {
			  strncpy(HBCode2,gDATABUF+i+4,6); //截取类别至具体用电器
			  SCI_Printf("APP2\n");
		  }
		  if(gDATABUF[i+3]=='2')
		  {
			  strncpy(HBCode3,gDATABUF+i+4,6); //截取类别至具体用电器
			  SCI_Printf("APP3\n");
		  }
		  WizRecLength = (WizRecLength>=10)?(WizRecLength = WizRecLength - 10):(WizRecLength);
		  if(!(WizRecLength%10) && WizRecLength!=0) //有多个APP
		  {
			  i = i + 10;
			  goto APP_Deal;
		  }
	  }
}

*/
