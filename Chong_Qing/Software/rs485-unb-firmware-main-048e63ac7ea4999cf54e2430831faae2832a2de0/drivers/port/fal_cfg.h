/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-01-12     starry       the first version
 */
#ifndef DRIVERS_PORT_FAL_CFG_H_
#define APPLICATIONS_FAL_CFG_H_

#include <rtconfig.h>
#include <board.h>

//#define NOR_FLASH_DEV_NAME             "norflash0"
#define AT32_FLASH_DEV_NAME             "onchip_flash"

/* ===================== Flash device Configuration ========================= */

extern struct fal_flash_dev M_nor_flash0;
extern const struct fal_flash_dev at32f4_onchip_flash;
/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &at32f4_onchip_flash,                                           \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                               \
{                                                                                    \
    {FAL_PART_MAGIC_WORD,        "bl", AT32_FLASH_DEV_NAME,         0,   20*1024, 0}, \
    {FAL_PART_MAGIC_WORD,       "app", AT32_FLASH_DEV_NAME,   20*1024,  876*1024, 0}, \
    {FAL_PART_MAGIC_WORD,       "db", AT32_FLASH_DEV_NAME,   896*1024,  128*1024, 0}, \
}

#endif /* FAL_PART_HAS_TABLE_CFG */


#endif /* DRIVERS_PORT_FAL_CFG_H_ */
