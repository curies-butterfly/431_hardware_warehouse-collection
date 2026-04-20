/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-13     starry       the first version
 */
#include <rtthread.h>
#include "board.h"
#include "drv_common.h"
#include <rtdevice.h>
#include "at_unb_config.h"
#define DBG_TAG "encoder"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define ADDR0_PIN GET_PIN(C, 10)
#define ADDR1_PIN GET_PIN(C, 11)
#define ADDR2_PIN GET_PIN(C, 12)
#define ADDR3_PIN GET_PIN(B, 3)

#define ADDR4_PIN GET_PIN(B, 4)
#define ADDR5_PIN GET_PIN(B, 5)
#define ADDR6_PIN GET_PIN(B, 6)
#define ADDR7_PIN GET_PIN(B, 7)

static rt_base_t pins[8]={ADDR0_PIN,ADDR1_PIN,ADDR2_PIN,ADDR3_PIN,\
                         ADDR4_PIN,ADDR5_PIN,ADDR6_PIN,ADDR7_PIN};
static uint8_t slave_addr=0;
static int addr_pin_init(void)
{
    rt_pin_mode(ADDR0_PIN,PIN_MODE_INPUT);
    rt_pin_mode(ADDR1_PIN,PIN_MODE_INPUT);
    rt_pin_mode(ADDR2_PIN,PIN_MODE_INPUT);
    rt_pin_mode(ADDR3_PIN,PIN_MODE_INPUT);

    rt_pin_mode(ADDR4_PIN,PIN_MODE_INPUT);
    rt_pin_mode(ADDR5_PIN,PIN_MODE_INPUT);
    rt_pin_mode(ADDR6_PIN,PIN_MODE_INPUT);
    rt_pin_mode(ADDR7_PIN,PIN_MODE_INPUT);

    return 0;
}

static uint8_t read_slave_level(void)
{
    uint8_t value=0;
    for(uint8_t i=0;i<8;i++)
    {
        value +=0xff & (rt_pin_read(pins[i])<<i);
    }
    return (~value) & 0x7f;
}
uint8_t get_slave_addr(void)
{
    rt_err_t ret = RT_EOK;
    uint8_t curret_addr=0,last_slave_addr=0;
    addr_pin_init();
    last_slave_addr=read_slave_level();
    rt_thread_mdelay(10);
    curret_addr=read_slave_level();
    if(last_slave_addr !=curret_addr)
    {
        LOG_E("get addr error!");
        return 0xff;
    }
    LOG_I("get addr:0x%x",curret_addr);
    return curret_addr;
}
uint8_t get_work_mode()
{
    uint8_t temp_mode=0,mode=0;
    temp_mode=rt_pin_read(pins[7]);
    rt_thread_mdelay(10);
    mode=rt_pin_read(pins[7]);
    if(temp_mode !=mode)
    {
        LOG_E("get mode error!");
        return 0xFF;
    }
    LOG_I("get mode:0x%x",mode);
    if(1==mode)
    {
        return MODE_RFID;
    }
    else {
        return MODE_DUST;
    }

}
static int slave_addr_s(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    uint8_t last_slave_addr=0;
//    addr_pin_init();
    last_slave_addr=read_slave_level();
    rt_thread_mdelay(10);
    slave_addr=read_slave_level();
    if(last_slave_addr !=slave_addr)
    {
        LOG_E("get addr error!");
        return -1;
    }
    LOG_I("get addr:0x%x",slave_addr);
    return ret;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(slave_addr_s, encoder_switch);


INIT_ENV_EXPORT(addr_pin_init);
