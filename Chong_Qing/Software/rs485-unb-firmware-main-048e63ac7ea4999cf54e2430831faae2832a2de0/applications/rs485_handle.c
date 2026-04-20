/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-05     starry       the first version
 */
#include <rtthread.h>
#include "board.h"
#include "drv_common.h"
#include <rtdevice.h>
#include "at_unb_config.h"
#define DBG_TAG "rs485"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define RS485_RE_PIN GET_PIN(A, 8)
#define RS485_TX_EN() rt_pin_write(RS485_RE_PIN, PIN_HIGH)
#define RS485_RX_EN() rt_pin_write(RS485_RE_PIN, PIN_LOW)


static rt_device_t _dev = RT_NULL;
static rt_sem_t _rx_sem = RT_NULL;

static rt_err_t rs485_ind_cb(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(_rx_sem);

    return RT_EOK;
}

static int rs485_send(uint8_t *buf, rt_size_t len)
{
    RS485_TX_EN();
    rt_device_write(_dev, 0, buf, len);
    RS485_RX_EN();

    return len;
}

static int rs485_receive(uint8_t *buf, int bufsz, int timeout, int bytes_timeout)
{
    int len = 0;

    while(1)
    {
        rt_sem_control(_rx_sem, RT_IPC_CMD_RESET, RT_NULL);

        int rc = rt_device_read(_dev, 0, buf + len, bufsz);
        if(rc > 0)
        {
            timeout = bytes_timeout;
            len += rc;
            bufsz -= rc;
            if(bufsz == 0)
                break;

            continue;
        }

        if(rt_sem_take(_rx_sem, rt_tick_from_millisecond(timeout)) != RT_EOK)
            break;
        timeout = bytes_timeout;
    }

    return len;
}

static int rs485_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */
    rt_pin_mode(RS485_RE_PIN, PIN_MODE_OUTPUT);
    RS485_RX_EN();

    _rx_sem = rt_sem_create("rs485", 0, RT_IPC_FLAG_FIFO);
    if(_rx_sem == RT_NULL)
    {
        LOG_E("create rx_sem failed.");
        return -RT_ERROR;
    }

    _dev = rt_device_find("uart8");
    if (_dev == RT_NULL)
    {
        LOG_E("can't find device uart8.");
        rt_sem_delete(_rx_sem);
        return -RT_ERROR;
    }
    /* step2：修改串口配置参数 */
    config.baud_rate = BAUD_RATE_9600;        //修改波特率为 9600
    config.data_bits = DATA_BITS_8;           //数据位 8
    config.stop_bits = STOP_BITS_1;           //停止位 1
    config.bufsz     = 128;                   //修改缓冲区 buff size 为 128
    config.parity    = PARITY_NONE;           //无奇偶校验位

    /* step3：控制串口设备。通过控制接口传入命令控制字，与控制参数 */
    rt_device_control(_dev, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_set_rx_indicate(_dev, rs485_ind_cb);
    rt_device_open(_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);

    return RT_EOK;
}
static rt_timer_t timer_rs;
//static rt_sem_t dynamic_sem = RT_NULL;
static struct rt_messagequeue mq_rs;
static rt_uint8_t msg_pool_rs[1024];

static uint8_t period_read=0;

static void timeout_rs(void *parameter)
{
    LOG_I("period RFID DATA SEND sem release");
//    rt_sem_release(dynamic_sem);
    period_read=1;


}
extern uint8_t get_slave_addr(void);
extern uint8_t get_work_mode();
//#define DBG_TAG "rtu_master"
#include <agile_modbus.h>

extern uint8_t current_labels_nums[20];
static uint8_t current_addr=0;
static uint8_t res_buf[100];
static uint8_t ctx_send_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
static uint8_t ctx_read_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
static uint8_t work_mode=MODE_RFID;
#define DUST_HEAD_H  0xAA
#define DUST_HEAD_L  0x11
#define TEMP_ABS(m,n) ((m) > (n) ? (m) - (n) : (n) - (m))

#define MONITORING_TICK_threshold 5
//温度变化门限  temperature * 100
#define MONITORING_CAHGNE_threshold 00
static uint8_t current_value_change_tick[30]={0};
static uint8_t value_cahnge_rate_monitor(uint16_t *indata,uint8_t length)
{
    static uint16_t current_value[30]={0};
    uint8_t i,flag=0;;
    for( i=0;i<length;i++)
    {
        if(TEMP_ABS(indata[i],current_value[i]) >  MONITORING_CAHGNE_threshold)
        {
            current_value_change_tick[i]++;
        }
        current_value[i]=indata[i];
    }
    for(i=0;i<length;i++)
    {
        if(current_value_change_tick[i] >= MONITORING_TICK_threshold)
       {
            flag=1;
       }
    }
    if(flag)
    {
//        rt_memset(current_value_change_tick, 0, 30);
        return 1;
    }


    return 0;
}
static void rs485_thread_entry(void *parameter)
{
    int startAddr=1105,nbs=10;
    uint8_t temp_id=0x30;
    rt_err_t result;
    rs485_init();
    work_mode=get_work_mode();
    if(work_mode==MODE_RFID){
       current_addr=get_slave_addr();
       current_addr=current_addr==0?1:current_addr;
       nbs=current_labels_nums[current_addr-1];
       LOG_I("work in rfid reader->addr=%d,nums=%d",current_addr,nbs);
    }else if(work_mode==MODE_DUST){
        current_addr=1;
        nbs=1;
        startAddr=14;
        temp_id +=get_slave_addr();
        LOG_I("work in dust reader->addr=%d,id=0x%x",current_addr,temp_id);
    }

   uint16_t hold_register[30];

   agile_modbus_rtu_t ctx_rtu;
   agile_modbus_t *ctx = &ctx_rtu._ctx;
   agile_modbus_rtu_init(&ctx_rtu, ctx_send_buf, sizeof(ctx_send_buf), ctx_read_buf, sizeof(ctx_read_buf));
   agile_modbus_set_slave(ctx, current_addr);

   LOG_I("Running.");
   rt_size_t send_len;
   uint8_t first_send=1;


   while (1)
  {
      LOG_I("tx.");
//      LOG_I("period_read=%d",period_read);
       send_len = agile_modbus_serialize_read_registers(ctx, startAddr, nbs);
//      LOG_I("send_len=%d",send_len);
      if(send_len<1)
      {
          continue;
      }
      rs485_send(ctx->send_buf, send_len);
//      LOG_I("rx.");
      int read_len = rs485_receive(ctx->read_buf, ctx->read_bufsz, 1000, 20);
//      res_buf=ctx->read_buf;

      if(work_mode==MODE_RFID){
          rt_memcpy(res_buf, ctx->read_buf, read_len);
      }
      else
      {
          res_buf[0]=DUST_HEAD_H;
          res_buf[1]=DUST_HEAD_L;
          rt_memcpy(res_buf+2, ctx->read_buf, read_len);
          res_buf[2]=temp_id;
      }

      if (read_len == 0)
      {
          LOG_W("Receive timeout.");
          continue;
      }

      int rc = agile_modbus_deserialize_read_registers(ctx, read_len, hold_register);
      if (rc < 0)
      {
          LOG_W("Receive failed.");
          if (rc != -1)
              LOG_W("Error code:%d", -128 - rc);

          continue;
      }
      LOG_I("Hold Registers:");
      for (int i = 0; i < 10; i++)
          LOG_I("temperature [%d]: %02d.%02d", i, hold_register[i]/100,hold_register[i]%100);
      if(first_send)
      {
          first_send=0;
      }
      else {
          if(work_mode==MODE_RFID)
          {
              if(0==value_cahnge_rate_monitor(hold_register,rc) && period_read==0)
               {
                  rt_thread_mdelay(30*1000);
                   continue ;
               }
              else {
                  LOG_I("OVER temp send way!!!!!!!!!!");
              }
          }
          else{
              if(period_read==0)
              {
                 rt_thread_mdelay(30*1000);
                 continue ;
              }
          }


    }

      period_read=0;
      rt_memset(current_value_change_tick, 0, 30);



      if(work_mode==MODE_DUST){
          read_len +=2;
      }
/*
      result = rt_mq_send(&mq_rs, &read_len, sizeof(read_len));
      if (result != RT_EOK)
     {
         rt_kprintf("rs485 rt_mq_send ERR\n");
     }
      result = rt_mq_send(&mq_rs, res_buf, read_len);
      if (result != RT_EOK)
      {
          rt_kprintf("rs485 rt_mq_send ERR\n");
      }
*/
     //Modified by KurehaTian

      // Production:
      // a Message block is organized as     | Len |     Data    | Null |
      // then pMsg (the head of this block) is push into MQ
      uint8_t *pMsg;
      pMsg = rt_malloc(read_len + 2);
      if(pMsg == NULL)
      {
          LOG_W("Mem alloc Error in msg production.");
          continue;
      }
      pMsg[0]=read_len;
      rt_memcpy(pMsg+1,res_buf, read_len);
      result = rt_mq_send(&mq_rs, &pMsg, sizeof(pMsg));
      if (result != RT_EOK)
      {
          rt_kprintf("rs485 rt_mq_send ERR\n");
      }

      rt_thread_mdelay(30*1000);

//

  }

}
static int rtu_sample(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;

    /* 创建 rtu 线程 */
    rt_thread_t thread = rt_thread_create("rtu-yh", rs485_thread_entry, RT_NULL, 1024, 25, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    return ret;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(rtu_sample, RTU 485 sample);


static uint8_t rsbbuffer[128];
// 用来存储转换后的字符串
//static char store_str[256];  // 确保足够大来存储转换结果
extern void DTU_UNB_DATA_SEND(uint8_t *data, uint8_t len);

static void rfid_unb_send_entry(void *parameter)
{
    uint8_t len;
   while (1)
   {
       uint8_t *customer=NULL;
       if (RT_EOK == rt_mq_recv(&mq_rs, &customer, sizeof(customer), RT_WAITING_FOREVER))
       {
           if (customer !=NULL)
           {
//               rt_mq_recv(&mq_rs, rsbbuffer, len, RT_WAITING_FOREVER);
               len=*customer;
               rt_memcpy(rsbbuffer, customer+1, len);
               rsbbuffer[len] = '\0';
               rt_free(customer);
               LOG_I("RFID READ");
               /*
               rt_kprintf("rs485 Received size:[%d]data: %s\n", len, rsbbuffer);
                     LOG_I("raw data:");
                for (int i = 0; i < len; i++)
                {
                 rt_kprintf("%02x ",rsbbuffer[i]);
                }
                rt_kprintf("\r\n");
               LOG_I("RFID READ END");

               rt_memset(store_str,0,256);
               store_str[0]='\0';
               for (size_t i = 0; i < len; i++) {
                       // 每次拼接时，确保不超出字符串长度
                       snprintf(store_str + strlen(store_str), sizeof(store_str) - strlen(store_str), "%02X", rsbbuffer[i]);
                       offset += snprintf(store_str + offset, MAX_STR_LEN - offset, "%02X", rsbbuffer[i]);

                   }
               // 打印转换后的字符串
               LOG_I("Converted HEX to String: %s\n", store_str);
               DTU_UNB_DATA_SEND(store_str,strlen(store_str));
               */

               DTU_UNB_DATA_SEND(rsbbuffer,len);
           }
       }
       rt_thread_mdelay(500);
   }


}
static int rtu_app_start(void)
{
    rt_err_t result;
    timer_rs = rt_timer_create("timer_rs", timeout_rs,
                             RT_NULL, 10*60*1000,
                             RT_TIMER_FLAG_PERIODIC);

    if (timer_rs != RT_NULL)
        rt_timer_start(timer_rs);

//    dynamic_sem = rt_sem_create("rsem", 0, RT_IPC_FLAG_FIFO);
//    if (dynamic_sem == RT_NULL)
//    {
//        LOG_E("create dynamic semaphore failed.\n");
//        return -1;
//    }
//    else
//    {
//        LOG_I("create done. dynamic semaphore value = 0.\n");
//    }

    result = rt_mq_init(&mq_rs, "unb_mq1", &msg_pool_rs[0], /* 内存池指向msg_pool */
                        4, /* 每个消息的大小是 1 字节 */
                        sizeof(msg_pool_rs), /* 内存池的大小是msg_pool的大小 */
                        RT_IPC_FLAG_FIFO); /* 如果有多个线程等待，按照先来先得到的方法分配消息 */
    if (result != RT_EOK)
    {
        rt_kprintf("init message queue failed.\n");
        return -1;
    }
    rt_err_t ret = RT_EOK;

    rt_thread_t tid1 = rt_thread_create("rfid_send", rfid_unb_send_entry, RT_NULL, 2048, 20, 10);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);


    /* 创建 rtu 线程 */
    rt_thread_t thread = rt_thread_create("rtu-read", rs485_thread_entry, RT_NULL, 2048, 20, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }
    return ret;
}
#if UNB_DOUBLE==0

INIT_APP_EXPORT(rtu_app_start);

#endif

/*
 *
 * result = rt_mq_send(&mq_rs, &read_len, sizeof(read_len));
      if (result != RT_EOK)
     {
         rt_kprintf("rs485 rt_mq_send ERR\n");
     }
      result = rt_mq_send(&mq_rs, res_buf, read_len);
      if (result != RT_EOK)
      {
          rt_kprintf("rs485 rt_mq_send ERR\n");
      }
 * */



static int hex_to_bytes(const char *hex_str, uint8_t *output) {
    size_t len = strlen(hex_str);
    if (len % 2 != 0) {  // 如果输入的HEX字符串长度是奇数，则不合法
        return -1;
    }

    for (size_t i = 0; i < len / 2; i++) {
        // 每两位字符转换成一个字节
        sscanf(&hex_str[i * 2], "%2hhx", &output[i]);
    }
    return len / 2;
}

static int rs485_shell(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }

    const char *hex_str = argv[1];  // 获取输入的HEX字符串
    size_t hex_len = strlen(hex_str);

    // 为存储转换后的字节数据分配足够的内存
    uint8_t data[hex_len / 2];  // 一半的长度是字节数

    // 将HEX字符串转换为字节数组
    int data_len = hex_to_bytes(hex_str, data);
    if (data_len < 0) {
        rt_kprintf("Invalid HEX input.\n");
        return -1;
    }

    // 打印转换后的字节数据
    rt_kprintf("Converted HEX bytes: ");
    for (int i = 0; i < data_len; i++) {
        rt_kprintf("%02X ", data[i]);
    }
    rt_kprintf("\n");

    // 假设你需要将这些字节数据发送到消息队列
    uint8_t read_len = data_len;  // 发送的字节数
    rt_err_t result = rt_mq_send(&mq_rs, &read_len, sizeof(read_len));
    if (result != RT_EOK) {
        rt_kprintf("rs485 rt_mq_send ERR1\n");
    }

    result = rt_mq_send(&mq_rs, data, read_len);  // 发送字节数据
    if (result != RT_EOK) {
        rt_kprintf("rs485 rt_mq_send ERR2\n");
    }

    return 0;
}
#if UNB_DOUBLE==0
MSH_CMD_EXPORT(rs485_shell, rs shell);

#endif
