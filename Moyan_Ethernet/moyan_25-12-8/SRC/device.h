#ifndef _DEVICE_H_
#define _DEVICE_H_
#include "DSP2833x_Device.h"     // DSP2833x 头文件
#include "DSP2833x_Examples.h"   // DSP2833x 例子相关头文件
#include "types.h"
#define DEVICE_ID "W5500"
#define FW_VER_HIGH   	1
#define FW_VER_LOW    	0
#define outputPar 0  //参数输出SCI模式

#define III 1  //异常检测调试输出SCI

#define Power_printf 0  //变化功率输出模式
extern unsigned char DevID_H16;        //终端号高16位
extern unsigned char DevID_H8;         //终端号高8位
extern unsigned char DevID_L16;       //终端号低16位
extern unsigned char DevID_L8;        //终端号低8位
extern uint8 server_ip[4];
extern uint16 server_port;
extern Uint16 AppVersion; //版本x.0.1
/*#define DevID_H16	0x66        //终端号高16位
#define DevID_H8 	0x77         //终端号高8位
#define DevID_L16	0x77       //终端号低16位
#define DevID_L8 	0x54       //终端号低8位*/
//#define ConnectIP {192,168,0,103}
//#define ConnectIP {47,118,52,48};
//#define ConnectPort 10009
/* Private function prototypes -----------------------------------------------*/
#define gBIT0    (uint32)0x00000001
#define gBIT1    (uint32)0x00000002
#define gBIT2    (uint32)0x00000004
#define gBIT3    (uint32)0x00000008
#define gBIT4    (uint32)0x00000010
#define gBIT5    (uint32)0x00000020
#define gBIT6    (uint32)0x00000040
#define gBIT7    (uint32)0x00000080
#define gBIT8    (uint32)0x00000100
#define gBIT9    (uint32)0x00000200
#define gBIT10   (uint32)0x00000400
#define gBIT11   (uint32)0x00000800
#define gBIT12   (uint32)0x00001000
#define gBIT13   (uint32)0x00002000
#define gBIT14   (uint32)0x00004000
#define gBIT15   (uint32)0x00008000
#define gBIT16    (uint32)0x00010000
#define gBIT17    (uint32)0x00020000
#define gBIT18    (uint32)0x00040000
#define gBIT19    (uint32)0x00080000
#define gBIT20    (uint32)0x00100000
#define gBIT21    (uint32)0x00200000
#define gBIT22    (uint32)0x00400000
#define gBIT23    (uint32)0x00800000
#define gBIT24    (uint32)0x01000000
#define gBIT25    (uint32)0x02000000
#define gBIT26    (uint32)0x04000000
#define gBIT27    (uint32)0x08000000
#define gBIT28    (uint32)0x10000000
#define gBIT29    (uint32)0x20000000
#define gBIT30    (uint32)0x40000000
#define gBIT31    (uint32)0x80000000
void GPIO_Configuration(void);
void NVIC_Configuration(void);
typedef  void (*pFunction)(void);
void set_network(void);
void write_config_to_eeprom(void);
void set_default(void);
void Reset_W5500(void);

void reboot(void);
void get_config(void);


void Show_BoardInfo(void);
void WAIT_STATE_Net(void);	//LED快闪 流水闪烁
/*DHCP*/
void set_w5500_mac(void);
void SoftwareReset(void);
void WatchDog_Init(void);
void Read_BoardInfo(void);
#endif

