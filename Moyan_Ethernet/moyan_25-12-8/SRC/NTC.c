#include "NTC.h"
#include "DSP2833x_Device.h"     // DSP2833x 头文件
#include "DSP2833x_Examples.h"   // DSP2833x 例子相关头文件
#include "stdio.h"
#include "math.h"

#define NTC_ConvertedValue_1 ((AdcRegs.ADCRESULT7)>>4)
#define NTC_ConvertedValue_2 ((AdcRegs.ADCRESULT9)>>4)
#define NTC_ConvertedValue_3 ((AdcRegs.ADCRESULT11)>>4)

#define ERROR_CODE_overflow -100
#define ERROR_CODE_NoNTC -80

Uint16 NTC_ADC_ConvertedValue = 0;
float V1=0,V2=0,V3=0;
float ntc_tmp1=0,ntc_tmp2=0,ntc_tmp3=0;
float Rt=0;
float Rp=10000;
float NTC_T2=273.15+25;
float Bx=3950;
float Ka=273.15;

char ntcfun_1()
{
	NTC_ADC_ConvertedValue=NTC_ConvertedValue_1;
	if(NTC_ADC_ConvertedValue > 4000)   //未插入NTC
	{
		//printf("请插入NTC1\n");
		return ERROR_CODE_NoNTC;
	}
	V1=(float) NTC_ADC_ConvertedValue/4096*2.93; // 读取转换的AD值
	Rt=V1*10000/(2.93-V1);
	ntc_tmp1=1/(1/NTC_T2+log(Rt/Rp)/Bx)-Ka+0.5;
	if(ntc_tmp1<=-20 || ntc_tmp1>=125)
	{
		ntc_tmp1 = ERROR_CODE_overflow;
		printf("超出温度范围\n");
		return (char)ntc_tmp1;
	}
	return (char)ntc_tmp1;
}

char ntcfun_2()
{
	NTC_ADC_ConvertedValue=NTC_ConvertedValue_2;
	if(NTC_ADC_ConvertedValue > 4000)   //未插入NTC
	{
		//printf("请插入NTC2\n");
		return ERROR_CODE_NoNTC;
	}
	V2=(float) NTC_ADC_ConvertedValue/4096*2.93; // 读取转换的AD值
	Rt=V2*10000/(2.93-V2);
	ntc_tmp2=1/(1/NTC_T2+log(Rt/Rp)/Bx)-Ka+0.5;

	if(ntc_tmp2<=-20 || ntc_tmp2>=125)
	{
		ntc_tmp2 = ERROR_CODE_overflow;
		printf("超出温度范围\n");
		return (char)ntc_tmp2;
	}
	return (char)ntc_tmp2;
}

char ntcfun_3()
{
	NTC_ADC_ConvertedValue=NTC_ConvertedValue_3;
	if(NTC_ADC_ConvertedValue > 4000)   //未插入NTC
	{
		//printf("请插入NTC3\n");
		return ERROR_CODE_NoNTC;
	}
	V3=(float) NTC_ADC_ConvertedValue/4096*2.93; // 读取转换的AD值
	Rt=V3*10000/(2.93-V3);
	ntc_tmp3=1/(1/NTC_T2+log(Rt/Rp)/Bx)-Ka+0.5;
	if(ntc_tmp3<=-20 || ntc_tmp3>=125)
	{
		ntc_tmp3 = ERROR_CODE_overflow;
		printf("超出温度范围\n");
		return (char)ntc_tmp3;
	}
	return (char)ntc_tmp3;
}

