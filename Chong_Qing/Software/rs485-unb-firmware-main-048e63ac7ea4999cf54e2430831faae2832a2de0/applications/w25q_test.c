/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-01-12     starry       the first version
 */
#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include "drv_spi.h"
//#include "spi_flash_sfud.h"

static int rt_hw_spi_flash_init(void)
{
//    __HAL_RCC_GPIOB_CLK_ENABLE();
    rt_hw_spi_device_attach("spi1", "spi10", GPIOC, GPIO_PINS_4);// spi10 表示挂载在 spi3 总线上的 0 号设备,PC0是片选，这一步就可以将从设备挂在到总线中。

    if (RT_NULL == rt_sfud_flash_probe("norflash0", "spi10"))  //注册块设备，这一步可以将外部flash抽象为系统的块设备
    {
        return -RT_ERROR;
    };

    return RT_EOK;
}
/* 导出到自动初始化 */
//INIT_DEVICE_EXPORT(rt_hw_spi_flash_init);
