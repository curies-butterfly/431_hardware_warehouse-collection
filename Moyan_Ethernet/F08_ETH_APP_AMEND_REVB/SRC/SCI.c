/*
 * SCI.c
 *
 *  Created on: 2020年8月10日
 *      Author: M_KHui
 */

#include "SCI.h"
#include <stdio.h>
#include "stdarg.h"
/*//printf 重定向
int fputc(int _c, register FILE *_fp)
{
        while (SciaRegs.SCICTL2.bit.TXEMPTY == 0);
                SciaRegs.SCITXBUF = _c;
        return _c;
}*/


// Test 1,SCIA  DLB, 8-bit word, baud rate 0x000F, default, 1 STOP bit, no parity
void scia_echoback_init()
{

	//SCI的工作模式和参数需要用户在后面的学习中，深入的了解一个寄存器底层相关的资料了，多看看芯片手册和寄存器的意思。
    //因为28335的寄存器太多了，所以在以后的学习过程中，就不会对寄存器进行详细的注释了。
 	SciaRegs.SCICCR.all =0x0007;   // 1 stop bit,  No loopback
                                   // No parity,8 char bits,
                                   // async mode, idle-line protocol
	SciaRegs.SCICTL1.all =0x0003;  // enable TX, RX, internal SCICLK,
                                   // Disable RX ERR, SLEEP, TXWAKE
	SciaRegs.SCICTL2.all =0x0003;
	SciaRegs.SCICTL2.bit.TXINTENA = 1;
	SciaRegs.SCICTL2.bit.RXBKINTENA =1;
	#if (CPU_FRQ_150MHZ)           //波特率的配置
	//计算方式 （37.5M/baud/8 ）-1= 寄存器值 （高8位 低8位分配下去）
//	      SciaRegs.SCIHBAUD    =0x0001;  // 9600 baud @LSPCLK = 37.5MHz.
//	      SciaRegs.SCILBAUD    =0x00E7;
	      SciaRegs.SCIHBAUD    =0x0000;  // 115200 baud @LSPCLK = 37.5MHz.
	      SciaRegs.SCILBAUD    =0x0027;
	#endif
	#if (CPU_FRQ_100MHZ)
      SciaRegs.SCIHBAUD    =0x0001;  // 9600 baud @LSPCLK = 20MHz.
      SciaRegs.SCILBAUD    =0x0044;
	#endif
	SciaRegs.SCICTL1.all =0x0023;  // Relinquish SCI from Reset
}

void scia_xmit(int a)//发送字节的函数
{
    while (SciaRegs.SCICTL2.bit.TXRDY == 0) {}
    /*    SciaRegs.SCITXBUF=a;*/


	SciaRegs.SCITXBUF=a;
/*
	while(ScibRegs.SCICTL2.bit.TXRDY!=1);       //写a进SCI TXBUF,当TXRDY=0时，发送寄存器满
	    while(SciaRegs.SCICTL2.bit.TXEMPTY!=1);     //当发送寄存器和移位寄存器都被装入数据，准备发送数据
*/

}

void scia_msg(char * msg,Uint16 N)//发送数组的函数
{
/*    int i;
    i = 0;
    while(msg[i] != '\0')
    {
        scia_xmit(msg[i]);
        i++;
    }*/

	Uint16 i;
    for(i=0;i<N;i++)
    {
      scia_xmit(msg[i]);
    }
}
void scia_msg1(char * msg)//发送数组的函数
{
    int i;
    i = 0;
    while(msg[i] != '\0')
    {
        scia_xmit(msg[i]);
        i++;
    }
}
void SCI_Printf(char *fmt,...)//这个是我们的printf函数
{
	char * ap;
	char string[128];
	va_start(ap,fmt);
	vsprintf(string,fmt,ap);
	scia_msg1(string);
	va_end(ap);


}
/**test Code*/
/*						Cnt = 12345;
		SCI_Printf("iamyou%d %f \n",(unsigned int)Cnt,14.021);
		Cnt = 32767;
		SCI_Printf("iamyou%d %d \n",(int)Cnt,14);
		Cnt = 40000;
		SCI_Printf("iamyou%d %d \n",(unsigned int)Cnt,14);


		*/
