//
// Created by starry on 2024/8/21.
//

#include <string.h>
#include <stdio.h>
#include "stdarg.h"
#include "updateParameters.h"
#include "chry_ringbuffer.h"
#include "SCI.h"
#include "Ethernet_Deal.h"
//#include<windows.h>
#define BOARD_INSTRUCT_MSG(s)  s

//#define USING_IN_WINDOWS

#ifdef USING_IN_WINDOWS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

//#define DEBUG 1
//#define LOGD(fmt, ...) {if (DEBUG == 1 ) printf("[D][%s:%d] "fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);}
//#define LOGE(fmt, ...) {if (DEBUG == 1 ) printf("[E][%s:%d] "fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);}
//#define LOGI(fmt, ...) {if (DEBUG == 1 ) printf("[I][%s:%d] "fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);}
#endif

/* 打印调试信息 */
void LOGD(const char* format, ...)
{
	SCI_Printf("Debug => ");
	SCI_Printf((char*)format);
	SCI_Printf("\n");
}

/* 打印错误信息 */
void LOGE(const char* format, ...)
{
	SCI_Printf("Error => ");
	SCI_Printf((char*)format);
	SCI_Printf("\n");
}

/* 打印参数信息 */
void LOGI(const char* format, ...)
{
	SCI_Printf("Info  => ");
	SCI_Printf((char*)format);
	SCI_Printf("\n");
}



static uint8_t StrConv2Hex(uint8_t *str_in)
{
    uint8_t res = 0, tmp = 0;

    // Convert the Higher Half-Byte
    if (*str_in >= 'a' && *str_in <= 'f')
    {
        *str_in = *str_in + 'A' - 'a';
    }
    if (*str_in >= 'A' && *str_in <= 'F')
        tmp = *str_in - 'A' + 10;
    if (*str_in >= '0' && *str_in <= '9')
        tmp = *str_in - '0';
    res = tmp << 4;

    // Convert the Lower Half-Byte
    str_in++;
    if (*str_in >= 'a' && *str_in <= 'f')
    {
        *str_in = *str_in + 'A' - 'a';
    }
    if (*str_in >= 'A' && *str_in <= 'F')
        tmp = *str_in - 'A' + 10;
    if (*str_in >= '0' && *str_in <= '9')
        tmp = *str_in - '0';
    res += tmp;
    return res;
}



static void HexConv2Str(uint8_t hex_in, uint8_t *str_out)
{
    // Convert the Higher Half-Byte
    uint8_t tmp = (hex_in >> 4) & 0x0f;
    if (tmp >= 0x00 && tmp <= 0x09)
        *str_out = tmp + '0';
    if (tmp >= 0x0A && tmp <= 0x0F)
        *str_out = tmp + 'A' - 0x0A;

    // Convert the Lower Half-Byte
    str_out++;
    tmp = hex_in & 0x0f;
    if (tmp >= 0x00 && tmp <= 0x09)
        *str_out = tmp + '0';
    if (tmp >= 0x0A && tmp <= 0x0F)
        *str_out = tmp + 'A' - 0x0A;
}
extern void correcting_init_value();
extern uint8 update_temp_connect_ip_port(uint8 *ip,uint16 port);
extern uint8 update_connect_ip_port(uint8 *ip,uint16 port);
extern void Write_id(uint8 *new_id);
extern void Write_TEMP_id(uint8 *new_id);
_Board magicEyeBoard;
chry_ringbuffer_t SCIArb;
uint8_t SCIAmempool[128];
interrupt void sciaRxIsr(void)
{
    unsigned char Res;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9; // Writing 1 to clear flag,must be needed

    Res = SciaRegs.SCIRXBUF.all;

    chry_ringbuffer_write_byte(&SCIArb, Res);

}



magicEye_event_t board_delay_ms(uint32_t t){

	DELAY_US(t*1000);
    return 1;
}
magicEye_event_t board_input_keys(void){
	if(0== KEY1_IN )
	{
		return 1;
	}
	else if(0== KEY2_IN )
	{
		return 2;
	}


    return 0;
}

magicEye_event_t board_check_mode_settins(void){
	uint8_t ip[4]={47,118,52,48};
	uint16_t port=10009;
	uint8_t id[4]={0XDF,0XFF,0X00,0X1D};
    /*设置ID*/
	Write_TEMP_id(id);
    /*设置IP*/
	update_temp_connect_ip_port(ip,port);
    /*设置PORT*/

    return 0;
}
static void test_env(void)
{
	board_delay_ms(500);
	Realy1_ON
	board_delay_ms(500);
	Realy2_ON
	board_delay_ms(500);
	Realy3_ON
	board_delay_ms(500);
	Realy1_OFF
	board_delay_ms(500);
	Realy2_OFF
	board_delay_ms(500);
	Realy3_OFF

}

static uint8_t get_ip_port_achieve(char *str ,uint16_t *out)
{
    char * ptr_splice;
    uint16_t temp_value=0;
    uint16_t temp_ip_port[5];
    uint8_t ip_tick=0;
    ptr_splice = strtok(str,".");
    while (ptr_splice){
        LOGI(ptr_splice);
        temp_value=atoi(ptr_splice);
        LOGI("get ip:%d",temp_value);
        temp_ip_port[ip_tick]=temp_value;
        ptr_splice= strtok(NULL,".");
        ip_tick++;
    }
    if(5==ip_tick)
    {
//        LOGI("get all");
//        for(int i=0;i<ip_tick;i++)
//            printf("%d.",temp_ip_port[i]);
//        LOGI("get all end");
        memcpy(out,temp_ip_port,5*sizeof (uint16_t));
        return 1;
    }
    return 0;
}

magicEye_event_t board_save_paramets_handle(uint8_t index)
{
	uint8_t  i;
    char * ptr_splice;
//    char * index_spilce;
    char * target_buffer=magicEyeBoard.input_instruct_str;
    char temp_buffer[30];
    uint8_t write_in_id[4]={0,0,0,0};
    uint16_t temp_value=0;
    uint16_t temp_ip_port[5];
    uint8_t write_in_ip[4]={0,0,0,0};
    uint16_t write_in_port=0;
    ptr_splice = strchr(magicEyeBoard.input_instruct_str,':');
    if(NULL ==ptr_splice)
        return 0;
    memset(temp_buffer,0,30);
    strcpy((char *)temp_buffer,(char *)target_buffer+(ptr_splice -  target_buffer+1));
    LOGI("get input ");
    SCI_Printf(temp_buffer);
    LOGI("get input end");
    switch (index) {
        //dev id
    	case 0:
    		LOGI("UPDATE ID->");
//		   if(temp_buffer[0] !='d')
//			   break;
		   temp_value=StrConv2Hex(temp_buffer);
		   //写入
		   write_in_id[0]=temp_value;
		   LOGI("get id h16=%x\r\n",temp_value);
		   //写入
		   temp_value=StrConv2Hex(temp_buffer+2);
		   write_in_id[1]=temp_value;
		   LOGI("get id h8=%x\r\n",temp_value);
		   temp_value=StrConv2Hex(temp_buffer+4);
		   //写入
		   write_in_id[2]=temp_value;
		   LOGI("get id l16=%x\r\n",temp_value);
		   temp_value=StrConv2Hex(temp_buffer+6);
		   //写入
		   write_in_id[3]=temp_value;
		   LOGI("get id l8=%x\r\n",temp_value);
		   magicEyeBoard.input_str_length=0;
		   Write_id(write_in_id);
		   break;
        //app ip port
        case 1:
        	LOGI("UPDATE Ip port ->");
           if(get_ip_port_achieve(temp_buffer,temp_ip_port))
           {
               LOGI("get all");
                for(i=0;i<4;i++)
                {
                	SCI_Printf("%d.",temp_ip_port[i]);
                	write_in_ip[i]=temp_ip_port[i]&0xff;
                }
                SCI_Printf(":%d",temp_ip_port[4]);
                write_in_port=temp_ip_port[4];
                SCI_Printf("\n");
                LOGI("get all end");
                LOGI("updated app ip port");
                update_connect_ip_port(write_in_ip,write_in_port);
           }

            magicEyeBoard.input_str_length=0;
            break;
        //boot ip port
        case 2:
            if(get_ip_port_achieve(temp_buffer,temp_ip_port))
            {
                LOGI("get all");
                for(i=0;i<5;i++)
                	SCI_Printf("%d.",temp_ip_port[i]);
                SCI_Printf("\n");
                LOGI("get all end");
                LOGI("updated boot ip port");
            }

            magicEyeBoard.input_str_length=0;
            break;
        default:
            break;
    }
}


void amend_dev_Init(_Board *curBoard)
{
    curBoard->amend_APP_IP_PORT_instrcut=BOARD_INSTRUCT_MSG("APP-IP-PORT-UPDATE");
    curBoard->amend_BOOT_IP_PORT_instrcut=BOARD_INSTRUCT_MSG("BOOT-IP-PORT-UPDATE");
    curBoard->delay_ms=board_delay_ms;
    curBoard->inputBUttons=board_input_keys;
    curBoard->set_check_mode_paramets=board_check_mode_settins;
    curBoard->save_paramets_handle=board_save_paramets_handle;
    curBoard->amend_BOOT_devID_instruct= BOARD_INSTRUCT_MSG("DEV-ID-UPDATE");
    curBoard->running_check=0;
    curBoard->current_state=0;
    curBoard->input_str_length=0;
}
void input_str_test()
{
//    scanf("%s",magicEyeBoard.input_instruct_str);
//    sprintf(magicEyeBoard.input_instruct_str,"DEV-ID-UPDATE:dfff001d");
    sprintf(magicEyeBoard.input_instruct_str,"APP-IP-PORT-UPDATE:47.118.52.48.10009");
//    sprintf(magicEyeBoard.input_instruct_str,"BOOT-IP-PORT-UPDATE:47.118.52.48.10009");
    magicEyeBoard.input_str_length=6;
}



/** 
 * @brief 初始化串口环形缓冲和设备对象；
 *在 60 次循环中，每秒检查一次输入；
 *能处理两类事件：
    按键 → 进入校准模式或退出模式；
    串口指令 → 修改设备参数并保存；
 *一旦检测到 running_check==1 或 loop_ticks 结束，关闭中断并退出。
 */
void update_dev_settings_task()
{
    uint8_t loop_ticks=60;

    /**
     * 需要注意的点是，init 函数第三个参数是内存池的大小（字节为单位）
     * 也是ringbuffer的深度，必须为 2 的幂次！！！。
     * 例如 4、16、32、64、128、1024、8192、65536等
     */
    LOGI("APP-start update settings");
    if (0 == chry_ringbuffer_init(&SCIArb, SCIAmempool, 128))
    {
        SCI_Printf("SCIAmempool success\r\n");
    }
    else
    {
        SCI_Printf("SCIAmempool error\r\n");
    }
    amend_dev_Init(&magicEyeBoard);
//    input_str_test();
    while(loop_ticks--)
    {
    	magicEyeBoard.input_str_length = chry_ringbuffer_read(&SCIArb, magicEyeBoard.input_instruct_str, 50);
    	magicEyeBoard.input_instruct_str[magicEyeBoard.input_str_length]='\0';
        //magicEyeBoard.input_str_length=1;
        //magicEyeBoard.input_instruct_str=....
        magicEyeBoard.delay_ms(1000);
        SCI_Printf("running:%d\r\n",loop_ticks);
//    if(1==magicEyeBoard.inputBUttons)
//    {
//    	LOGI("get button pressed");
//    	loop_ticks=60;
//    }
//    else if(2==magicEyeBoard.inputBUttons)
//    {
//    	LOGI("get button pressed");
//        magicEyeBoard.running_check=1;
//    }
        if(1==board_input_keys())
        {
        	LOGI("get button pressed  1");
        	magicEyeBoard.set_check_mode_paramets;
        	loop_ticks=60;
        	correcting_init_value();
        	test_env();
        }
        else if(2==board_input_keys())
        {
        	LOGI("get button pressed  2");
            magicEyeBoard.running_check=1;
        }

    if(magicEyeBoard.running_check==1)
    {

        IER &= ~M_INT9;
        PieCtrlRegs.PIEIER9.bit.INTx1=0;
        LOGI("out cur mode ");
        return ;
    }

    if(magicEyeBoard.input_str_length <5)
        continue;

    if(NULL != strstr(magicEyeBoard.input_instruct_str,magicEyeBoard.amend_BOOT_devID_instruct))
    {
//        loop_ticks++;
        LOGI("GET ID AMEND");
        magicEyeBoard.save_paramets_handle(0);
    }
    else if(NULL != strstr(magicEyeBoard.input_instruct_str,magicEyeBoard.amend_APP_IP_PORT_instrcut))
    {
//        loop_ticks++;
    	LOGI("GET Ip port AMEND");
        magicEyeBoard.save_paramets_handle(1);
    }
    else if(NULL != strstr(magicEyeBoard.input_instruct_str,magicEyeBoard.amend_BOOT_IP_PORT_instrcut))
    {
//        loop_ticks++;
        magicEyeBoard.save_paramets_handle(2);
    }

    }

    IER &= ~M_INT9;
    PieCtrlRegs.PIEIER9.bit.INTx1=0;

}
