#ifndef _EEPROM_H_
#define _EEPROM_H_

#define Relay1_addr		1
#define Relay2_addr		2
#define Relay3_addr		3
#define Correction_value_Addr1 4
#define Correction_value_Addr2 5
#define Correction_value_Addr3 6
#define REBOOT_TIMES_ADDRESS 7
#define isCLR_REBOOT_TIMES_ADDRESS 8
#define ZeroCV1_Addr 9
#define ZeroCV2_Addr 10
#define ZeroCV3_Addr 11
#define BL_Status_ADDRESS 12
#define CntAPP2Addr 13
#define IdentifyAddr1 14
#define IdentifyAddr2 15
#define IdentifyAddr3 16
#define IdentifyAddr4 17
#define BootVER_Addr1 18
#define BootVER_Addr2 19
#define ArcTH1_Addr 20
#define ArcTH2_Addr 21
#define ArcTH3_Addr 22

#define CONNECT_IP_H16 30
#define CONNECT_IP_H8  31
#define CONNECT_IP_L16 32
#define CONNECT_IP_L8  33
#define CONNECT_PORT_H8  34
#define CONNECT_PORT_L8  35


// #define CLUSTER_COUNT_ADDR         36
// #define CLUSTER_MARGIN_ADDR1       37
// #define CLUSTER_MARGIN_ADDR2       38
// #define CLUSTER_MERGE_THRESH_ADDR1 39
// #define CLUSTER_MERGE_THRESH_ADDR2 40
// #define CLUSTER_DATA_START_ADDR    41  // 聚类数据开始地址

// EEPROM存储地址定义
#define CLUSTER_COUNT_ADDR         36  // 存放聚类总数
#define LEARNING_MODE_ADDR         37  // 学习/检测状态标志位
#define CLUSTER_DATA_START_ADDR    38  // 聚类边界数据开始地址
/****************EEPROM地址数据定义*******************
 * addr  | comment
 *------------------------
 *  1    | 继电器1状态
 *  2    | 继电器2状态
 *  3    | 继电器3状态
 *  4    | 校准值1
 *  5    | 校准值2
 *  6    | 校准值3
 *  7    | reboottimes
 *  8    | 清零等待时间标志位
 *  9    |
 *  10   |
 *  11   |
 *  12   |
 *  13   |
 *  14   |
 *  15   |
 *  16   |
 *		 |
 *		 |
 *  255  | Check
 */
void AT24CXX_WriteData(Uint16 addr,Uint16 data);
Uint16 AT24CXX_ReadData(Uint16 addr);
void AT24CXX_Eerom_Gpio_Init(void);
unsigned char AT24CXX_Check(void);


#endif
