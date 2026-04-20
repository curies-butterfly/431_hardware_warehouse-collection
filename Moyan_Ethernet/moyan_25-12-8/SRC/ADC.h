/*
 * ADC.h
 *
 *  Created on: 2020Äê8ÔÂ10ÈÕ
 *      Author: M_KHui
 */

#ifndef ADC_H_
#define ADC_H_

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

#define ADC_CKPS   0x1   // ADC module clock = HSPCLK/1      = 25.5MHz/(1)   = 25.0 MHz
#define ADC_SHCLK  0x5 // S/H width in ADC module periods                  = 2 ADC cycle
#define AVG        1000  // Average sample limit
#define ZOFFSET    0x00  // Average Zero offset


void ADC_Setup(void);


#endif /* ADC_H_ */
