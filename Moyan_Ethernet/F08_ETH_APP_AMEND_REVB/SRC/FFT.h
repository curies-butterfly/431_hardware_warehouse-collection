/*
 * FFT.h
 *
 *  Created on: 2020年8月11日
 *      Author: M_KHui
 */

#ifndef FFT_H_
#define FFT_H_


#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include  <math.h>
#define Power_printf

#ifdef Power_printf
#endif
extern char ArcTHBase;
//#define Current_Diff_ON 1
#define SampCnt 256 //采样点数
#define TypParCnt 30 //识别参数个数
#define WaveFlag 3// 防止功率变化波动（功率分包发送） 在功率变化后WaveFlag个周波  记录数据
#define Vout    1520//3000.00*2048/4096//1470  //采样信号直流偏置+20
#define ARC_SenCnt  21 //电弧灵敏度个数
#define ARC_10s 38
#define ARC_30s 110
#define Pwr_Mini  15 //最小功率触发值 2021年11月9日19:39:47  25-> 15
#define Diff_Samples_Num 5
#define Iproportion1 1.00
#define Iproportion2 1.00
#define Iproportion3 1.00


#define MinClassfiValueUpperTH 50 //15-30，30-50
#define MinClassfiValueLowerTH 30
#define Set_Factory_TH  70  //默认门槛功率，即不做限制最大功率
#define Set_Voltage_TH_Max	250//过压值 单位：V
#define Set_Voltage_TH_Min	198//欠压值
#define Ignore_detection_voltage_Up 500//上限限不检测电压
#define Ignore_detection_voltage_Down 100//下限不检测电压

void Freq_Analysis1();
void Freq_Analysis2();
void Freq_Analysis3();
void max30out(float * arry,float *MaxArry,char * ID);
Uint16 LAN_Deal(float *Arry,char CH);
extern float  I_Array1[SampCnt];//电流数组
extern float  U_Array1[SampCnt];//电压数组
extern float  Sample_Leakge_Current_1[SampCnt];//剩余电流数组
extern char   IsADC1; //ADC采集处理完成标志位

extern float  I_Array2[SampCnt];//电流数组
extern float  U_Array2[SampCnt];//电压数组
extern float  Sample_Leakge_Current_2[SampCnt];//剩余电流数组
extern char   IsADC2; //ADC采集处理完成标志位

extern float  I_Array3[SampCnt];//电流数组
extern float  U_Array3[SampCnt];//电压数组
extern float  Sample_Leakge_Current_3[SampCnt];//剩余电流数组
extern char   IsADC3; //ADC采集处理完成标志位

extern float  I_Array_Old1[SampCnt];//上一次电流数组
extern float  I_Array_Old2[SampCnt];//上一次电流数组
extern float  I_Array_Old3[SampCnt];//上一次电流数组
extern float  Diff_I_Buff[SampCnt];//差分电流数组

extern Uint16 LeakgeI1,LeakgeI2,LeakgeI3;   //剩余电流值
extern float P_total1,P_total2,P_total3;
extern float UA1,UA2,UA3;
extern float P_change1,P_change2,P_change3;
extern float P_change1_old,P_change2_old,P_change3_old;
extern float P_total_new1_CRR,P_total_new2_CRR,P_total_new3_CRR;
extern float PwrFactor1,PwrFactor2,PwrFactor3;
extern char  PhDiff1,PhDiff2,PhDiff3;
extern float gTypPara1[TypParCnt],gTypPara2[TypParCnt],gTypPara3[TypParCnt]; //识别参数
extern float gTypParaMax[TypParCnt];
extern char  FreID[TypParCnt];
extern Uint16 ArcCnt1,ArcCnt2,ArcCnt3;		   //电弧产生次数
extern Uint16 Arc_timeFlag1,Arc_timeFlag2,Arc_timeFlag3; //电弧计数标志
extern Uint16 Arc_Cnt_Base1,Arc_Cnt_Base2,Arc_Cnt_Base3; //arc计数值
/*电流微分*/
//extern float P_total_buff_1,P_total_buff_2,P_total_buff_3;    //总功率—buff
//extern float P_change_buff_1,P_change_buff_2,P_change_buff_3;    //单功率-buff
extern char Current_Diff_Start_1;//开始计算电流速率标志
extern char Current_Diff_Start_2;//开始计算电流速率标志
extern char Current_Diff_Start_3;//开始计算电流速率标志
extern float Power_Buff1[Diff_Samples_Num];//功率数组
extern float Power_Buff2[Diff_Samples_Num];//功率数组
extern float Power_Buff3[Diff_Samples_Num];//功率数组
extern int Power_Diff1,Power_Diff2,Power_Diff3; //电流变化率
extern Uint16 CurtDiffPeriod;  //unit：ms 电流采样单周期
extern Uint16 Diff_Cnt; //电流计数值
extern unsigned char NumOfPacksLess1,NumOfPacksLess2,NumOfPacksLess3;
extern unsigned char NumOfPacksMore1,NumOfPacksMore2,NumOfPacksMore3;
/*矫正值*/
extern Uint16 Correction_value_1,Correction_value_2,Correction_value_3;  //校正值
extern Uint16 ZeroCV1,ZeroCV2,ZeroCV3;  //零点位置
/*end*/
extern Uint16 SingleArcTH1,SingleArcTH2,SingleArcTH3;  //电弧单周期阈值数值
//LCD
extern char HBCode1[],HBCode2[],HBCode3[];//用电器编码
/***/
extern char RefreshPowerFlag1,RefreshPowerFlag2,RefreshPowerFlag3;
void LeakgeI_Cal(void);
int Cal_Current_dif(float *buf,char N);
Uint16 ReturnLkgI(Uint16 temp);
#endif /* FFT_H_ */
