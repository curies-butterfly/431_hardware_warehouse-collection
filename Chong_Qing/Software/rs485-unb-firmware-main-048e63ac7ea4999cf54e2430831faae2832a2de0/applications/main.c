/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-11-30     RT-Thread    first version
 */

#include <rtthread.h>
#include "board.h"
#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include "tp2210.h"

#include "drv_common.h"

#define LED_R GET_PIN(C, 13)
#define WDT_DEVICE_NAME    "wdt"    /* 看门狗设备名称 */
static rt_device_t wdg_dev;         /* 看门狗设备句柄 */
static void idle_hook(void)
{
    /* 在空闲线程的回调函数里喂狗 */
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
//    rt_kprintf("feed the dog!\n ");

}

#include <fal.h>
int main(void)
{
    int count = 1;
    fal_init();                                         //抽象层初始化

    static int pin_num1;
    rt_kprintf("HERE IS APP RUNNING ! V1.1.2\r\n");


    /* 设置PIN脚模式为输出 */
    pin_num1=LED_R;
    rt_pin_mode(pin_num1, PIN_MODE_OUTPUT);


    /* 根据设备名称查找看门狗设备，获取设备句柄 */
    wdg_dev = rt_device_find(WDT_DEVICE_NAME);
    rt_uint32_t timeout = 1;
    rt_err_t ret = RT_EOK;


    if (wdg_dev==RT_NULL)
    {
      rt_kprintf("find %s failed!\n", WDT_DEVICE_NAME);
    }

    /* 设置看门狗溢出时间 */
    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK)
    {
      rt_kprintf("set %s timeout failed!\n", WDT_DEVICE_NAME);
    }


    /* 启动看门狗 */
    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK)
    {
      rt_kprintf("start %s failed!\n", WDT_DEVICE_NAME);
    }
      /* 设置空闲线程回调函数 */
//    rt_thread_idle_sethook(idle_hook);

    while (count++)
    {
        idle_hook();                            //在空闲程序里喂狗
        rt_pin_write(pin_num1, PIN_LOW);
        rt_thread_mdelay(100);
        rt_pin_write(pin_num1, PIN_HIGH);
        rt_thread_mdelay(100);

    }

    return RT_EOK;      //返回运行状态
}

//#include "sgm706.h"
//
//static int rt_hw_sgm706_port(void)
//{
//    rt_hw_sgm706_init("wdt", 35); /* PB0 */
//
//    return RT_EOK;
//}
///* 注册看门狗设备 */
//INIT_COMPONENT_EXPORT(rt_hw_sgm706_port);


#include "at32f403a_407.h"
static int ota_app_vtor_reconfig(void)
{
    nvic_vector_table_set(0x08000000,0x5000);
    return 0;
}

INIT_BOARD_EXPORT(ota_app_vtor_reconfig);
