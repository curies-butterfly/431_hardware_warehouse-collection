//###########################################################################
// 释放日期: 2021.
//_2022-01-28Ethernet of Application
//以太网版本application
//###########################################################################
/*********************
 *
 * 20220319:三路小包改好
 * ********************/
#include "DSP2833x_Device.h"     // DSP2833x 头文件
#include "DSP2833x_Examples.h"   // DSP2833x 例子相关头文件
#include  <math.h>
#include "SCI.h"
#include "FFT.h"
#include "ADC.h"
#include "EPwmSetup.h"
#include "Data_Frame.h"
#include "main.h"
/**************************************宏定义************************************************/
//W5500头文件
#include "device.h"
#include "spi2.h"
#include "ult.h"
#include "socket.h"
#include "w5500.h"
#include "24c16.h"
#include "string.h"
#include "dhcp.h"

#include "Ethernet_Deal.h"
#include "I2cEeprom.h"
#include "BootLoader.h"
//LCD
#include "LCD.h"

#include "online_cluster.h"
#include "types.h"
/********************************************************************************************/
#define POST_SHIFT   0  // Shift results after the entire sample table is full
#define INLINE_SHIFT 1  // Shift results as the data is taken from the results regsiter
#define NO_SHIFT     0  // Do not shift the results

// ADC start parameters
#if (CPU_FRQ_150MHZ)     // Default - 150 MHz SYSCLKOUT
  #define ADC_MODCLK 0x3 // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 150/(2*3)   = 25.0 MHz
#endif
#if (CPU_FRQ_100MHZ)
  #define ADC_MODCLK 0x2 // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 100/(2*2)   = 25.0 MHz
#endif

//#define SampCnt 256

/**************************************变量定义************************************************/
Uint32 t1=0,t2=0,t3=0,t4=0,T1=0,T2=0,t5,t6,t7,t8,T3,T4,t9,t10,t11,t12,T5,T6;
//Uint16 SampleTable_I1[SampCnt];  //定义接收BUF的SIZE
//Uint16 SampleTable_U1[SampCnt];  //定义接收BUF的SIZE
//Uint16 SampleTable_I2[SampCnt];  //定义接收BUF的SIZE
//Uint16 SampleTable_U2[SampCnt];  //定义接收BUF的SIZE
//Uint16 SampleTable_I3[SampCnt];  //定义接收BUF的SIZE
//Uint16 SampleTable_U3[SampCnt];  //定义接收BUF的SIZE
//Uint16 Sample_Leakge_Current_1[SampCnt];
//Uint16 Sample_Leakge_Current_2[SampCnt];
//Uint16 Sample_Leakge_Current_3[SampCnt];
Uint16 Sample_Temp;  //采样临时变量
Uint16 array_index_I1;  //定义变量
Uint16 array_index_U1;  //定义变量
Uint16 array_index_I2;  //定义变量
Uint16 array_index_U2;  //定义变量
Uint16 array_index_I3;  //定义变量
Uint16 array_index_U3;  //定义变量
Uint16 array_index_LeakgeCurrent;
Uint16 TIM0_Cnt_Base = 0; //定时器0中断计数值
Uint16 _500ms_Cnt_Base = 0; //500ms计数值
Uint16 TIM1_Cnt = 0;		//计时器 1 计数值
char isBlink = 0;  //灯是否闪烁  0:不闪 1：绿灯闪   2：红灯闪
char IWD_Cnt = 0;   //看门狗复位计数值
char Handshake_FLG=0; //握手包发送标志位
Uint16 Current_Diff_Cnt_1=0; //电流微分计算计数因子
Uint16 Current_Diff_Cnt_2=0; //电流微分计算计数因子
Uint16 Current_Diff_Cnt_3=0; //电流微分计算计数因子
char Current_Diff_Flag_1 = 0; //电流微分计算计数标志
char Current_Diff_Flag_2 = 0; //电流微分计算计数标志
char Current_Diff_Flag_3 = 0; //电流微分计算计数标志
Uint16 ENV_Count_Base = 0; //环境包计数基础
u8 ENV_Upload_Flag = 0;//发送标志
Uint16 LCD_Count_Base = 0; //LCD刷新计数基础
u8 LCDRefreshFlag = 0;//LCD更新标志
u8 WaitFlag = 0; //30swait Flag
#define SocketMaxTryTimes 15
uint32 RST_Count_Base = 0; //RESET计数
u8 Reset_flag = 0;//RST标志

u8 learning_mode = 0; // 学习模式标志
unsigned char feed_num = 0;
//online_cluster_t oc;
uint16 ranges_min[6], ranges_max[6];
uint8 range_count;
/**************************************声明区**************************************************/
interrupt void ISRCap1(void);
interrupt void ISRCap2(void);
interrupt void ISRCap3(void);
interrupt void epwm_int(void);
interrupt void ISRTimer0(void);  //声明定时器TIME0中断
interrupt void ISRTimer1(void);

extern    void InitCapl(void);
void Del_ArcFlag(void);

void TriggerAlarm(void);
//extern    void EPwmSetup();
//extern    void Freq_Analysis();
/*extern    void scia_echoback_init();
extern    void scia_xmit(int a);          //声明发送字节的函数
extern    void scia_msg(char *msg);       //声明发送数组的函数
extern    void ADC_Setup(void);*/
/***************************************W5500*************************************************/
//extern uint8 txsize[];
//extern uint8 rxsize[];
uint8 buffer[64];
char NetworkCableState=0;//网线连接状态
char NetworkCableStateCopy=-1;//网线连接状态
uint16 WizRecLength=0;
/***************************************W5500************************************************/
/**************************************主程序************************************************/
Uint32 Cnt;
char LCD_Index = 0;  //LCD翻页（0第一页），联网的动态参数
void MemCopyUser(Uint16 *SourceAddr, Uint16* SourceEndAddr, Uint16* DestAddr)
{
    while(SourceAddr < SourceEndAddr)
    {
       *DestAddr++ = *SourceAddr++;
    }
    return;
}
extern interrupt void sciaRxIsr(void);
void main(void)
{
   Uint16 i;              //定义变量
   unsigned char app1StartBuff[] = "!!APP1_Start!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
   unsigned char app2StartBuff[] = "!!APP2_Start!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
   int wait_min = 0; //当前等待时间
   u8 isCLR_REBOOT_TIMES = 0; //是否清零等待时间标志
   u8 Socket_Retry_Times = 0; //socket失败次数
   uint8 REBOOT_TIMES = 0;	//重启的次数
   int reboot_time_array[10] = {1,3,5,10,15,20,30,60,120,240}; //超时数组 unit:min
/****************************************W5500***********************************************/
	uint16 local_port=6000;
/****************************************W5500************************************************/
// 初始化系统控制:
// 设置PLL, WatchDog, 使能外设时钟
// 下面这个函数可以从DSP2833x_SysCtrl.c文件中找到..
   InitSysCtrl();
  // Xintf总线IO初始化
     InitXintf16Gpio();

     // 初始化串口SCI-A的GPIO
        InitSciaGpio();

//EALLOW，EDIS是成对使用的，有些寄存器是受到保护的，不能任意写，
//EALLOW相当于去掉保护，对写保护的寄存器进行操作后 EDIS 是重新把这个寄存器保护起来的意思。
   EALLOW;
   SysCtrlRegs.HISPCP.all = ADC_MODCLK;	// ADC时钟的配置 HSPCLK = SYSCLKOUT/ADC_MODCLK
   EDIS;

// 清除所有中断初始化中断向量表:
// 禁止CPU全局中断
   DINT;

// 初始化PIE中断向量表，并使其指向中断服务子程序（ISR）
// 这些中断服务子程序被放在了DSP280x_DefaultIsr.c源文件中
// 这个函数放在了DSP2833x_PieVect.c源文件里面.
   InitPieCtrl();
   MemCopyUser(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
// 禁止CPU中断和清除所有CPU中断标志
   IER = 0x0000;
   IFR = 0x0000;

// PIE 向量表指针指向中断服务程(ISR)完成其初始化.
// 即使在程序里不需要使用中断功能，也要对 PIE 向量表进行初始化.
// 这样做是为了避免PIE引起的错误.
   InitPieVectTable();

//EALLOW，EDIS是成对使用的，有些寄存器是受到保护的，不能任意写，
//EALLOW相当于去掉保护，对写保护的寄存器进行操作后 EDIS 是重新把这个寄存器保护起来的意思。
   EALLOW;  // This is needed to write to EALLOW protected registers
   PieVectTable. ECAP1_INT = &ISRCap1;  // 将CAP1中断添加都中断向量表里
   PieVectTable. ECAP2_INT = &ISRCap2;  // 将CAP2中断添加都中断向量表里
   PieVectTable. ECAP3_INT = &ISRCap3;  // 将CAP2中断添加都中断向量表里
   PieVectTable. EPWM1_INT = &epwm_int;
   PieVectTable. TINT0     = &ISRTimer0;  //将定时器中断添加都中断向量表里
   PieVectTable. XINT13    = &ISRTimer1;

   // 串口中断
   PieVectTable.SCIRXINTA = &sciaRxIsr;

   EDIS;    // This is needed to disable write to EALLOW protected registers

	for (i=0; i<SampCnt; i++)               //For循环
	{
		I_Array_Old1[i] = 0;//上一次电流数组
		I_Array_Old2[i] = 0;//上一次电流数组
		I_Array_Old3[i] = 0;//上一次电流数组
		Diff_I_Buff[i] = 0;//差分电流数组
	}
// 初始化CAP的相关配置
   InitCapl();

// 初始化ADC
   InitAdc();

   //ADC配置
   ADC_Setup();
   //epwm设置
   EPwmSetup();

// 初始化SCI-A工作方式和参数配置
   scia_echoback_init();

   InitCpuTimers();   // 定时器初始化


//通过以下面程序就可以让定时器 0 每隔一段时间产生一次中断，这段时间的
//计算公式为： △T= Freq * Period /150000000（s）；（其中 150000000 是
//CPU 的时钟频率，即 150MHz 的 时钟频率）针对此实验，Frep 为 150，Period 为 1000000，那么△T=1s。
    ConfigCpuTimer(&CpuTimer0, 150, 100000);// 100ms
    ConfigCpuTimer(&CpuTimer1, 150, 1000);//((float)CurtDiffPeriod)*1000);//1ms
    StartCpuTimer0();  //开启定时器0
    StartCpuTimer1();  //开启定时器1

    IER |= M_INT1;    //使能第一组中断
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1; //使能总中断
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1; //使能第一组中断里的第七个中断--定时器中断

    IER |= M_INT4;  //使能第一组中断
    IER |= M_INT3;  //使能epwm中断
    IER |= M_INT13; //定时器1中断



    PieCtrlRegs.PIEIER3.bit.INTx1 = 1; //使能第三组中断里的第一个中断--epwm中断
    PieCtrlRegs.PIEIER4.bit.INTx1 = 1; //使能第四组中断里的第一个中断--CAP1中断
    PieCtrlRegs.PIEIER4.bit.INTx2 = 1; //使能第四组中断里的第二个中断--CAP2中断
    PieCtrlRegs.PIEIER4.bit.INTx3 = 1; //使能第四组中断里的第二个中断--CAP3中断

    EINT;   // 中断使能
    ERTM;   // 使能总实时中断

/*********************************EEPROM*************************************/
    AT24CXX_Eerom_Gpio_Init();
    REBOOT_TIMES = AT24CXX_ReadData(REBOOT_TIMES_ADDRESS); //read boot times
    i = AT24CXX_ReadData(BL_Status_ADDRESS);
    ClrAPP_Cnt();//clr app cnt
    Read_BoardInfo(); //读取ID
    if(i == BL_APP1)
    	scia_msg((char*)app1StartBuff,sizeof(app1StartBuff)); //复位时发送数据
    if(i == BL_APP2)
        scia_msg((char*)app2StartBuff,sizeof(app2StartBuff)); //复位时发送数据

    gpio_config();
	extern void update_dev_settings_task();
	if(0== KEY1_IN || 0== KEY2_IN)
	{
		DELAY_US(100000);
		if(0== KEY1_IN || 0== KEY2_IN)
		{
		    IER|=M_INT9;
		    PieCtrlRegs.PIEIER9.bit.INTx1=1;
		    update_dev_settings_task();
		}

	}

    /***************LCD*****/
#if LCD_Enable
    LCD_GPIO_Init();
	LCD_Initial();
	LCD_PageLogo();
	DELAY_US(1000000);
	Screen_clear(0,7);
	Title_LCD();
	LCD_Loading(i);
#endif
    /***********************************************W5500********************/
    //移植W5500的初始化函数
    InitSpiaGpio();
//    gpio_config();
    spi_init();		  // init SPI
	BEEP_ON;
	delay_loop(5);
	BEEP_OFF;
    Reset_W5500();
    //set_default();
    //set_network();
    set_w5500_mac();
    init_dhcp_client();
    /**********************************************W5500END*********************/
    close_socket(0);close_socket(1);//清理端口缓存
	DELAY_US(50000);
	Show_BoardInfo();
	Self_Check(); //自检及恢复

	/*SingleArcTH1 = ArcTHBase;
	SingleArcTH2 = ArcTHBase;
	SingleArcTH3 = ArcTHBase;

	AT24CXX_WriteData(ArcTH1_Addr,0xff);
	AT24CXX_WriteData(ArcTH2_Addr,0xff);
	AT24CXX_WriteData(ArcTH3_Addr,0xff);*/



	WatchDog_Init();  //看门狗初始化
	ServiceDog();		//喂狗

	// 初始化：合并阈值=30，最小聚类大小=3，边界扩展=20
	online_cluster_init(&oc, 100, 3, 20);

	printf("初始化完毕!\n");
	SCI_Printf("初始化完成\n");
	Cnt = 0;
	while(1)
	{

		Cnt++;
		IWD_Cnt = 0;  //喂狗
		/*DHCP过程*/
		do_dhcp();
		if(initScreenFlag) //定期初始化LCD，防止白屏
		{
			initScreenFlag = 0;
			TimingInitializationLCD();
		}
		if(Reset_flag)
		{
			SCI_Printf("NetWork超时重启\r\n");
			//W5500_RST();
			SoftwareReset();
			RST_Count_Base = 0;
			Reset_flag  = 0;
		}
		/*网线连接状态-处理*/
		NetworkCableState = getNetworkCableState();   //为1是有连接
		if((NetworkCableState != NetworkCableStateCopy)||(NetworkCableStateCopy==-1))
		{
			NetworkCableStateCopy = NetworkCableState;
			if(NetworkCableState)
			{
				printf("\r\n网线连接\r\n");
				SCI_Printf("网线已连接\n");
			}
			else
			{
				printf("\r\n网线断开\r\n");
				SCI_Printf("网线已断开\n");
				Blue_Led_OFF;//网络灯关闭
				Err_Code = Err_Code|gBIT1; //set net error
				DELAY_US(500000);
				//reset_w5500();											/*硬复位W5500*/
				//socket_buf_init(txsize, rxsize);		/*初始化8个Socket的发送接收缓存大小*/
				close_socket(0);close_socket(1);//清理端口缓存
			}
		}
		while(!NetworkCableState)
		{
			NetworkCableState = getNetworkCableState();
			DELAY_US(1000000);
			printf("Network cable lose\n");
			SCI_Printf("Network cable lose\n");
			IWD_Cnt = 0;  //喂狗
		}
		/*网线连接状态-处理结束*/

		/*TCPC的业务代码*/
		if(dhcp_state != STATE_DHCP_LEASED) //未完成DHCP
		{
			//DELAY_US(300000);
			if(dhcp_state == STATE_DHCP_REREQUEST)
				SCI_Printf("reDHCPing\n");
			else
			{
				DELAY_US(300000);
				SCI_Printf("firstDHCPing\n");
				updateLoading(LCD_Index%5);
			}
			continue ; //结束本次循环
		}
		switch(getSn_SR(0))
		{
			case SOCK_INIT:
					connect(0, server_ip,server_port);
			break;
			case SOCK_ESTABLISHED:
					if(getSn_IR(0) & Sn_IR_CON)
					{
						setSn_IR(0, Sn_IR_CON);
					}
					Blue_Led_ON;//网络灯开启
					/*RST_Count_Base = 0;//复位计数置零
					Reset_flag  = 0;//复位标志置零*/
					//2021年8月9日20:24:02 改为收不到SGNT复位
					Socket_Retry_Times = 0;
					if(isCLR_REBOOT_TIMES == 0)  //保证复位后只执行一次
					{
						if(AT24CXX_ReadData(isCLR_REBOOT_TIMES_ADDRESS) != 55) //读取是否为0x55 若不是，则需要清零等待时间
						{
							REBOOT_TIMES = 0;
							isCLR_REBOOT_TIMES = 55;
							AT24CXX_WriteData(REBOOT_TIMES_ADDRESS,REBOOT_TIMES);
							AT24CXX_WriteData(isCLR_REBOOT_TIMES_ADDRESS,isCLR_REBOOT_TIMES);
							printf("清零了等待时间\n");
							SCI_Printf("清零了等待时间\n");
						}
						else
						{
							printf("无需清零等待时间\n");
							SCI_Printf("无需清零等待时间\n");
							isCLR_REBOOT_TIMES = 1;
						}
					}
					Err_Code = Err_Code&(~gBIT1); //clear net fault
					/*****************************通用逻辑************************/
					if(uploadFlag)  //发送标志置位
					{
						//数据包上发，附带电流变化率
						if(Current_Diff_Flag_1)//*s
						{
							Current_Diff_Flag_1 = 0;
							if(NumOfPacksMore1 || NumOfPacksLess1)
							{
								NumOfPacksMore1 = NumOfPacksMore1|0xe0;
								LAN_SendTyp(0x00);
								SCI_Printf("1大 小%d %d\n",NumOfPacksMore1,NumOfPacksLess1);
							}
							NumOfPacksLess1 = 0;
							NumOfPacksMore1 = 0;
						}
						if(Current_Diff_Flag_2)//*s
						{
							Current_Diff_Flag_2 = 0;
							if(NumOfPacksMore2 || NumOfPacksLess2)
							{
								NumOfPacksMore2 = NumOfPacksMore2|0xe0;
								LAN_SendTyp(0x01);
								SCI_Printf("2大 小%d %d\n",NumOfPacksMore2,NumOfPacksLess2);
							}
							NumOfPacksLess2 = 0;
							NumOfPacksMore2 = 0;
						}
						if(Current_Diff_Flag_3)//*s
						{
							Current_Diff_Flag_3 = 0;
							if(NumOfPacksMore3 || NumOfPacksLess3)
							{
								NumOfPacksMore3 = NumOfPacksMore3|0xe0;
								LAN_SendTyp(0x02);
								SCI_Printf("3大 小%d %d\n",NumOfPacksMore3,NumOfPacksLess3);
							}
							NumOfPacksLess3 = 0;
							NumOfPacksMore3 = 0;
						}
						//频谱分析，(无)上发
						Del_ArcFlag();
						if(TIM1_Cnt==0)
							Freq_Analysis1();
						if(TIM1_Cnt==3)
							Freq_Analysis2();
						if(TIM1_Cnt==6)
							Freq_Analysis3();
						//LCD
						if(LCDRefreshFlag)
						{
							LCDRefreshFlag = 0;
							if(key1_push == 0)
							{
								LCD_Index++;
								if(LCD_Index == 100)
									LCD_Index-=60;
								LCD_Task(LCD_Index%4 + 1);
								//LCD_Task(1);
								//LCD_Index++;
							}
						}
						KEY_Scan();

						//环境包上发
						if(ENV_Upload_Flag)//30S
						{
							ENV_Upload_Flag = 0;
							//SCI_Printf("\nH.%f %f %f",(float)P_total1,(float)P_total2,(float)P_total3);
							LAN_SendEnvHeartBeatPackage();//send heartbeat pkg
							isSendFaultPkg(); //send fault pkg
						}
					}
					else	//发送握手包
					{
						if(Handshake_FLG) //3s发送一次握手包
						{
							LAN_SendSYN();
							Handshake_FLG = 0;
						}
					}
					/*************************************************************/
					WizRecLength=getSn_RX_RSR(0);
					if(WizRecLength>0)
					{
						//scia_msg("接收到数据\n",12);
						recv(0,buffer,WizRecLength);
						 if(pBuffLen+WizRecLength>MaxBuffSize)
						{
							 printf("缓冲区满");
							 pBuffLen = 0;
						}
						if(WizRecLength)//设置输入缓冲区
						{
						//拷贝数据入缓冲区
							memcpy(WizDealBuffer+pBuffLen,buffer,WizRecLength);
							pBuffLen = pBuffLen + WizRecLength;
						}
					}
					Deal_Recvnew();
			break;
			case SOCK_CLOSE_WAIT:
					close_socket(0);
			break;
			case SOCK_CLOSED:
					Blue_Led_OFF;//网络灯关闭
					Err_Code = Err_Code|gBIT1; //set net error
					socket(0,Sn_MR_TCP,local_port,Sn_MR_ND);
					printf("socket close\n");
					SCI_Printf("socket close\n");
					Socket_Retry_Times ++;
					/*if(Socket_Retry_Times>1)
						SoftwareReset(); //softReset*/
					if(Socket_Retry_Times>SocketMaxTryTimes) //连接失败5次
					{
						close_socket(0);close_socket(1);
						Socket_Retry_Times = 0;
						REBOOT_TIMES++;
						if(REBOOT_TIMES >= 10)
							REBOOT_TIMES = 10;
						AT24CXX_WriteData(REBOOT_TIMES_ADDRESS,REBOOT_TIMES);
						printf("进入等待模式\n");
						SCI_Printf("进入等待模式\n");
						SCI_Printf("需要等待%dMin\n",reboot_time_array[REBOOT_TIMES-1]);
						isCLR_REBOOT_TIMES = 66;
						AT24CXX_WriteData(isCLR_REBOOT_TIMES_ADDRESS,isCLR_REBOOT_TIMES);
						while(1)
						{
							if(WaitFlag)
							{
								WaitFlag = 0;
								wait_min ++;
								if(wait_min>=reboot_time_array[REBOOT_TIMES-1]*2)
								{
									SCI_Printf("等待完毕！！\n");
									break;
								}
								SCI_Printf("等待了%d分钟\n",wait_min/2);
								SCI_Printf("还需要等待%d分钟\n",reboot_time_array[REBOOT_TIMES-1]-wait_min/2);
							}
							/* 喂狗 */
							IWD_Cnt = 0;  //喂狗
							WAIT_STATE_Net();
						}
						SoftwareReset(); //softReset
					}
			break;
			default :
				;
		}
		/*TCPC的业务代码结束*/

		if (KEY1_IN == 0)
		{
			DELAY_US(200000);
			if (KEY1_IN == 0)
			{
				SCI_Printf("key1按下\n");
        		oc.learning_mode = !oc.learning_mode;
				AT24CXX_WriteData(LEARNING_MODE_ADDR, oc.learning_mode & 0x01);
				if(oc.learning_mode == 1)
				{	
					online_cluster_load_from_eeprom(&oc);//保证使用最新数据
					printf("进入学习模式\n");
					SCI_Printf("进入学习模式\n");
				}
				if(oc.learning_mode == 0)
				{	
					online_cluster_save_to_eeprom(&oc);
					online_cluster_load_from_eeprom(&oc);//避免放在每次检测前浪费空间
					SCI_Printf("退出学习，自动保存结果，进入检测\n");
				}
			}
		}

		if (KEY2_IN == 0)
		{ // 清除学习数据
			DELAY_US(200000);
			if (KEY2_IN == 0)
			{
				online_cluster_clear_eeprom();
				online_cluster_load_from_eeprom(&oc);
				printf("异常检测数据已清除，重新学习\n");
				SCI_Printf("异常检测数据已清除，重新学习\n");
			}
		}

		if(feed_num>=200)
		{
			feed_num = 0;
			online_cluster_save_to_eeprom(&oc);
			AT24CXX_WriteData(LEARNING_MODE_ADDR, oc.learning_mode & 0x00);
			online_cluster_load_from_eeprom(&oc);//保证自动切换检测后使用最新数据
			SCI_Printf("数量达到n自动检测\n");
		}
		
// 		if (learning_mode == 1 && P_change1 > 25)
// 		{
// 			feed_num++;
// 			online_cluster_update(&oc, P_total1);
// 			if(feed_num >= 50)
// 				{
// 				online_cluster_save_to_eeprom(&oc);
// 				feed_num = 0;
// 				}
// 		}
// 		if(learning_mode == 0 && (P_change1>25))
// 		{
// 			online_cluster_load_from_eeprom(&oc);
// //			printf("退出异常检测学习模式\n");
// //			SCI_Printf("退出异常检测学习模式开始加载数据\n");
// 			// 保存学习结果
// 			if(online_cluster_is_abnormal(&oc,P_total1)==1)
// 				TriggerAlarm();
// 		}


	 }
}

interrupt void ISRCap1(void)
{

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;//0x0001赋给12组中断ACKnowledge寄存器，对其全部清除，不接受其他中断
    ECap1Regs.ECCLR.all=0xFFFF;//写0XFFFF对CAP1中断清除寄存器进行清除操作
	t1= ECap1Regs.CAP1;        //赋值
	t2= ECap1Regs.CAP2;        //赋值
	t3= ECap1Regs.CAP3;        //赋值
  	t4= ECap1Regs.CAP4;        //赋值
    T1=t3-t1;T2=t4-t3;
   if(T1<=250000) //如果小于这个时间 则认为是干扰
    return;
    IsADC1=0x1;
    array_index_I1=0; //ADC值数组索引置0
    array_index_U1=0; //ADC值数组索引置0

}

interrupt void ISRCap2(void)
{

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;//0x0001赋给12组中断ACKnowledge寄存器，对其全部清除，不接受其他中断
    ECap2Regs.ECCLR.all=0xFFFF;//写0XFFFF对CAP2中断清除寄存器进行清除操作
    t5= ECap2Regs.CAP1;        //赋值
	t6= ECap2Regs.CAP2;        //赋值
	t7= ECap2Regs.CAP3;        //赋值
  	t8= ECap2Regs.CAP4;        //赋值
    T3=t7-t5;T4=t8-t7;
    if(T3<=250000) //如果小于这个时间 则认为是干扰
    return;
    IsADC2=0x1;
    array_index_I2=0; //ADC值数组索引置0
    array_index_U2=0; //ADC值数组索引置0

}

interrupt void ISRCap3(void)
{

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;//0x0001赋给12组中断ACKnowledge寄存器，对其全部清除，不接受其他中断
    ECap3Regs.ECCLR.all=0xFFFF;//写0XFFFF对CAP2中断清除寄存器进行清除操作
    t9= ECap3Regs.CAP1;        //赋值
	t10= ECap3Regs.CAP2;        //赋值
	t11= ECap3Regs.CAP3;        //赋值
  	t12= ECap3Regs.CAP4;        //赋值
    T5=t11-t9;T6=t12-t11;
    if(T5<=250000) //如果小于这个时间 则认为是干扰
    return;
    IsADC3=0x1;
    array_index_I3=0; //ADC值数组索引置0
    array_index_U3=0; //ADC值数组索引置0

}
#pragma CODE_SECTION(epwm_int,"ramfuncs");
interrupt void epwm_int(void)
{

	        while(AdcRegs.ADCST.bit.INT_SEQ1 == 0);                 //等待ADC的中断位为1
			AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;                     //清除排序器中断位

			/*********************电流数据处理********************/
			Sample_Temp = ( (AdcRegs.ADCRESULT0)>>4);
			I_Array1[array_index_I1++] = ((Sample_Temp* 2930.0f / 4096.0f)- Vout + ZeroCV1)*0.05;  // 暂未使用EEPROM读出值ZeroCV3
			if(array_index_I1>=SampCnt)
				 array_index_I1 = 0;

			Sample_Temp = ( (AdcRegs.ADCRESULT2)>>4);
			I_Array2[array_index_I2++] = ((Sample_Temp* 2930.0f / 4096.0f)- Vout + ZeroCV2)*0.05;  //开口式50A需要改为0.033333（根据量程决定）
			if(array_index_I2>=SampCnt)
			   array_index_I2 = 0;

			Sample_Temp = ( (AdcRegs.ADCRESULT4)>>4);
			I_Array3[array_index_I3++] = ((Sample_Temp* 2930.0f / 4096.0f)- Vout + ZeroCV3)*0.05;
			if(array_index_I3>=SampCnt)
			   array_index_I3 = 0;
			/****************剩余电流********************/
			Sample_Temp = ( (AdcRegs.ADCRESULT6)>>4);
			Sample_Leakge_Current_1[array_index_LeakgeCurrent] = (float)Sample_Temp*2930.0f/3000.0f;
			Sample_Temp = ( (AdcRegs.ADCRESULT8)>>4);
			Sample_Leakge_Current_2[array_index_LeakgeCurrent] = (float)Sample_Temp*2930.0f/3000.0f;
			Sample_Temp = ( (AdcRegs.ADCRESULT10)>>4);
			Sample_Leakge_Current_3[array_index_LeakgeCurrent++] = (float)Sample_Temp*2930.0f/3000.0f;
			if(array_index_LeakgeCurrent>=SampCnt)
				array_index_LeakgeCurrent = 0;

			while(AdcRegs.ADCST.bit.INT_SEQ2 == 0);                 //等待ADC的中断位为1
			AdcRegs.ADCST.bit.INT_SEQ2_CLR = 1;                     //清除排序器中断位

			/*********************电压数据处理********************/
			Sample_Temp = ( (AdcRegs.ADCRESULT1)>>4);
			U_Array1[array_index_U1++] = ((Sample_Temp*2.93/4096.0)- 1.50)*396.0;
			if(array_index_U1>=SampCnt)
			  {
				 array_index_U1 = 0;
				 IsADC1 = 0x2;//采集完成
			   }

			Sample_Temp = ( (AdcRegs.ADCRESULT3)>>4);
			U_Array2[array_index_U2++] = ((Sample_Temp*2.93/4096.0)- 1.50)*396.0;
			if(array_index_U2>=SampCnt)
			  {
				 array_index_U2 = 0;
				 IsADC2 = 0x2;//采集完成
			   }


			Sample_Temp = ( (AdcRegs.ADCRESULT5)>>4);
			U_Array3[array_index_U3++] = ((Sample_Temp*2.93/4096.0)- 1.50)*396.0;
			if(array_index_U3>=SampCnt)
			  {
				 array_index_U3 = 0;
				 IsADC3 = 0x2;//采集完成
			   }

	  PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
      EPwm1Regs.ETCLR.bit.INT=1;

}

Uint16 initScreenCnt = 0;
Uint16 initScreenFlag = 0;
interrupt void ISRTimer0(void)  // T = 100ms
{
   // Acknowledge this interrupt to receive more interrupts from group 1
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; //0x0001赋给12组中断ACKnowledge寄存器，对其全部清除，不接受其他中断
    CpuTimer0Regs.TCR.bit.TIF=1;  // 定时到了指定时间，标志位置位，清除标志
    CpuTimer0Regs.TCR.bit.TRB=1;  // 重载Timer0的定时数据

    initScreenCnt++;
    if(initScreenCnt > 1200) //刷新屏幕 标志
    {
    	initScreenCnt = 0;
    	initScreenFlag = 1;
    }

    if(IWD_Cnt <100)
    {
        ServiceDog();  //WD
        IWD_Cnt++;
    }
    TIM0_Cnt_Base++;
    if(TIM0_Cnt_Base>30)  //3s
    {
    	TIM0_Cnt_Base = 0;
    	Handshake_FLG = 1;
    	if((Err_Code&(uint32)0xFFFFFFFF) == 0)  //无故障无危险
    	{
    		Green_Led_ON;
    		Red_Led_OFF;
    		isBlink = 0;
    		//printf("safe\n");
    	}
    	if((Err_Code&(uint32)0xf0f0f000) != 0) //有危险
    	{
    		if((Err_Code&(uint32)0x10101000) != 0)   //arc danger是电弧危险时红灯闪烁，绿灯灭
    		{
    			Green_Led_OFF;
    			isBlink = 2;
    			//printf("arcdanger\n");
    		}
    		else  //other danger			//其他危险如功率过高时，红灯亮，绿灯闪烁
    		{
    			Green_Led_OFF;
				Red_Led_ON;
				isBlink = 0;
				//printf("otherdanger\n");
    		}
    	}
    	else if((Err_Code&(uint32)0x0f0f0fff) != 0) //仅有故障时
		{
    		Red_Led_ON;
			isBlink = 1;
			//printf("fault\n");
		}
    }
    _500ms_Cnt_Base++;
    if(_500ms_Cnt_Base>=5) //500ms
    {
        if((dhcp_state != STATE_DHCP_LEASED)&&(NetworkCableState))//未完成DHCP且网线连接
        {
        	Blue_LED_toggle;
#if LCD_Enable
        	//updateLoading(LCD_Index%5);
        	LCD_Index++;
#endif
        }
    	_500ms_Cnt_Base = 0;
    	if(isBlink == 1)
    	{
    		//Green闪烁
    		Green_Led_toggle;
    	}
    	else if(isBlink == 2)
    	{
    		//red led blink
    		Red_Led_toggle;
    	}
    }
    RST_Count_Base ++;
	if(RST_Count_Base >= 600*5)   //丢网5Min重启
	{
		RST_Count_Base = 0;
		Reset_flag = 1;
	}
    ENV_Count_Base ++;	//环境计数
    if(ENV_Count_Base >= 300)   //30s
    {
    	Err_Code = Err_Code&(~(uint32)0x10101000); //clear arc danger bit
    	ENV_Upload_Flag = 1;
    	WaitFlag = 1;
    	ENV_Count_Base = 0;
    	RefreshPowerFlag1 = 1;
    	RefreshPowerFlag2 = 1;
    	RefreshPowerFlag3 = 1;
    }
    //LCD计数
    LCD_Count_Base++;
    if(LCD_Count_Base>=40)
    {
    	LCD_Count_Base = 0;
    	LCDRefreshFlag = 1;
    }
	//Arc Handler
	if((Arc_timeFlag1==1) || (Arc_timeFlag1==2))   //ARC1
	    	Arc_Cnt_Base1++;
	if((Arc_timeFlag2==1) || (Arc_timeFlag2==2))   //ARC2
	    	Arc_Cnt_Base2++;
	if((Arc_timeFlag3==1) || (Arc_timeFlag3==2))   //ARC3
	    	Arc_Cnt_Base3++;
}
void Del_ArcFlag(void)
{
    if(Arc_Cnt_Base1>=100)//10s
    {
    	Arc_timeFlag1 = 2;
    	if(Arc_Cnt_Base1==300)
    	{
    		Arc_Cnt_Base1 = 0;
    		Arc_timeFlag1 = 3;
    		if(ArcCnt1<ARC_30s)
    			Arc_timeFlag1=0;
    		ArcCnt1 = 0;
    	}
    }
    if(Arc_Cnt_Base2>=100)//10s
    {
    	Arc_timeFlag2 = 2;
    	if(Arc_Cnt_Base2==300)
    	{
    		Arc_Cnt_Base2 = 0;
    		Arc_timeFlag2 = 3;
    		if(ArcCnt2<ARC_30s)
    			Arc_timeFlag2=0;
    		ArcCnt2 = 0;
    	}
    }
    if(Arc_Cnt_Base3>=100)//10s
    {
    	Arc_timeFlag3 = 2;
    	if(Arc_Cnt_Base3==300) //30s
    	{
    		Arc_Cnt_Base3 = 0;
    		Arc_timeFlag3 = 3;
    		if(ArcCnt3<ARC_30s)
    			Arc_timeFlag3=0;
    		ArcCnt3 = 0;
    	}
    }
}
interrupt void ISRTimer1(void)  //T = 1ms,使用
{
   // Acknowledge this interrupt to receive more interrupts from group 1
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; //0x0001赋给12组中断ACKnowledge寄存器，对其全部清除，不接受其他中断
    CpuTimer1Regs.TCR.bit.TIF=1; // 定时到了指定时间，标志位置位，清除标志
    CpuTimer1Regs.TCR.bit.TRB=1;  // 重载Timer1的定时数据

    if(Current_Diff_Start_1)
    {
    	Current_Diff_Cnt_1++;
    	if(Current_Diff_Cnt_1==10000) //500ms到  3.8  2022   10syici
    	{
    		Current_Diff_Flag_1 = 1; //发送标志置位
    		Current_Diff_Start_1 = 0;//清零计数标志
    		Current_Diff_Cnt_1 = 0;  //清零计数值
    	}
    }
    if(Current_Diff_Start_2)
	{
		Current_Diff_Cnt_2++;
		if(Current_Diff_Cnt_2==10000) //500ms到
		{
			Current_Diff_Flag_2 = 1; //发送标志
			Current_Diff_Start_2 = 0;
			Current_Diff_Cnt_2 = 0;
		}
	}
    if(Current_Diff_Start_3)
	{
		Current_Diff_Cnt_3++;
		if(Current_Diff_Cnt_3==10000) //500ms到
		{
			Current_Diff_Flag_3 = 1; //发送标志
			Current_Diff_Start_3 = 0;
			Current_Diff_Cnt_3 = 0;
		}
	}

	TIM1_Cnt++;
	if(TIM1_Cnt>9)
		TIM1_Cnt = 0;
    //DHCP handler
	DHCP_timer_handler();
}

void TriggerAlarm(void)
{
	BEEP_ON;		   // 蜂鸣器开启
	DELAY_US(1000000); // 延时1秒
	BEEP_OFF;
}
//===========================================================================
// No more.
//===========================================================================


