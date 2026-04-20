/*
 * ADC.c
 *
 *  Created on: 2020年8月9日
 *      Author: M_KHui
 */

#include "ADC.h"



void ADC_Setup(void)
{

	// ADC的配置和参数设置
	  // ADC_SHCLK
	   AdcRegs.ADCTRL1.bit.ACQ_PS = ADC_SHCLK;  // 顺序采样模式
	                        //                     = 1/(3*40ns) =8.3MHz (for 150 MHz SYSCLKOUT)
						    //                     = 1/(3*80ns) =4.17MHz (for 100 MHz SYSCLKOUT)
						    // If Simultaneous mode enabled: Sample rate = 1/[(3+ACQ_PS)*ADC clock in ns]
	   AdcRegs.ADCTRL3.bit.ADCCLKPS = ADC_CKPS; //ADC工作在25Mhz下，不再分频
	   AdcRegs.ADCTRL1.bit.SEQ_CASC = 0x0;      // 设置双通道模式
	  // AdcRegs.ADCTRL1.bit.SEQ_CASC = 0x1;      // 设置级联模式
	   AdcRegs.ADCTRL3.bit.SMODE_SEL = 1;       // 设置同步采样模式
	   AdcRegs.ADCTRL1.bit.CONT_RUN = 0;        // 设置非连续采样模式
	   //AdcRegs.ADCTRL1.bit.CONT_RUN = 1;        // 设置连续采样模式
	   AdcRegs.ADCTRL1.bit.SEQ_OVRD = 1;        // 使能排序覆盖
	   AdcRegs.ADCMAXCONV.bit.MAX_CONV1 = 0x7;  // 最大采集通道数为8路

	   AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0x0;   // 使能A0通道进行采样
	   AdcRegs.ADCCHSELSEQ1.bit.CONV01 = 0x1;   // 使能A1通道进行采样
	   AdcRegs.ADCCHSELSEQ1.bit.CONV02 = 0x2;   // 使能A2通道进行采样
	   AdcRegs.ADCCHSELSEQ1.bit.CONV03 = 0x3;   // 使能A2通道进行采样

	   AdcRegs.ADCCHSELSEQ3.bit.CONV08 = 0x4;   // 使能B0通道进行采样
	   AdcRegs.ADCCHSELSEQ3.bit.CONV09 = 0x5;   // 使能B1通道进行采样
	   AdcRegs.ADCCHSELSEQ3.bit.CONV10 = 0x6;   // 使能B2通道进行采样
	   AdcRegs.ADCCHSELSEQ3.bit.CONV11 = 0x7;   // 使能B2通道进行采样


	   AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 = 0x1; //允许向CPU发出中断请求  使能SEQ1中断
	   AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 = 0x1; //使能PWMA SOCA触发

	   AdcRegs.ADCTRL2.bit.INT_ENA_SEQ2 = 0x1;     //打开SEQ2中断
	   AdcRegs.ADCTRL2.bit.EPWM_SOCB_SEQ2 = 0x1;   //使能 PWMB SOCB触发
}





