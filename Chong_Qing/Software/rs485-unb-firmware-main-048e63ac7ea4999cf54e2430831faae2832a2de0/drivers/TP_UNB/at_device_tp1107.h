/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-12     pc       the first version
 */
#ifndef DRIVERS_TP_UNB_AT_DEVICE_TP1107_H_
#define DRIVERS_TP_UNB_AT_DEVICE_TP1107_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <rtthread.h>
#include <at.h>
#include <rtdevice.h>
#include <stdlib.h>

typedef struct 
{
    char *device_name;
    char *client_name;
    uint8_t net_joined;
    int rst_pin;
    int wake_pin;
    char ESN[13];
    at_response_t at_resp;
    void *user_data;
}at_device_tp1107;





#ifdef __cplusplus
}
#endif
#endif /* DRIVERS_TP_UNB_AT_DEVICE_TP1107_H_ */
