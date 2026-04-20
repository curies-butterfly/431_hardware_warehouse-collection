/*
 * SCI.h
 *
 *  Created on: 2020年8月10日
 *      Author: M_KHui
 */

#ifndef SCI_H_
#define SCI_H_

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

void scia_echoback_init(void);  //声明SCI-A工作方式和参数配置
void scia_xmit(int a);          //声明发送字节的函数
void scia_msg(char * msg,Uint16 N);//发送数组的函数
void SCI_Printf(char *fmt,...);//这个是我们的printf函数

#endif /* SCI_H_ */
