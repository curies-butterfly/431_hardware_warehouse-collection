/********************************************************************
* 文件名： 	IO口模拟I2C读写Eeprom程序
* 描述:  	执行该程序，读写Eeprom			 
* 版本号： 	V1.1
*		 	
***********************************************************************/
/********************************************************************
程序说明：1.调用Eerom_Gpio_Init函数，初始化与Eeprom相关的IO
		  2.调用 AT24CXX_WriteData(Uint16 addr,Uint16 data); //写Eeprom
            	 AT24CXX_ReadData(Uint16 addr);				 //读Eeprom
		  3.查看读取的内容与写入内容是否一致
********************************************************************/


#include "DSP2833x_Device.h"     // DSP2833x 头文件
#include "DSP2833x_Examples.h"   // DSP2833x 例子相关头文件
#include "I2cEeprom.h"

#define SDA_R   GpioDataRegs.GPBDAT.bit.GPIO32;     //SDA 读状态
#define SDA_W0  GpioDataRegs.GPBCLEAR.bit.GPIO32=1; //SDA 输出0 写状态
#define SDA_W1  GpioDataRegs.GPBSET.bit.GPIO32=1;   //SDA 输出1 写状态
#define SCL_0   GpioDataRegs.GPBCLEAR.bit.GPIO33=1; //SCL 输出0
#define SCL_1   GpioDataRegs.GPBSET.bit.GPIO33=1;   //SCL 输出1
#define delay1_UNIT	10								//宏定义延时时间常数
Uint16 eromrw_err;									//Eeprom读写错误指示
/******************************函数声明****************************/
void delay1(Uint16 time);
void initiic() ;
void begintrans();
void stoptrans();
void ack();
void bytein(Uint16 ch);
Uint16 byteout(void);

/******************************IO口初始化****************************/
void AT24CXX_Eerom_Gpio_Init(void)
{
	EALLOW;
    GpioCtrlRegs.GPBPUD.bit.GPIO32 = 0;	  	//上拉
    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 1;   	// 输出端口
    GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0;  	// IO口
    GpioCtrlRegs.GPBQSEL1.bit.GPIO32 = 3; 	// 不同步

    GpioCtrlRegs.GPBPUD.bit.GPIO33 = 0;	  	//上拉
    GpioCtrlRegs.GPBDIR.bit.GPIO33 = 1;   	// 输出端口
    GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 0;  	// IO口
    GpioCtrlRegs.GPBQSEL1.bit.GPIO33 = 3;   // 不同步
    EDIS;
}
void SDA_READ(void)
{
    EALLOW;
    GpioCtrlRegs.GPBDIR.bit.GPIO32=0;       //Input, SDA
    EDIS;
}

void SDA_WRITE(void)
{
    EALLOW;
    GpioCtrlRegs.GPBDIR.bit.GPIO32=1;       //Output. SDA
    EDIS;
}




///=========================GPIO SIMULATE I2c communication=====================*/
void delay1(Uint16 time) 					//延时函数
{
    for(; time>0 ; time--)
    {
        asm(" nop");
        asm(" nop");
        asm(" nop");
        asm(" nop");
		asm(" nop");
        asm(" nop");
        asm(" nop");
        asm(" nop");
    }
}


void begintrans(void)       				//发送START 信号
{
    SDA_W1;         						//SDA=1
    delay1(delay1_UNIT * 10);         		//延时
    SDA_WRITE();            				//SDA 方向为输出到EEPROM
    delay1(delay1_UNIT * 10);         		//延时
    SCL_1;          						//SCL=1
    delay1(delay1_UNIT * 10);         		//延时
    SDA_W0;         						//SDA=0
    delay1(delay1_UNIT * 10);        		//延时
}

void stoptrans(void)        				//发送STOP 信号
{
    SDA_WRITE();            				//SDA方向为输出到EEPROM
    delay1(delay1_UNIT * 10);        		//延时
    SDA_W0;         						//SDA=0
    delay1(delay1_UNIT * 10);         		//延时
    SCL_1;          						//SCL=1
    delay1(delay1_UNIT * 10);         		//延时
    SDA_W1;         						//SDA=1
    delay1(delay1_UNIT * 10);
}

void ack(void)              				//等待EEPROM 的ACK 信号
{
    Uint16 d;                               //变量定义
    Uint16  i;                              //变量定义
    SDA_READ();             				//SDA方向为从EEPROM 输入
    delay1(delay1_UNIT * 2);          		//延时
    SCL_1;          						//SCL=1
    delay1(delay1_UNIT * 2);         		//延时
    i = 0;              
    do
    {
        d = SDA_R;
        i++;
        delay1(delay1_UNIT);
    }
    while((d == 1) && (i <= 500));      	//等待EEPROM 输出低电平,4ms后退出循环

    if (i >= 499)
    {
        eromrw_err = 0xff;
    }
    
    i = 0;                                  //i=0
    SCL_0;          						//SCL=0
    delay1(delay1_UNIT * 2);          		//延时
}

void bytein(Uint16 ch)  					//向EEPROM 写入一个字节 
{
    Uint16 i;                               //变量定义
    SCL_0;          						//SCL=0
    delay1(delay1_UNIT * 2);				//延时
    SDA_WRITE();            				//SDA方向为输出到EEPROM
    delay1(delay1_UNIT);         			//延时
    for(i=8;i>0;i--)
    {	
        if ((ch & 0x80)== 0) 
    	{
            SDA_W0;     					//数据通过SDA 串行移入EEPROM
            delay1(delay1_UNIT);			//延时
    	}
        else 
    	{
            SDA_W1;
            delay1(delay1_UNIT);		    //延时
    	}
        SCL_1;      						//SCL=1 
        delay1(delay1_UNIT * 2);      		//延时
        ch <<= 1;
        SCL_0;      						//SCL=0 
        delay1(delay1_UNIT);      			//延时
    } 
    ack();
}

Uint16 byteout(void)        				//从EEPROM 输出一个字节
{
    unsigned char i;                        //变量定义
    Uint16 ch;                              //变量定义
    ch = 0;                                 //CH值初始化

    SDA_READ();             				//SDA 的方向为从EEPROM 输出
    delay1(delay1_UNIT * 2);         	    //延时
    for(i=8;i>0;i--)
    {
        ch <<= 1;
        SCL_1;      						//SCL=1
        delay1(delay1_UNIT);      			//延时
        ch |= SDA_R;    					//数据通过SDA 串行移出EEPROM
        delay1(delay1_UNIT);     			//延时
        SCL_0;      						//SCL=0
        delay1(delay1_UNIT * 2);      		//延时
    }
    return(ch);
}

void AT24CXX_WriteData(Uint16 addr,Uint16 data) 	//向EEPROM 指定地址写入一个字节的数据
{
    begintrans();							//开始
    bytein(0xA0 + ((addr & 0x0300) >> 7));  //写入写控制字0xA0
    bytein(addr);       					//写入指定地址
    bytein(data);      						//写入待写入EEPROM 的数据
    stoptrans();							//停止
    delay1(8000);
}



Uint16 AT24CXX_ReadData(Uint16 addr) 				//从EEPROM 指定地址读取一个字节的数据
{
    Uint16 c;                               //变量定义
    begintrans();       					//开始
    bytein(0xA0 + ((addr & 0x0300) >> 7));  //写入写控制字0xA0
    bytein(addr);       					//写入指定地址
    begintrans();       					//开始
    bytein(0xA1);       					//写入读控制字0xA1
    c = byteout();      					//读出EEPROM 输出的数据
    stoptrans();        					//停止
    delay1(2000);        					//延时
    return(c);
}


/*********************************
 * 函数名：AT24CXX_Check
 * 描述：检查芯片是否正常
 * 输入：无
 * 输出：1-异常，0-正常
 ********************************/
unsigned char AT24CXX_Check(void)
{
	unsigned char temp = 0;
	temp = AT24CXX_ReadData(255);
	if(temp == 0x36)return 0;
	else
	{
		AT24CXX_WriteData(255,0x36);
		temp = AT24CXX_ReadData(255);
		if(temp == 0x36)return 0;
	}
	return 1;
}


/*
 *
Test Code
Uint16 dat[8]={11,22,33,44,55,66,77,89};
Uint16 addr = 0;
Uint16 dat1[8]={0};


	if(!AT24CXX_Check())
				printf("success\n");
			else
				printf("fail\n");
	        for(addr = 0;addr<=0xf;addr++)
	    	{
	        	AT24CXX_WriteData(addr,dat[addr]);					//写Eeprom
	            delay(50000);
	            dat1[addr] = AT24CXX_ReadData(addr);			    //读Eeprom
	            delay(50000);
	    	}

void delay(Uint32 k)
{
   while(k--);
}
	   while(1);
*/



//===========================================================================
// No more.
//===========================================================================
