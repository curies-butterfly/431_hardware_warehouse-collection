/*
 * Data_Frame.h
 *
 *  Created on: 2020年8月11日
 *      Author: M_KHui
 */

#ifndef DATA_FRAME_H_
#define DATA_FRAME_H_

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include "Ethernet_Deal.h"
#include "device.h"
#define TYP_FLAG_H   	 0xAA  //电器识别帧头高八位
#define TYP_FLAG_L   	 0xBB  //电器识别帧头低八位
#define ARC_FLAG_H   	 0xBB  //电弧识别帧头高八位
#define ARC_FLAG_L   	 0xAA  //电弧识别帧头低八位
#define ENV_HRT_FLAG_H	 0xEA  //环境心跳包高八位
#define ENV_HRT_FLAG_L	 0xEA  //环境心跳包低八位
#define SYN_FLAG_H    0xAA	   //握手包头
#define SYN_FLAG_L    0xDD     //握手包头
#define SGN_FLAG_H    0xCC	   //SGN包头
#define SGN_FLAG_L    0x9A     //SGN包头
#define VER_FLAG_H    0xE1	   //Version包头
#define VER_FLAG_L    0xE2     //Version包头
#define FAULT_FLAG_H	 0xFA  //错误码包头
#define FAULT_FLAG_L	 0xFA  //错误码包头
#define TYP_CNT      	 143+2   //电器识别包字节数
#define ARC_CNT       	 20    //电弧识别包字节数
#define ENV_CNT      	 37    //环境心跳包字节数
#define FAULT_CNT     	 23    //故障包字节数
#define SYN_CNT        	 9     //握手包字节数
#define SGN_CNT        	 13     //SGN包字节数
#define VER_CNT        	 15     //Version包字节数
//#define Current_CNT   12 //剩余电流包包长度
//#define HRT_CNT       45    //心跳包字节数
#define END_TAG_H    	 0xEE  //帧尾
#define END_TAG_L    	 0xFF  //帧尾


/*
extern const unsigned char DevID;
extern unsigned char SensorID;
extern Uint16 OnOff;		//继电器通断状态，总闸开关（1-继电器闭合，后级电路断开    0-继电器断开，后级电路闭合）
*/

/*void SCI_SendTyp(char ID);
void SCI_SendArc(char ID);
void SCI_SendMaxTyp();
void SCI_SendHeart();*/
extern uint32 Err_Code;
void LAN_SendTyp(char ID);
void LAN_SendArc(char ID);
void LAN_SendEnvHeartBeatPackage(void);
void LAN_SendFaultPackage(u8 ID);
void LAN_SendSYN(void);
void Self_Check(void);
void CRR_Data_Process(char SEL);
void LAN_SendFaultPackage(u8 ID);
void isSendFaultPkg(void);
void LAN_RetSGN(void);
void LAN_RetVersion(void);
#endif /* DATA_FRAME_H_ */
