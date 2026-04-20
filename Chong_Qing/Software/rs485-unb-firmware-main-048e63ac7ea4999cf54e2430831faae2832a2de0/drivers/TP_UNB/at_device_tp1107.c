/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-12     pc       the first version
 */
#include "at_device_tp1107.h"
#include "board.h"
#include "drv_common.h"
#include "at_unb_config.h"
#define DBG_TAG "unb_tp1107"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 消息队列控制块 */
static struct rt_messagequeue mq_1;
static rt_uint8_t msg_pool_1[2048];

#if UNB_DOUBLE
static struct rt_messagequeue mq_2;
static rt_uint8_t msg_pool_2[2048];

#endif
/* 定时器的控制块 */
static rt_timer_t timer1;
static rt_timer_t timer2;

/**
 * @brief Convert the Data from Hex in String (2 Characters indicate 1 Hex) to real Hex.
 * @note This function HAS NO SAFETY CHECK, ensure the input is legal
 * @param data Pointer to First character
 * @return uint8_t a number in hex
 */
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

/**
 * @brief Convert the Data from real Hex to Hex in String (2 Characters indicate 1 Hex)
 * @note This function HAS NO SAFETY CHECK, ensure the input is legal
 * @param hex_in data to be converted
 * @param str_out pointer to string pointer which to store convert result.
 */
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
static void basic_hard_rst_operate(at_device_tp1107 *ins)
{
    rt_pin_write(ins->rst_pin, PIN_LOW);
    rt_thread_mdelay(1000);
    rt_pin_write(ins->rst_pin, PIN_HIGH);
    rt_thread_mdelay(1000);
}
static void basic_hard_wake_up_operate(at_device_tp1107 *ins, rt_base_t status)
{
    rt_pin_write(ins->wake_pin, status);
}

static void basic_io_init(at_device_tp1107 *ins)
{
    rt_pin_mode(ins->rst_pin, PIN_MODE_OUTPUT);
    rt_pin_mode(ins->wake_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ins->rst_pin, PIN_HIGH);
    rt_pin_write(ins->wake_pin, PIN_HIGH);
}
static const char GO_AT_MODE[] = "+++\r\n";
static const char DEF_DEFAUT[] = "AT+DEF\r\n";
static const char *COMMAND_LISTS[] = { "AT+VER?", "AT+EUI?", "AT+JOIN?", "AT+FREQ?", "AT+REBOOT" , "AT+JOIN"};
static const char *COMMAND_RESPS[] = { "TP1107", "FF", "joined", "UL", "NONE" };

at_device_tp1107 tp1107 = {
TP1107_DEVICE_NAME,
TP1107_CLIENT_NAME, 0,
TP1107_RST_PIN,
TP1107_WAKE_PIN,
TP1107_DEFAUT_ESN,
NULL,
NULL };
static void urc_func1(struct at_client *client, const char *data, rt_size_t size)
{
    RT_ASSERT(data);
    rt_kprintf("URC data : %.*s", size, data);
    LOG_I("tp1107 joined gateway");
    tp1107.net_joined = 1;
}
static void urc_func2(struct at_client *client, const char *data, rt_size_t size)
{
    RT_ASSERT(data);

    uint8_t hex_len = 0;
    uint8_t str[256] = { 0 };

    rt_kprintf("URC data : %.*s", size, data);
    sscanf(data, "+NNMI:%d,%s", &hex_len, str);
#if UNB_DEBUG

    LOG_I("tp1107 recived form gateway->%s",msg);
#else
#if UNB_DOUBLE
    // TODO 这里暂时屏蔽单TP1107来自URC的数据，后续的配置处理还需进一步考虑


    int result;
    uint8_t len = hex_len * 2;

 /*   result = rt_mq_send(&mq_1, &len, 1);
    if (result != RT_EOK)
    {
        rt_kprintf("unb1 rt_mq_send ERR\n");
    }
    else {
        LOG_I("unb1 len=%d",len);
    }
//    rt_kprintf("unb1: send message_len - %d\n", len);
//    rt_kprintf("str:%s\r\n", str);
//    str[len]='\0';
    result = rt_mq_send(&mq_1, &str, len);
    if (result != RT_EOK)
    {
        rt_kprintf("unb1 rt_mq_send ERR\n");
    }
    else{
        LOG_I("unb1 data mq send");
    }
*/

    uint8_t *pMsg;
    pMsg = rt_malloc(len + 2);
    if(pMsg == NULL)
    {
        LOG_W("Mem alloc Error in msg production.");
        return;
    }
    pMsg[0]=len;
    rt_memcpy(pMsg+1,str, len);
    result = rt_mq_send(&mq_1, &pMsg, sizeof(pMsg));
    if (result != RT_EOK)
    {
        rt_kprintf("unb1 rt_mq_send ERR\n");
    }
//    rt_kprintf("unb1: send message - %s\n", str);
#endif
#endif
}
static const struct at_urc tp1107_urc_table[] = { { "joined", "\r\n", urc_func1 }, { "+NNMI", "\r\n", urc_func2 }, };
#define UNB_URC_TABLE_SIZE sizeof(tp1107_urc_table) / sizeof(tp1107_urc_table[0])
static char send_temp_str[512] = { 0 };
int unb_data_send(at_device_tp1107 *ins, uint8_t *data, uint8_t len)
{

    char str_hex[256] = { 0 };
    if (len >= 100)
        return 1;
    for (int i = 0; i < len; i++)
    {
        HexConv2Str(*(data + i), str_hex + (i << 1));
    }
    rt_sprintf(send_temp_str, "AT+UNBSEND=%d,%s,1", len, str_hex);
//    LOG_I(send_temp_str);
    if (at_obj_exec_cmd(at_client_get(ins->client_name), ins->at_resp, send_temp_str) != RT_EOK)
    {
        LOG_I(" [%s] AT client send commands failed, response error or timeout !", ins->device_name);
        return -1;
    }
    else
    {
        LOG_D(" [%s] send successfull->%s", ins->ESN, send_temp_str);
        if (RT_NULL != at_resp_get_line_by_kw(ins->at_resp, "SENT"))
        {
            return 0;
        }
    }
    return 1;
}

static int unb_data_send_direct(at_device_tp1107 *ins, uint8_t *data, uint8_t len)
{
    char str[256] = { 0 };
    if (len >= 128)
        return 1;
    rt_sprintf(str, "AT+UNBSEND=%d,%s,1", len, data);
    if (at_obj_exec_cmd(at_client_get(ins->client_name), ins->at_resp, str) != RT_EOK)
    {
        LOG_I(" [%s] AT client send commands failed, response error or timeout !", ins->device_name);
        return -1;
    }
    else
    {
        if (RT_NULL != at_resp_get_line_by_kw(ins->at_resp, "SENT"))
        {
            LOG_D(" [%s] send successfull->%s", ins->ESN, str);
            LOG_I(" [%s] send successfull[%d]", ins->ESN,len);
            return 0;
        }
    }
    return 1;
}
static char hexData[256] = { 0 };  // 用来存储 HEX 数据的字符串
static int unb_data_send_pack(at_device_tp1107 *ins, uint8_t *data, uint8_t len)
{
    char str[256] = { 0 };
    if (len >= 128)
        return 1;
    // 将 HEX 数据填充到 str 中

    for (uint8_t i = 0; i < len; i++) {
        // 将每个字节转换为 2 位十六进制并附加到 hexData 中
        snprintf(hexData + i * 2, 3, "%02X", data[i]);
    }
    rt_sprintf(str, "AT+UNBSEND=%d,%s,1", len, hexData);
    if (at_obj_exec_cmd(at_client_get(ins->client_name), ins->at_resp, str) != RT_EOK)
    {
        LOG_I(" [%s] AT client send commands failed, response error or timeout !", ins->device_name);
        return -1;
    }
    else
    {
        LOG_D(" [%s] send successfull->%s", ins->ESN, str);
        if (RT_NULL != at_resp_get_line_by_kw(ins->at_resp, "SENT"))
        {
            return 0;
        }
    }
    return 1;
}



void DTU_UNB_DATA_SEND(uint8_t *data, uint8_t len)
{
//    unb_data_send(&tp1107,data,len);
//    unb_data_send_pack(&tp1107,data,len);
//    char str[256] = { 0 };
    if (len >= 128)
        return ;
    // 将 HEX 数据填充到 str 中
    for (uint8_t i = 0; i < len; i++) {
        // 将每个字节转换为 2 位十六进制并附加到 hexData 中
        snprintf(hexData + i * 2, 3, "%02X", data[i]);
    }



    int result;
    uint8_t nlen = len * 2;
/*    result = rt_mq_send(&mq_1, &nlen, 1);
    if (result != RT_EOK)
    {
        rt_kprintf("unb1 rt_mq_send ERR\n");
    }
    else {
        LOG_I("unb1 len=%d",len);
    }
//    rt_kprintf("unb1: send message_len - %d\n", len);
//    rt_kprintf("str:%s\r\n", str);
//    str[len]='\0';
    result = rt_mq_send(&mq_1, &hexData, nlen);
    if (result != RT_EOK)
    {
        rt_kprintf("unb1 rt_mq_send ERR\n");
    }
    else{
        LOG_I("unb1 data mq send");
    }*/
    uint8_t *pMsg;
    pMsg = rt_malloc(nlen + 2);
    if(pMsg == NULL)
    {
        LOG_W("Mem alloc Error in msg production.");
        return;
    }
    pMsg[0]=nlen;
    rt_memcpy(pMsg+1,hexData, nlen);
    result = rt_mq_send(&mq_1, &pMsg, sizeof(pMsg));
    if (result != RT_EOK)
    {
        rt_kprintf("unb1 rt_mq_send ERR\n");
    }
}
#if UNB_DOUBLE

at_device_tp1107 tp1107_2 = {
TP1107_2_DEVICE_NAME,
TP1107_2_CLIENT_NAME, 0,
TP1107_2_RST_PIN,
TP1107_2_WAKE_PIN,
TP1107_2_DEFAUT_ESN,
NULL,
NULL };
static void urc_func3(struct at_client *client, const char *data, rt_size_t size)
{
    RT_ASSERT(data);
    rt_kprintf("URC2 data : %.*s", size, data);
    LOG_I("tp1107_2 joined gateway");
    tp1107_2.net_joined = 1;
}
static void urc_func4(struct at_client *client, const char *data, rt_size_t size)
{
    RT_ASSERT(data);
    int hex_len = 0;
    uint8_t str[256] = { 0 };
    rt_kprintf("URC2 data : %.*s", size, data);
    sscanf(data, "+NNMI:%d,%s", &hex_len, str);
#if UNB_DEBUG
    char msg[128] =
    {   0};
    for (uint16_t i = 0; i < hex_len; i++)
    {
        msg[i] = StrConv2Hex(str + i * 2);
    }
    LOG_I("tp1107 recived form gateway->%s",msg);
#else
    int result;
    uint8_t len = hex_len * 2;
    /*
    result = rt_mq_send(&mq_2, &len, 1);
    if (result != RT_EOK)
    {
        rt_kprintf("unb2 rt_mq_send ERR\n");
    }
    rt_kprintf("unb2: send message_len - %d\n", len);
//    rt_kprintf("str:%s\r\n",str);
//    str[len]='\0';
    result = rt_mq_send(&mq_2, &str, len);
    if (result != RT_EOK)
    {
        rt_kprintf("unb2 rt_mq_send ERR\n");
    }
    */
    uint8_t *pMsg;
    pMsg = rt_malloc(len + 2);
    if(pMsg == NULL)
    {
        LOG_W("Mem alloc Error in msg production.");
        return;
    }
    pMsg[0]=len;
    rt_memcpy(pMsg+1,str, len);
    result = rt_mq_send(&mq_2, &pMsg, sizeof(pMsg));
    if (result != RT_EOK)
    {
        rt_kprintf("unb2 rt_mq_send ERR\n");
    }
    rt_kprintf("unb2: send message - %s\n", str);
#endif
}
static const struct at_urc tp1107_2_urc_table[] = { { "joined", "\r\n", urc_func3 }, { "+NNMI", "\r\n", urc_func4 }, };
#endif
int unb_tp1107_rea_init(at_device_tp1107 *unb_device, const struct at_urc urc_temp[])
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /* 初始化配置参数 */
    static rt_device_t device;
    char *str_ptr;
    uint8_t index = 0;
    uint8_t join_tick = 0;
    uint8_t err_tick=0;
    uint8_t once = 1;
//    unb_device=&tp1107_2;
    RESTART: basic_io_init(unb_device);
    basic_hard_wake_up_operate(unb_device, 1);
    basic_hard_rst_operate(unb_device);
    device = rt_device_find(unb_device->client_name);
    LOG_I(" [%s] init start", unb_device->device_name);
    if (!device)
    {
        LOG_E("find %s failed!\n", unb_device->client_name);
        return RT_ERROR;
    }
    config.baud_rate = BAUD_RATE_9600;        //修改波特率为 9600
    config.data_bits = DATA_BITS_8;           //数据位 8
    config.stop_bits = STOP_BITS_1;           //停止位 1
    config.bufsz = 128;                   //修改缓冲区 buff size 为 128
    config.parity = PARITY_NONE;           //无奇偶校验位
    rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_open(device, RT_DEVICE_FLAG_INT_RX);
    rt_device_write(device, 0, GO_AT_MODE, (sizeof(GO_AT_MODE) - 1));



    /* init at client for command mode */
    if (1 == once)
    {
        once = 0;
        at_client_init(unb_device->client_name, 255);
        at_obj_set_urc_table(at_client_get(unb_device->client_name), urc_temp, UNB_URC_TABLE_SIZE);
        unb_device->at_resp = at_create_resp(128, 2, rt_tick_from_millisecond(4000));
    }

//    at_obj_exec_cmd(at_client_get(unb_device->client_name),unb_device->at_resp, COMMAND_LISTS[0]);
    if (at_obj_exec_cmd(at_client_get(unb_device->client_name), unb_device->at_resp, COMMAND_LISTS[0]) != RT_EOK)
    {
        LOG_I("AT client send commands failed, response error or timeout !");
        rt_device_write(device, 0, GO_AT_MODE, (sizeof(GO_AT_MODE) - 1));
    }
    else
    {
        LOG_D("send successfull->%s", COMMAND_LISTS[0]);
        LOG_I("resp=%s", at_resp_get_line_by_kw(unb_device->at_resp, COMMAND_RESPS[0]));
    }

    if (at_obj_exec_cmd(at_client_get(unb_device->client_name), unb_device->at_resp, COMMAND_LISTS[1]) != RT_EOK)
    {
        LOG_I("AT client send commands failed, response error or timeout !");
        rt_device_write(device, 0, GO_AT_MODE, (sizeof(GO_AT_MODE) - 1));
    }
    else
    {
        LOG_D("send successfull->%s", COMMAND_LISTS[1]);
        str_ptr = at_resp_get_line_by_kw(unb_device->at_resp, (const char *) COMMAND_RESPS[1]);
        if (RT_NULL != str_ptr)
        {
            for (index = 0; index < 12; index++)
            {
                unb_device->ESN[index] = *str_ptr;
                str_ptr++;
            }
            unb_device->ESN[12] = '\0';
            LOG_I("getted %s  ESN :%s", unb_device->device_name, unb_device->ESN);
        }
    }
    join_tick = 0;
    while (0 == unb_device->net_joined)
    {
        if (at_obj_exec_cmd(at_client_get(unb_device->client_name), unb_device->at_resp, COMMAND_LISTS[2]) != RT_EOK)
        {
            LOG_I("AT client send commands failed, response error or timeout !");
            rt_device_write(device, 0, GO_AT_MODE, (sizeof(GO_AT_MODE) - 1));
        }
        else
        {
            LOG_D("send successfull->%s", COMMAND_LISTS[2]);
            str_ptr = at_resp_get_line_by_kw(unb_device->at_resp, (const char *) COMMAND_RESPS[2]);
            if (RT_NULL != str_ptr)
            {
                LOG_I(" %s joined", unb_device->device_name);
                unb_device->net_joined = 1;
            }
        }
        join_tick++;
        if (join_tick > 20)
        {
            join_tick = 0;
            err_tick++;
            if(err_tick>=5)
            {
                //单片机复位
                rt_hw_cpu_reset();
            }
            //当前在扫频
            rt_thread_mdelay(3000);
            rt_device_write(device, 0, DEF_DEFAUT, (sizeof(DEF_DEFAUT) - 1));
            rt_thread_mdelay(3000);
            rt_device_close(device);
            goto RESTART;
        }
        rt_thread_mdelay(5000);
    }
    LOG_I("[%s] init successfull", unb_device->device_name);
    rt_device_close(device);
    return 0;
}

int unb_send_s(int argc, char *argv[])
{
    uint8_t *obj;
    uint8_t *str;
    at_device_tp1107 *ins = NULL;
#if UNB_DOUBLE
    if (argc < 3)
        return -1;
    if (rt_strstr(obj, "1"))
    {
        ins = &tp1107;
    }
    else
    {
        ins = &tp1107_2;
    }
    obj = argv[1];
    str = argv[2];
#else
    if(argc <2 )
    return -1;
    ins=&tp1107;
    str=argv[1];
#endif
    uint8_t length = (uint8_t) rt_strlen((const char * )str);
    unb_data_send(ins, str, length);
    return 0;
}
MSH_CMD_EXPORT(unb_send_s, unb shell);

static rt_thread_t tid1 = RT_NULL;
static uint8_t unbbuffer1[128];
#if UNB_DOUBLE
static rt_thread_t tid2 = RT_NULL;
static uint8_t unbbuffer2[128];

#endif
static void unb1_entry(void *parameter)
{

    uint8_t len;
    uint8_t res;
    uint8_t err_tick=0;
//    char msg[128] = { 0 };
//    for (uint16_t i = 0; i < hex_len; i++)
//    {
//        msg[i] = StrConv2Hex(str + i * 2);
//    }
    at_device_tp1107 *device;
#if UNB_DOUBLE
    device=&tp1107_2;
#else
    device=&tp1107;
#endif
   while (1)
   {
       uint8_t *customer=NULL;

       if (RT_EOK == rt_mq_recv(&mq_1, &customer, sizeof(customer), RT_WAITING_FOREVER))
       {
           if (customer !=NULL)
           {
               len=*customer;
               rt_memcpy(unbbuffer1, customer+1, len);

//               rt_mq_recv(&mq_1, unbbuffer1, len, RT_WAITING_FOREVER);
               unbbuffer1[len] = '\0';
               rt_free(customer);
               LOG_D("unb1 Received size:[%d]data: %s\n", len, unbbuffer1);
               rt_thread_mdelay(10);
#if UNB_DOUBLE

#else

#endif
               res=unb_data_send_direct(device, unbbuffer1, len / 2);
               while(res !=0) {
                   res=unb_data_send_direct(device, unbbuffer1, len / 2);
                   rt_thread_mdelay(500);
                   err_tick++;
                   LOG_E("[%s] send err tick:%d",device->device_name,err_tick);
                   if(0==err_tick % 10)
                   {
                       //reboot
                       if (at_obj_exec_cmd(at_client_get(device->client_name),device->at_resp, COMMAND_LISTS[4]) != RT_EOK)
                       {
                           LOG_I("AT client send commands failed, response error or timeout !");
                       }
                       else{
                           LOG_I("reboot unb :%s",device->device_name);
                       }

                   }
                   else if(0==err_tick %5){
                       //意外断网？发送AT+JOIN
                       if (at_obj_exec_cmd(at_client_get(device->client_name), device->at_resp, COMMAND_LISTS[5]) != RT_EOK)
                       {
                           LOG_I("AT client send commands failed, response error or timeout !");
                       }
                       else {
                           LOG_I("re JOIN unb :%s",device->device_name);
                    }
                   }


                   if(err_tick==17)
                   {
                       //单片机复位
                       rt_hw_cpu_reset();

                   }
                   rt_thread_mdelay(1000);
               }
               err_tick=0;



           }
       }
       rt_thread_mdelay(500);
   }


}
#if UNB_DOUBLE
static void unb2_entry(void *parameter)
{

    uint8_t len;
    uint8_t res;
    uint8_t err_tick=0;
    while (1)
    {
        uint8_t *customer=NULL;
        if (RT_EOK == rt_mq_recv(&mq_2, &customer, sizeof(customer), RT_WAITING_FOREVER))
        {
          if (customer !=NULL)
          {
//              rt_mq_recv(&mq_2, unbbuffer2, len, RT_WAITING_FOREVER);
              len=*customer;
              rt_memcpy(unbbuffer2, customer+1, len);
              unbbuffer2[len] = '\0';
              rt_free(customer);
              LOG_D("unb2 Received size:[%d]data: %s\n", len, unbbuffer2);
              rt_thread_mdelay(10);
//              unb_data_send_direct(&tp1107, unbbuffer2, len / 2);
              res=unb_data_send_direct(&tp1107, unbbuffer2, len / 2);
              while(res !=0) {
                  res=unb_data_send_direct(&tp1107, unbbuffer2, len / 2);
                  rt_thread_mdelay(500);
                  err_tick++;
                  LOG_E("[%s] send err tick:%d",tp1107.device_name,err_tick);
                  if(0==err_tick %10)
                  {
                      //reboot
                      if (at_obj_exec_cmd(at_client_get(tp1107.client_name),tp1107.at_resp, COMMAND_LISTS[4]) != RT_EOK)
                      {
                          LOG_I("AT client send commands failed, response error or timeout !");
                      }
                      else{
                          LOG_I("reboot unb :%s",tp1107.device_name);
                      }

                  }
                  else if(0==err_tick %5){
                      //意外断网？发送AT+JOIN
                      if (at_obj_exec_cmd(at_client_get(tp1107.client_name), tp1107.at_resp, COMMAND_LISTS[5]) != RT_EOK)
                      {
                          LOG_I("AT client send commands failed, response error or timeout !");
                      }
                      else {
                          LOG_I("re JOIN unb :%s",tp1107.device_name);
                   }
                  }


                  if(err_tick==17)
                  {
                      //单片机复位
                      rt_hw_cpu_reset();

                  }

                  rt_thread_mdelay(1000);
              }
              err_tick=0;
          }
      }
      rt_thread_mdelay(1000);
    }

}
#endif
//@#1
const static uint8_t UNB1_HEART[6]="402331";
//@#2
const static uint8_t UNB2_HEART[6]="402332";

static void timeout1(void *parameter)
{
    while(1)
    {
        rt_thread_mdelay(HEART_TICK_MINUTES*60*1000);
        LOG_I("send to[ %s] heart pack !!!",tp1107.ESN);
          int result;
          uint8_t len = 6;
      // 中断中不能加锁
          uint8_t *pMsg;
          pMsg = rt_malloc(len + 2);
          if(pMsg == NULL)
          {
              LOG_W("Mem alloc Error in msg production.");
              continue;
          }
          pMsg[0]=len;
          rt_memcpy(pMsg+1,UNB1_HEART, len);
          result = rt_mq_send(&mq_1, &pMsg, sizeof(pMsg));
          if (result != RT_EOK)
          {
              rt_kprintf("unb1 rt_mq_send ERR\n");
          }
    }


/*
    result = rt_mq_send(&mq_1, &len, 1);
    if (result != RT_EOK)
    {
      rt_kprintf("unb1 rt_mq_send ERR\n");
    }
    else {
//      LOG_I("unb1 len=%d",len);
    }
    //    rt_kprintf("unb1: send message_len - %d\n", len);
    //    rt_kprintf("str:%s\r\n", str);
    //    str[len]='\0';
    result = rt_mq_send(&mq_1, UNB1_HEART, len);
    if (result != RT_EOK)
    {
      rt_kprintf("unb1 rt_mq_send ERR\n");
    }
    else{
//      LOG_I("unb1 data mq send");
    }

*/
}
#if UNB_DOUBLE
static void timeout2(void *parameter)
{
    while(1)
    {
        rt_thread_mdelay(HEART_TICK_MINUTES*60*1000);
        LOG_I("send to[ %s] heart pack !!!",tp1107_2.ESN);
        int result;
        uint8_t len = 6;
        uint8_t *pMsg;
        pMsg = rt_malloc(len + 2);
        if(pMsg == NULL)
        {
            LOG_W("Mem alloc Error in msg production.");
            continue;
        }
        pMsg[0]=len;
        rt_memcpy(pMsg+1,UNB2_HEART, len);
        result = rt_mq_send(&mq_2, &pMsg, sizeof(pMsg));
        if (result != RT_EOK)
        {
            rt_kprintf("unb2 rt_mq_send ERR\n");
        }
    }

/*    result = rt_mq_send(&mq_2, &len, 1);
    if (result != RT_EOK)
    {
      rt_kprintf("unb2 rt_mq_send ERR\n");
    }
    else {
//      LOG_I("unb2 len=%d",len);
    }
    //    rt_kprintf("unb1: send message_len - %d\n", len);
    //    rt_kprintf("str:%s\r\n", str);
    //    str[len]='\0';
    result = rt_mq_send(&mq_2, UNB2_HEART, len);
    if (result != RT_EOK)
    {
      rt_kprintf("unb2 rt_mq_send ERR\n");
    }
    else{
//      LOG_I("unb2 data mq send");
    }*/

}
#endif

int unb_tp1107_init(void)
{
    rt_err_t result;
    result = rt_mq_init(&mq_1, "unb_mq1", &msg_pool_1[0], /* 内存池指向msg_pool */
    4, /* 每个消息的大小是 1 字节 */
    sizeof(msg_pool_1), /* 内存池的大小是msg_pool的大小 */
    RT_IPC_FLAG_FIFO); /* 如果有多个线程等待，按照先来先得到的方法分配消息 */
    if (result != RT_EOK)
    {
        rt_kprintf("init message queue failed.\n");
        return -1;
    }
#if UNB_DOUBLE
    result = rt_mq_init(&mq_2, "unb_mq2", &msg_pool_2[0], /* 内存池指向msg_pool */
    4, /* 每个消息的大小是 1 字节 */
    sizeof(msg_pool_2), /* 内存池的大小是msg_pool的大小 */
    RT_IPC_FLAG_FIFO); /* 如果有多个线程等待，按照先来先得到的方法分配消息 */
    if (result != RT_EOK)
    {
        rt_kprintf("init message queue failed.\n");
        return -1;
    }
#endif
    unb_tp1107_rea_init(&tp1107, tp1107_urc_table);



#if UNB_DOUBLE
    unb_tp1107_rea_init(&tp1107_2, tp1107_2_urc_table);




#endif

    tid1 = rt_thread_create("UNB1_handle", unb1_entry, RT_NULL, 1024, 20, 10);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    tid1 = rt_thread_create("UNB2_heart", timeout1, RT_NULL, 512, 20, 10);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);
#if UNB_DOUBLE
    tid2 = rt_thread_create("UNB2_handle", unb2_entry, RT_NULL, 1024, 20, 10);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);


    tid2 = rt_thread_create("UNB2_heart", timeout2, RT_NULL, 512, 20, 10);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);
#endif
    return 0;
}
INIT_APP_EXPORT(unb_tp1107_init);

