/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-11-30     pc       the first version
 */
#ifndef DRIVERS_TP_UNB_TP2210_H_
#define DRIVERS_TP_UNB_TP2210_H_

#include <rtthread.h>
#include <rtdevice.h>

struct tp2210_manager{
    uint8_t ops;
};


void TP2210_CONFIG(struct tp2210_manager *ins);
#endif /* DRIVERS_TP_UNB_TP2210_H_ */
