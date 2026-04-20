/*
 * Data_Frame.c
 *
 *  Created on: 2020年8月11日
 *      Author: M_KHui
 *      update on:2021.6 By ZLL
 */
#include "Data_Frame.h"
#include "FFT.h"
#include "SCI.h"
#include "Ethernet_Deal.h"
#include "socket.h"
#include "NTC.h"
#include "I2cEeprom.h"
#include "device.h"
#include "stdio.h"
#include "FFT.h"
/**************************************变量定义************************************************/
unsigned char s_ucTxBuf[1024];   /*待发送数据缓冲区*/
//const unsigned char DevID = 0;
//unsigned char SensorID=1;
//const char  SensorID=0x00;     //传感器初始ID
/*const char DevID_H16=0x77;        //终端号高16位
const char DevID_H8 =0x77;         //终端号高8位
const char DevID_L16=0x77;       //终端号低16位
const char DevID_L8 =0x77;        //终端号低8位*/
//Uint16 OnOff=0;		//继电器通断状态，总闸开关（1-继电器闭合，后级电路断开    0-继电器断开，后级电路闭合）
//8位	8位		8位		8位
//3路	2路		1路		通用状态
//每一路|| 7剩余电流 || 6功率过高 || 5过压     || 4电弧 |||| 3欠压 || 2霍尔环未接 || 1联网 || 0e2prom |
//			  √			  √			  √		   √		  √			√		 	√	      √
//765432	765432	765432	10
// 3路		 2路		 1路		通用
//前24每八位代表每一路的故障码，其中联网和EEPROM为通用，记录在低八位，上发时将前24位的2-7与低8位0-1拼接后上发
uint32 Err_Code = 0;  //故障码  0000（危险绿灯灭） 0000（故障绿灯闪）
/* 实际效果
 * 蓝灯长亮，网络通畅。快闪或者灭，正在联网。蓝灯慢闪，等待状态。
 * 绿灯长亮一切正常。绿灯闪烁有故障。
 * 红灯灭无故障或危险。红灯长亮绿灯闪烁有故障。红灯闪烁绿灯灭有电弧危险。*/
/*******************************************************************************************/
/*
要点提示:
1. float和unsigned long具有相同的数据结构长度
2. union据类型里的数据存放在相同的物理空间
*/
typedef union
{
    float fdata;
    unsigned long ldata;
}FloatLongType;

/*
将浮点数f转化为4个字节数据存放在byte[4]中
 N为偏移量
*/
void Float_to_Byte(float f,unsigned char byte[],char N)
{
    FloatLongType fl;
    fl.fdata=f;
    byte[N+0]=(unsigned char)fl.ldata;
    byte[N+1]=(unsigned char)(fl.ldata>>8);
    byte[N+2]=(unsigned char)(fl.ldata>>16);
    byte[N+3]=(unsigned char)(fl.ldata>>24);
}

/***********************************
 * 描述：发送电器识别数据包
 * 输入：传感器ID
 * 返回：无
 ************************************/
void LAN_SendTyp(char ID)
{
	Uint16 i;

	for(i=0;i<TYP_CNT;i++)
	{
		s_ucTxBuf[i] =0;  //清空缓冲区
	}

	s_ucTxBuf[0]=TYP_FLAG_H;     //包头  0xAA 0xBB
	s_ucTxBuf[1]=TYP_FLAG_L;
	s_ucTxBuf[2]=TYP_CNT;      //字节数
	s_ucTxBuf[3]=DevID_L8;    //终端号低8位
	s_ucTxBuf[4]=DevID_L16;    //终端号低16位
	s_ucTxBuf[5]=DevID_H8;    //终端号高8位
	s_ucTxBuf[6]=DevID_H16;    //终端号高16位
	s_ucTxBuf[7]=(char)ID; //环号

	 switch(ID)
	    {
	        case 0:
				s_ucTxBuf[8]=OnOff_1;            //总闸开关状态（1开0关）
				Float_to_Byte(P_total1,s_ucTxBuf,9);
				Float_to_Byte(P_change1,s_ucTxBuf,13);
				for(i=0;i<TypParCnt;i++)
				{
					Float_to_Byte(gTypPara1[i],s_ucTxBuf,17+i*4);
				}
				Float_to_Byte(PwrFactor1,s_ucTxBuf,137);
//				s_ucTxBuf[141] = (u8)(NumOfPacks1&0x00ff);
//				s_ucTxBuf[142] = (u8)(NumOfPacks1>>8);
				s_ucTxBuf[141] = (u8)(NumOfPacksLess1);
				s_ucTxBuf[142] = (u8)(NumOfPacksMore1);
	            break;

	        case 1:
	        	s_ucTxBuf[8]=OnOff_2;            //总闸开关状态（1开0关）
	            Float_to_Byte(P_total2,s_ucTxBuf,9);
	            Float_to_Byte(P_change2,s_ucTxBuf,13);
	            for(i=0;i<TypParCnt;i++)
				{
	            	Float_to_Byte(gTypPara2[i],s_ucTxBuf,17+i*4);
				}
				Float_to_Byte(PwrFactor2,s_ucTxBuf,137);
//				s_ucTxBuf[141] = (u8)(NumOfPacks2&0x00ff);
//				s_ucTxBuf[142] = (u8)(NumOfPacks2>>8);
				s_ucTxBuf[141] = (u8)(NumOfPacksLess2);
				s_ucTxBuf[142] = (u8)(NumOfPacksMore2);
	            break;

	        case 2:
				s_ucTxBuf[8]=OnOff_3;            //总闸开关状态（1开0关）
				Float_to_Byte(P_total3,s_ucTxBuf,9);
				Float_to_Byte(P_change3,s_ucTxBuf,13);
				for(i=0;i<TypParCnt;i++)
				{
					Float_to_Byte(gTypPara3[i],s_ucTxBuf,17+i*4);
				}
				Float_to_Byte(PwrFactor3,s_ucTxBuf,137);
//				s_ucTxBuf[141] = (u8)(NumOfPacks3&0x00ff);
//				s_ucTxBuf[142] = (u8)(NumOfPacks3>>8);
				s_ucTxBuf[141] = (u8)(NumOfPacksLess3);
				s_ucTxBuf[142] = (u8)(NumOfPacksMore3);
	            break;

	        default:
				s_ucTxBuf[8]=OnOff_1;            //总闸开关状态（1开0关）
				// 默认发送第一房间内数据
				Float_to_Byte(P_total1,s_ucTxBuf,9);
				Float_to_Byte(P_change1,s_ucTxBuf,13);
				for(i=0;i<TypParCnt;i++)
				{
					Float_to_Byte(gTypPara1[i],s_ucTxBuf,17+i*4);
				}
				Float_to_Byte(PwrFactor1,s_ucTxBuf,137);
//				s_ucTxBuf[141] = (u8)(NumOfPacks1&0x00ff);
//				s_ucTxBuf[142] = (u8)(NumOfPacks1>>8);
				s_ucTxBuf[141] = (u8)(NumOfPacksLess1);
				s_ucTxBuf[142] = (u8)(NumOfPacksMore1);
	            break;
	    }

/*	memcpy(s_ucTxBuf+6,&P_total1,4);  //总功率
	memcpy(s_ucTxBuf+10,&P_change1,4);  //变化功率

	for(i=0;i<15;i++)
	   {
		memcpy(s_ucTxBuf+14+i*4,&gTypPara1[i],4); //15个识别参数
	   }
	memcpy(s_ucTxBuf+74,&PwrFactor1,4);  //功率因子*/
	s_ucTxBuf[TYP_CNT-2]=END_TAG_H;
	s_ucTxBuf[TYP_CNT-1]=END_TAG_L;

	send(SOCK_TCPS,s_ucTxBuf,TYP_CNT);
	//scia_msg((char*)s_ucTxBuf,TYP_CNT); //SCI发送
}


/***********************************
 * 描述：发送电弧识别数据包
 * 输入：传感器ID
 * 返回：无
 ************************************/
void LAN_SendArc(char ID)
{
	//Uint16 i;

	s_ucTxBuf[0]=ARC_FLAG_H;     //包头  0xBB 0xAA
	s_ucTxBuf[1]=ARC_FLAG_L;
	s_ucTxBuf[2]=ARC_CNT;      //字节数
	s_ucTxBuf[3]=DevID_L8;    //终端号低8位
	s_ucTxBuf[4]=DevID_L16;    //终端号低16位
	s_ucTxBuf[5]=DevID_H8;    //终端号高8位
	s_ucTxBuf[6]=DevID_H16;    //终端号高16位
	s_ucTxBuf[7]=(char)ID;    //环号

	//SCI_Printf("\nA.%f %f %f",(float)P_total1,(float)P_total2,(float)P_total3);
	 switch(ID)
	    {
	        case 0:
//	            memcpy(s_ucTxBuf+6,&P_total1,4);  //总功率
//	            memcpy(s_ucTxBuf+10,&P_change1,4);  //单功率
	        	s_ucTxBuf[8]=OnOff_1;            //总闸开关状态（1开0关）
	        	Float_to_Byte(P_total1,s_ucTxBuf,9);//总功率
	        	Float_to_Byte(P_change1,s_ucTxBuf,13);//单功率
	            s_ucTxBuf[17]=ArcCnt1;
	            break;

	        case 1:
	        	s_ucTxBuf[8]=OnOff_2;            //总闸开关状态（1开0关）
	        	Float_to_Byte(P_total2,s_ucTxBuf,9);//总功率
	        	Float_to_Byte(P_change2,s_ucTxBuf,13);//单功率

	            s_ucTxBuf[17]=ArcCnt2;
	            break;
	        case 2:
	        	s_ucTxBuf[8]=OnOff_3;            //总闸开关状态（1开0关）
	        	Float_to_Byte(P_total3,s_ucTxBuf,9);//总功率
	        	Float_to_Byte(P_change3,s_ucTxBuf,13);//单功率
	            s_ucTxBuf[17]=ArcCnt3;
	            break;
	        default:
	            // 默认发送第一房间内数据
	        	s_ucTxBuf[8]=OnOff_1;
	        	Float_to_Byte(P_total1,s_ucTxBuf,9);//总功率
	        	Float_to_Byte(P_change1,s_ucTxBuf,13);//单功率
	            s_ucTxBuf[17]=ArcCnt1;
	            break;
	    }

	s_ucTxBuf[ARC_CNT-2]=END_TAG_H;
	s_ucTxBuf[ARC_CNT-1]=END_TAG_L;

	send(SOCK_TCPS,s_ucTxBuf,ARC_CNT);
	//scia_msg((char*)s_ucTxBuf,ARC_CNT); //SCI发送
}


/***********************************
 * 描述：发送环境心跳包
 * 输入：无
 * 返回：无
 ************************************/
void LAN_SendEnvHeartBeatPackage()
{
	//LeakgeI1 = ReturnLkgI(LeakgeI1);//将ADC采样值转化成实际电流值，随后上发 已转化注释
	//LeakgeI2 = ReturnLkgI(LeakgeI2);
	//LeakgeI3 = ReturnLkgI(LeakgeI3);
	s_ucTxBuf[0]=ENV_HRT_FLAG_H;     //起始位
	s_ucTxBuf[1]=ENV_HRT_FLAG_L;
	s_ucTxBuf[2]=ENV_CNT ;        	//长度位
	s_ucTxBuf[3]= DevID_L8;   		//终端号(低8)
	s_ucTxBuf[4]= DevID_L16;    	//终端号(低16)
	s_ucTxBuf[5]= DevID_H8;    		//终端号(高8)
	s_ucTxBuf[6]= DevID_H16;  		//终端号(高16)
	s_ucTxBuf[7]= ntcfun_1();		//温度1
	s_ucTxBuf[8]= ntcfun_2();		//温度2
	s_ucTxBuf[9]= ntcfun_3();		//温度3
	Float_to_Byte(P_total1,s_ucTxBuf,10);	//总功率1
	Float_to_Byte(P_total2,s_ucTxBuf,14);	//总功率2
	Float_to_Byte(P_total3,s_ucTxBuf,18);	//总功率3
	s_ucTxBuf[22] = (u8)(LeakgeI1&0x00ff);  //剩余电流1
	s_ucTxBuf[23] = (u8)(LeakgeI1>>8);
	s_ucTxBuf[24] = (u8)(LeakgeI2&0x00ff);  //剩余电流2
	s_ucTxBuf[25] =	(u8)(LeakgeI2>>8);
	s_ucTxBuf[26] = (u8)(LeakgeI3&0x00ff);  //剩余电流3
	s_ucTxBuf[27] = (u8)(LeakgeI3>>8);
	s_ucTxBuf[28] = OnOff_1;
	s_ucTxBuf[29] = OnOff_2;
	s_ucTxBuf[30] = OnOff_3;
	s_ucTxBuf[31] = (u8)(SingleArcTH1&0x00ff);//备用改电弧阈值
	s_ucTxBuf[32] = (u8)(SingleArcTH2&0x00ff);
	s_ucTxBuf[33] = (u8)(SingleArcTH3&0x00ff);
	s_ucTxBuf[34] = 0x00;
	s_ucTxBuf[ENV_CNT-2] = END_TAG_H;
	s_ucTxBuf[ENV_CNT-1] = END_TAG_L;

	send(SOCK_TCPS,s_ucTxBuf,ENV_CNT);
}
/**********************************
 * 描述：是否发送故障包
 * 输入：none
 * 返回：无
 ***********************************/
void isSendFaultPkg(void)
{
	if(Err_Code&(uint32)0x04040403)
	{
		if((Err_Code & (uint32)0x04000000) != 0) //第二路EEPROM或联网有问题
		{
			LAN_SendFaultPackage(2);
		}
		if((Err_Code & (uint32)0x00040000) != 0)
		{
			LAN_SendFaultPackage(1);
		}
		if((Err_Code & (uint32)0x00000403) != 0)
		{
			LAN_SendFaultPackage(0);
		}
	}

}
/**********************************
 * 描述：发送故障包
 * 输入：传感器ID
 * 返回：无
 ***********************************/
void LAN_SendFaultPackage(u8 ID)
{
	s_ucTxBuf[0]=FAULT_FLAG_H;     //起始位
	s_ucTxBuf[1]=FAULT_FLAG_L;
	s_ucTxBuf[2]=FAULT_CNT ;       //长度位
	s_ucTxBuf[3]= DevID_L8;   //终端号(低8)
	s_ucTxBuf[4]= DevID_L16;   //终端号(低16)
	s_ucTxBuf[5]= DevID_H8;   //终端号(高8)
	s_ucTxBuf[6]= DevID_H16;   //终端号(高16)
	s_ucTxBuf[7]= ID;//环号
	//LeakgeI1 = ReturnLkgI(LeakgeI1);
	//LeakgeI2 = ReturnLkgI(LeakgeI2);
	//LeakgeI3 = ReturnLkgI(LeakgeI3);
	switch(ID)
	{
		case 0:
			s_ucTxBuf[8]= ntcfun_1();				//温度1
			Float_to_Byte(P_total1,s_ucTxBuf,9);	//总功率1
			s_ucTxBuf[13] = (u8)(LeakgeI1&0x00ff);	//剩余电流1
			s_ucTxBuf[14] = (u8)(LeakgeI1>>8);
			s_ucTxBuf[15] = OnOff_1;
			s_ucTxBuf[16] = ErrDec(0);
			s_ucTxBuf[17] = 0x00;
			s_ucTxBuf[18] = 0x00;
			s_ucTxBuf[19] = 0x00;
			s_ucTxBuf[20] = 0x00;
			break;
		case 1:
			s_ucTxBuf[8]= ntcfun_2();				//温度2
			Float_to_Byte(P_total2,s_ucTxBuf,9);	//总功率2
			s_ucTxBuf[13] = (u8)(LeakgeI2&0x00ff);	//剩余电流2
			s_ucTxBuf[14] = (u8)(LeakgeI2>>8);
			s_ucTxBuf[15] = OnOff_2;
			s_ucTxBuf[16] = ErrDec(1);
			s_ucTxBuf[17] = 0x00;
			s_ucTxBuf[18] = 0x00;
			s_ucTxBuf[19] = 0x00;
			s_ucTxBuf[20] = 0x00;
			break;
		case 2:
			s_ucTxBuf[8]= ntcfun_3();				//温度1
			Float_to_Byte(P_total3,s_ucTxBuf,9);	//总功率1
			s_ucTxBuf[13] = (u8)(LeakgeI3&0x00ff);	//剩余电流1
			s_ucTxBuf[14] = (u8)(LeakgeI3>>8);
			s_ucTxBuf[15] = OnOff_3;
			s_ucTxBuf[16] = ErrDec(2);
			s_ucTxBuf[17] = 0x00;
			s_ucTxBuf[18] = 0x00;
			s_ucTxBuf[19] = 0x00;
			s_ucTxBuf[20] = 0x00;
			break;
		default:
			break;
	}
	s_ucTxBuf[FAULT_CNT -2]=END_TAG_H;
	s_ucTxBuf[FAULT_CNT -1]=END_TAG_L;
	send(SOCK_TCPS,s_ucTxBuf,FAULT_CNT);
}

/**********************************
 * 描述：检查异常以及状态恢复
 * 输入：无
 * 返回：无
 ***********************************/
void Self_Check(void)
{
	Uint16 temp = 0;
	int temp1 = 0;
	temp = AT24CXX_Check();  //检查EEPROM功能
	if(!temp)
		Err_Code = Err_Code&(uint32)0xFFFFFFFE;
	else
		Err_Code = Err_Code|gBIT0;
	//恢复上一次继电器状态
	temp = AT24CXX_ReadData(Relay1_addr);
	if(temp==1)
		Realy1_ON;
	temp = AT24CXX_ReadData(Relay2_addr);
	if(temp==1)
		Realy2_ON;
	temp = AT24CXX_ReadData(Relay3_addr);
	if(temp==1)
		Realy3_ON;
	//恢复校正值
	temp = AT24CXX_ReadData(Correction_value_Addr1);
	Correction_value_1 = temp;
	temp = AT24CXX_ReadData(Correction_value_Addr2);
	Correction_value_2 = temp;
	temp = AT24CXX_ReadData(Correction_value_Addr3);
	Correction_value_3 = temp;
	//恢复零点位置
	temp1 = AT24CXX_ReadData(ZeroCV1_Addr);
	ZeroCV1 = temp1;
	temp1 = AT24CXX_ReadData(ZeroCV2_Addr);
	ZeroCV2 = temp1;
	temp1 = AT24CXX_ReadData(ZeroCV3_Addr);
	ZeroCV3 = temp1;
	//恢复电弧阈值
	SingleArcTH1 = AT24CXX_ReadData(ArcTH1_Addr);
	if(SingleArcTH1==255||(!SingleArcTH1)) SingleArcTH1=ArcTHBase;
	SingleArcTH2 = AT24CXX_ReadData(ArcTH2_Addr);
	if(SingleArcTH2==255||(!SingleArcTH2)) SingleArcTH2=ArcTHBase;
	SingleArcTH3 = AT24CXX_ReadData(ArcTH3_Addr);
	if(SingleArcTH3==255||(!SingleArcTH3)) SingleArcTH3=ArcTHBase;
}
/**********************************
 * 描述：检测输入按钮值并存储校准值
 * 输入：1:写入 0：读取 2:清零
 * 返回：无
 ***********************************/
#define CRR_Min 35	//50->35 2021年10月12日
void CRR_Data_Process(char SEL)
{
	Uint16 temp = 0;
	if(SEL == 1)
	{
		//写入
		temp = (Uint16)(((P_total_new1_CRR>CRR_Min)?P_total_new1_CRR:CRR_Min)+0.5) + 1;
		AT24CXX_WriteData(Correction_value_Addr1,temp);
		temp = (Uint16)(((P_total_new2_CRR>CRR_Min)?P_total_new1_CRR:CRR_Min)+0.5) + 1;
		AT24CXX_WriteData(Correction_value_Addr2,temp);
		temp = (Uint16)(((P_total_new3_CRR>CRR_Min)?P_total_new1_CRR:CRR_Min)+0.5) + 1;
		AT24CXX_WriteData(Correction_value_Addr3,temp);
	}
	else if(SEL == 0)
	{
		temp = AT24CXX_ReadData(Correction_value_Addr1);
		Correction_value_1 = temp;
		temp = AT24CXX_ReadData(Correction_value_Addr2);
		Correction_value_2 = temp;
		temp = AT24CXX_ReadData(Correction_value_Addr3);
		Correction_value_3 = temp;
	}
	else if(SEL == 2)
	{
		temp = 0;
		AT24CXX_WriteData(Correction_value_Addr1,temp);
		AT24CXX_WriteData(Correction_value_Addr2,temp);
		AT24CXX_WriteData(Correction_value_Addr3,temp);
	}
}
/**********************************
 * 描述：发送握手包
 * 输入：无
 * 返回：无
 ***********************************/
void LAN_SendSYN(void)
{
	//u8 i;

	s_ucTxBuf[0]=SYN_FLAG_H;     //起始位  0xAA 0xDD
	s_ucTxBuf[1]=SYN_FLAG_L;
	s_ucTxBuf[2]=SYN_CNT;        //长度位
	s_ucTxBuf[3]= DevID_L8;   //终端号(低8)
	s_ucTxBuf[4]= DevID_L16;   //终端号(低16)
	s_ucTxBuf[5]= DevID_H8;   //终端号(高8)
	s_ucTxBuf[6]= DevID_H16;   //终端号(高16)
	s_ucTxBuf[7]=END_TAG_H;
	s_ucTxBuf[8]=END_TAG_L;
//	for(i=0; i<SYN_CNT; i++)
//	{
//		printf("%d ",s_ucTxBuf[i]);
//	}
//	printf("\r\n");
	send(SOCK_TCPS,s_ucTxBuf,SYN_CNT);
}
/**********************************
 * 描述：发送信号质量回执
 * 输入：无
 * 返回：无
 ***********************************/
void LAN_RetSGN(void)
{
	//u8 i;

	s_ucTxBuf[0]=SGN_FLAG_H;     //起始位  0xCC 0x99
	s_ucTxBuf[1]=SGN_FLAG_L;
	s_ucTxBuf[2]=SGN_CNT;        //长度位
	s_ucTxBuf[3]= DevID_L8;   //终端号(低8)
	s_ucTxBuf[4]= DevID_L16;   //终端号(低16)
	s_ucTxBuf[5]= DevID_H8;   //终端号(高8)
	s_ucTxBuf[6]= DevID_H16;   //终端号(高16)
	s_ucTxBuf[7] = SigTobuff[0];
	s_ucTxBuf[8] = SigTobuff[1];
	s_ucTxBuf[9] = SigTobuff[2];
	s_ucTxBuf[10] = SigTobuff[3];
	s_ucTxBuf[11]=END_TAG_H;
	s_ucTxBuf[12]=END_TAG_L;
	send(SOCK_TCPS,s_ucTxBuf,SGN_CNT);
}
/**********************************
 * 描述：发送版本回执
 * 输入：无
 * 返回：无
 ***********************************/
void LAN_RetVersion(void)
{
	u8 temp = 0;
	u8 ver[2];
	temp = AT24CXX_ReadData(BL_Status_ADDRESS);
	ver[0] = AT24CXX_ReadData(BootVER_Addr1);
	ver[1] = AT24CXX_ReadData(BootVER_Addr2);

	s_ucTxBuf[0]=VER_FLAG_H;     //起始位  0xE1 0xE1
	s_ucTxBuf[1]=VER_FLAG_L;
	s_ucTxBuf[2]=VER_CNT;        //长度位
	s_ucTxBuf[3]= DevID_L8;   //终端号(低8)
	s_ucTxBuf[4]= DevID_L16;   //终端号(低16)
	s_ucTxBuf[5]= DevID_H8;   //终端号(高8)
	s_ucTxBuf[6]= DevID_H16;   //终端号(高16)
	s_ucTxBuf[7] = ver[1];
	s_ucTxBuf[8] = ver[0];
	s_ucTxBuf[9] = 0x00;
	s_ucTxBuf[10] = (u8)(AppVersion&0x00ff);
	s_ucTxBuf[11] = (u8)((AppVersion>>8)&0x00ff);
	s_ucTxBuf[12] = temp + 1;
	s_ucTxBuf[13]=END_TAG_H;
	s_ucTxBuf[14]=END_TAG_L;
	send(SOCK_TCPS,s_ucTxBuf,VER_CNT);
}
/*//电器识别数据包
//测试用 挑选频点中最大30个点
void SCI_SendMaxTyp()
{
	Uint16 i;
	s_ucTxBuf[0]=0xAA;     //包头  0xAA 0xBB
	s_ucTxBuf[1]=0xcc;
	s_ucTxBuf[2]=170;      //字节数
	s_ucTxBuf[3]=DevID;    //终端号
	s_ucTxBuf[4]=0x1; //环号
	s_ucTxBuf[5]=OnOff;            //总闸开关状态（1开0关）

	        	Float_to_Byte(P_total2,s_ucTxBuf,6);
	            Float_to_Byte(P_change2,s_ucTxBuf,10);

	            for(i=0;i<TypParCnt;i++)
	               {
		        	Float_to_Byte(gTypParaMax[i],s_ucTxBuf,14+i*4);
	               }
	            for(i=0;i<TypParCnt;i++)
	               {
	            	s_ucTxBuf[i+134] = FreID[i];
	                }
	                Float_to_Byte(PwrFactor2,s_ucTxBuf,164);

	s_ucTxBuf[168]=END_TAG_H;
	s_ucTxBuf[169]=END_TAG_L;

	scia_msg((char*)s_ucTxBuf,170); //SCI发送
}*/

/*void SCI_SendHeart()
{
	s_ucTxBuf[0]=HRT_FLAG_H;     //ÆðÊ¼Î»  0xAA 0xEE
	s_ucTxBuf[1]=HRT_FLAG_L;
	s_ucTxBuf[2]=HRT_CNT;        //³¤¶ÈÎ»
	s_ucTxBuf[3]=DevID_L8;    //终端号低8位
	s_ucTxBuf[4]=DevID_L16;    //终端号低16位
	s_ucTxBuf[5]=DevID_H8;    //终端号高8位
	s_ucTxBuf[6]=DevID_H16;    //终端号高16位
	//s_ucTxBuf[7]=0x0; //ÖÕ¶ËºÅ(µÍ×Ö½Ú)
	//s_ucTxBuf[8]=END_TAG_H;
	//s_ucTxBuf[9]=END_TAG_L;
	Float_to_Byte(P_total1,s_ucTxBuf,7);//总功率1
    Float_to_Byte(P_change1,s_ucTxBuf,11);//单功率1
    Float_to_Byte(P_total2,s_ucTxBuf,15);//总功率2
    Float_to_Byte(P_change2,s_ucTxBuf,19);//单功率2
    Float_to_Byte(P_total3,s_ucTxBuf,23);//总功率3
    Float_to_Byte(P_change3,s_ucTxBuf,27);//单功率3
    Float_to_Byte(UA1,s_ucTxBuf,31);//电压1
    Float_to_Byte(UA2,s_ucTxBuf,35);//电压2
    Float_to_Byte(UA3,s_ucTxBuf,39);//电压3
	s_ucTxBuf[43]=END_TAG_H;
	s_ucTxBuf[44]=END_TAG_L;

	scia_msg((char*)s_ucTxBuf,HRT_CNT); //SCI发送
}*/

