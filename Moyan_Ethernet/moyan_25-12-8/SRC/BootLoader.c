/*
 * BootLoader.c
 *
 *  Created on: 2021年7月
 *      Author: ZLL
 */
#include "BootLoader.h"
#include "I2cEeprom.h"
#include "device.h"
#include "SCI.h"
typedef  void (*pFunction1)(void);

u8 update_flag=0;
u8 updata_IORP = 0; //更新主动或被动标志位  0：主动更新 1；被动更新
void __disable_irq(void)
{
	DINT;
	DRTM;
}
void __enable_irq(void)
{
	EINT;
	ERTM;
}
void __set_PRIMASK(u8 state)
{
	if(state == 1)
	{
		__disable_irq();
	}
	else if(state == 0)
	{
		__enable_irq();
	}
	else
	{
		return;
	}
}

void BOOT_JumpToApplication(uint32_t Addr)
{
	//  asm(" LB  0x310000");
   // (*((void(*)(void))(Addr)))();
	//(*(pFunction)(Addr))();
	pFunction1 jump;
	jump = (pFunction1)(Addr);
	jump();

}
/*******************************************************************************
* 描述    : 清除进入APP次数
* 输入    :
* 返回值  : 无
*******************************************************************************/
void ClrAPP_Cnt(void)
{
    uint16_t temp=0;
    temp = AT24CXX_ReadData(CntAPP2Addr);
    if(temp > 80)
    	AT24CXX_WriteData(CntAPP2Addr,0);
}
void jumptoBOOT(void) //跳转到BOOT
{
	if((*((uint32_t *)BOOT_START_ADDR)!=0xFFFFFFFF))
	{
		AT24CXX_WriteData(BL_Status_ADDRESS,BL_Passive);
		SoftwareReset();
		/*__set_PRIMASK(1);
		SCI_Printf("jump to BOOT confirm \r\n");
		BOOT_JumpToApplication(BOOT_START_ADDR);
		SCI_Printf("Failed\r\n");*/
	}
	else
	{
		SCI_Printf("出厂固件BOOT程序错误...\r\n");
	}
}
