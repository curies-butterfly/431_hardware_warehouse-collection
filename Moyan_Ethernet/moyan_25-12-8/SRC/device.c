
#include "device.h"
#include "config.h"
#include "socket.h"
#include "ult.h"
#include "w5500.h"
#include "24c16.h"
#include <stdio.h> 
#include <string.h>
#include "dhcp.h"
#include "Data_Frame.h"
#include "DSP2833x_Device.h"     // DSP2833x 头文件
#include "DSP2833x_Examples.h"   // DSP2833x 例子相关头文件
#include "I2cEeprom.h"
#include "SCI.h"
CONFIG_MSG  ConfigMsg, RecvMsg;
uint8 mac[6]={0x30,0xdc,0,0,0,0};
uint8 txsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};		// 选择8个Socket每个Socket发送缓存的大小，在w5500.c的void sysinit()有设置过程
uint8 rxsize[MAX_SOCK_NUM] = {2,2,2,2,2,2,2,2};		// 选择8个Socket每个Socket接收缓存的大小，在w5500.c的void sysinit()有设置过程
/*Device ID*/
unsigned char DevID_H16=0xDF;        //终端号高16位
unsigned char DevID_H8 =0xFF;         //终端号高8位
unsigned char DevID_L16=0x01;       //终端号低16位
unsigned char DevID_L8 =0x6D;        //终端号低8位

unsigned char MACDevID_L16=0x00;       //终端号低16位
unsigned char MACDevID_L8 =0x1D;        //终端号低8位
Uint16 AppVersion = 0x0201; //版本x.2.1  x=0 BOOT x=1 APP1  x=2 app2
/*Connect IP & Port*/
//    uint8 server_ip[4]={192,168,1,102};
//	uint8 server_ip[4]={192,168,1,103};
//    uint8 server_ip[4]={192,168,1,188};
//uint8 server_ip[4]={47,118,52,48};
uint8 server_ip[4]={101,37,253,97};
// uint8 server_ip[4]={47,118,50,191};
//  uint16 server_port=10020;  //杭电端口
//uint16 server_port=10009;
uint16 server_port=10009;

void Read_BoardInfo(void)
{
	uint8 port_h8=0,port_l8=0;



	port_h8=AT24CXX_ReadData(CONNECT_PORT_H8)&0xff;
	port_l8=AT24CXX_ReadData(CONNECT_PORT_L8)&0xff;
	if( port_h8==255 && port_l8==255)
	{
		AT24CXX_WriteData(CONNECT_IP_H16,101);
		AT24CXX_WriteData(CONNECT_IP_H8,37);
		AT24CXX_WriteData(CONNECT_IP_L16,253);
		AT24CXX_WriteData(CONNECT_IP_L8,97);
		AT24CXX_WriteData(CONNECT_PORT_H8,(10009>>8)&0XFF);
		AT24CXX_WriteData(CONNECT_PORT_L8,10009&0XFF);

		port_h8=AT24CXX_ReadData(CONNECT_PORT_H8)&0xff;
		port_l8=AT24CXX_ReadData(CONNECT_PORT_L8)&0xff;

		AT24CXX_WriteData(IdentifyAddr1,DevID_H16);
		AT24CXX_WriteData(IdentifyAddr2,DevID_H8);
		AT24CXX_WriteData(IdentifyAddr3,DevID_L16);
		AT24CXX_WriteData(IdentifyAddr4,DevID_L8);

	}
	DevID_H16 = AT24CXX_ReadData(IdentifyAddr1);
	DevID_H8 = AT24CXX_ReadData(IdentifyAddr2);
	DevID_L16 = AT24CXX_ReadData(IdentifyAddr3);
	DevID_L8 = AT24CXX_ReadData(IdentifyAddr4);

	server_ip[0]=AT24CXX_ReadData(CONNECT_IP_H16);
	server_ip[1]=AT24CXX_ReadData(CONNECT_IP_H8);
	server_ip[2]=AT24CXX_ReadData(CONNECT_IP_L16);
	server_ip[3]=AT24CXX_ReadData(CONNECT_IP_L8);
//	server_port=AT24CXX_ReadData(CONNECT_PORT_H8)&0xff;

//	SCI_Printf("read port h=%d,l=%d\r\n",port_h8,port_l8);
	server_port=  port_h8*256 + port_l8;
//	SCI_Printf("read port=%d\r\n",server_port);

}
uint8 update_temp_connect_ip_port(uint8 *ip,uint16 port)
{
	uint8 i=0;
	for(i=0;i<4;i++)
	{
		server_ip[i]=ip[i]&&0xff;
	}
	server_port=port;
	SCI_Printf("updated ip and port");
	Show_BoardInfo();
}
uint8 update_connect_ip_port(uint8 *ip,uint16 port)
{
	uint8 i=0;
	for(i=0;i<4;i++)
	{
		server_ip[i]=ip[i]&0xff;
	}
	server_port=port;
	SCI_Printf("updated ip and port and write in eeprom\r\n");
	AT24CXX_WriteData(CONNECT_IP_H16,server_ip[0]);
	AT24CXX_WriteData(CONNECT_IP_H8,server_ip[1]);
	AT24CXX_WriteData(CONNECT_IP_L16,server_ip[2]);
	AT24CXX_WriteData(CONNECT_IP_L8,server_ip[3]);
	AT24CXX_WriteData(CONNECT_PORT_H8,(server_port>>8)&0XFF);
	AT24CXX_WriteData(CONNECT_PORT_L8,server_port&0XFF);
	SCI_Printf("write port h=%d,l=%d\r\n",(server_port>>8)&0XFF,server_port&0XFF);

	Show_BoardInfo();
}
void Write_TEMP_id(uint8 *new_id)
{
	DevID_H16 = new_id[0];
	DevID_H8  =  new_id[1];
	DevID_L16 = new_id[2];
	DevID_L8  = new_id[3];
}

void Write_id(uint8 *new_id)
{
	Uint16 temp[4] = {0,0,0,0};
	temp[0] = AT24CXX_ReadData(IdentifyAddr1);
	temp[1] = AT24CXX_ReadData(IdentifyAddr2);
	temp[2] = AT24CXX_ReadData(IdentifyAddr3);
	temp[3] = AT24CXX_ReadData(IdentifyAddr4);

	if((temp[0] == new_id[0])&&(temp[1] == new_id[1])&&(temp[2] == new_id[2])&&(temp[3] == new_id[3]))
	{
		SCI_Printf("ID未更改，无需写入\r\n");
	}
	else
	{
		SCI_Printf("写入ID\n");
		AT24CXX_WriteData(IdentifyAddr1,new_id[0]);
		AT24CXX_WriteData(IdentifyAddr2,new_id[1]);
		AT24CXX_WriteData(IdentifyAddr3,new_id[2]);
		AT24CXX_WriteData(IdentifyAddr4,new_id[3]);
	}
	Write_TEMP_id(new_id);
	Show_BoardInfo();

}
void Show_BoardInfo()
{
	SCI_Printf("APP:ConnectIP : %d.%d.%d.%d :%d\r\n", server_ip[0],server_ip[1],server_ip[2],server_ip[3],server_port);
	SCI_Printf("APP:BoardID : %02X.%02X.%02X.%02X\r\n", DevID_H16,DevID_H8,DevID_L16,DevID_L8);
}
void WAIT_STATE_Net()
{
	Blue_Led_ON;
	DELAY_US(200000);
	Blue_Led_OFF;
	DELAY_US(4000000);
}
void GPIO_Configuration(void)
{
  
}

void WatchDog_Init()
{
	EALLOW;
	SysCtrlRegs.SCSR = 0;
	EDIS;
	EALLOW;
	SysCtrlRegs.WDCR = 0x002f;
	EDIS;
}
void SoftwareReset()
{
	EALLOW;
	SysCtrlRegs.WDCR = 0x000f;
	EDIS;
}


void reboot(void)
{

}
void set_w5500_mac(void)
{
	mac[2] = MACDevID_L8;
	mac[3] = MACDevID_L16;
	mac[4] = DevID_L8;
	mac[5] = DevID_L16;
	memcpy(ConfigMsg.mac, mac, 6);
	setSHAR(ConfigMsg.mac);	/**/
	memcpy(DHCP_GET.mac, mac, 6);
}
void set_network(void)			// 配置初始化IP信息并打印，初始化8个Socket
{
  uint8 ip[4];
  setSHAR(ConfigMsg.mac);
  //printf("mac:");
  setSUBR(ConfigMsg.sub);
  setGAR(ConfigMsg.gw);
  setSIPR(ConfigMsg.lip);

  sysinit(txsize, rxsize); 											// 初始化8个socket
  setRTR(2000);														// 设置超时时间
  setRCR(3);														// 设置最大重新发送次数

  
  getSIPR (ip);
  printf("IP : %d.%d.%d.%d\r\n", ip[0],ip[1],ip[2],ip[3]);
  getSUBR(ip);
  printf("SN : %d.%d.%d.%d\r\n", ip[0],ip[1],ip[2],ip[3]);
  getGAR(ip);
  printf("GW : %d.%d.%d.%d\r\n", ip[0],ip[1],ip[2],ip[3]);
}

void set_default(void)									// 设置默认MAC、IP、GW、SUB、DNS
{  
  uint8 mac[6]={0x00,0x08,0xdc,0x11,0x11,0x12};
  uint8 lip[4]={192,168,1,150};
  uint8 sub[4]={255,255,255,0};
  uint8 gw[4]={192,168,1,1};
  uint8 dns[4]={8,8,8,8};
  memcpy(ConfigMsg.lip, lip, 4);
  //printf("lip: %d.%d.%d.%d\r\n",lip[0],lip[1],lip[2],lip[3]);
  memcpy(ConfigMsg.sub, sub, 4);
 // printf("sub: %d.%d.%d.%d\r\n",sub[0],sub[1],sub[2],sub[3]);
  memcpy(ConfigMsg.gw,  gw, 4);
  //printf("gw: %d.%d.%d.%d\r\n",gw[0],gw[1],gw[2],gw[3]);
  memcpy(ConfigMsg.mac, mac,6);

  memcpy(ConfigMsg.dns,dns,4);
  //printf("dns: %d.%d.%d.%d\r\n",dns[0],dns[1],dns[2],dns[3]);

  ConfigMsg.dhcp=0;
  ConfigMsg.debug=1;
  ConfigMsg.fw_len=0;
  
  ConfigMsg.state=NORMAL_STATE;
  ConfigMsg.sw_ver[0]=FW_VER_HIGH;
  ConfigMsg.sw_ver[1]=FW_VER_LOW;
}

void write_config_to_eeprom(void)
{
  uint8 data;
  uint16 i,j;
  uint16 dAddr=0;
  for (i = 0, j = 0; i < (uint8)(sizeof(ConfigMsg)-4);i++) 
  {
    data = *(uint8 *)(ConfigMsg.mac+j);
    at24c16_write(dAddr, data);
    dAddr += 1;
    j +=1;
  }
}
void get_config(void)
{
  uint8 tmp=0;
  uint16 i;
  for (i =0; i < CONFIG_MSG_LEN; i++) 
  {
    tmp=at24c16_read(i);
    *(ConfigMsg.mac+i) = tmp;
  }
  if((ConfigMsg.mac[0]==0xff)&&(ConfigMsg.mac[1]==0xff)&&(ConfigMsg.mac[2]==0xff)&&(ConfigMsg.mac[3]==0xff)&&(ConfigMsg.mac[4]==0xff)&&(ConfigMsg.mac[5]==0xff))
  {
    set_default();
  }
}


void NVIC_Configuration(void)
{
 						// only app, no boot included
}

