/*
 * BootLoader.h
 *
 *  Created on: 2017年4月19日
 *      Author: admin
 */

#ifndef BOOTLOADER_BOOTLOADER_H_
#define BOOTLOADER_BOOTLOADER_H_
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
typedef unsigned short int u16;
typedef unsigned long int u32;
#define u8 unsigned char
#include "stdint.h"
#define APP1_START_ADDR  ((uint32_t)0x310000)
#define APP2_START_ADDR  ((uint32_t)0x320000)
#define BOOT_START_ADDR  ((uint32_t)0x33FFF6)
#define BL_APP1      0x00
#define BL_APP2     0x01
#define BL_Passive     0x03
#define BL_Active     0xff
//#define FW_TYPE         CAN_BL_BOOT

extern u8 update_flag;
extern u8 updata_IORP; //更新主动或被动标志位  0：主动更新 1；被动更新

void __disable_irq(void);
void __enable_irq(void);
void __set_PRIMASK(u8 state);


void BOOT_JumpToApplication(uint32_t Addr);
void jumptoBOOT(void); //跳转到BOOT
void ClrAPP_Cnt(void);
#endif /* BOOTLOADER_BOOTLOADER_H_ */
