/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-15     starry       the first version
 */
#ifndef APPLICATIONS_AT_UNB_CONFIG_H_
#define APPLICATIONS_AT_UNB_CONFIG_H_





#define  TP1107_DEVICE_NAME "tp1107"
#define  TP1107_CLIENT_NAME "uart3"
#define  TP1107_RST_PIN   GET_PIN(A, 1)
#define  TP1107_WAKE_PIN   GET_PIN(A, 0)
#define  TP1107_DEFAUT_ESN "FF01FFFF0000"

#define UNB_DOUBLE 1
#define UNB_DEBUG  0
#define HEART_TICK_MINUTES  9
#if UNB_DOUBLE

#define  TP1107_2_DEVICE_NAME "tp1107_2"
#define  TP1107_2_CLIENT_NAME "uart6"
#define  TP1107_2_RST_PIN   GET_PIN(C, 8)
#define  TP1107_2_WAKE_PIN   GET_PIN(B, 12)
#define  TP1107_2_DEFAUT_ESN "FF01FFFF0000"



#endif



#define LABEL_1 "READER_1"
#define LABEL_2 "READER_2"
#define LABEL_3 "READER_3"
#define LABEL_4 "READER_4"
#define LABEL_5 "READER_5"
#define LABEL_6 "READER_6"
#define LABEL_7 "READER_7"
#define LABEL_8 "READER_8"
#define LABEL_9 "READER_9"
#define LABEL_10 "READER_10"
#define LABEL_11 "READER_11"
#define LABEL_12 "READER_12"
#define LABEL_13 "READER_13"
#define LABEL_14 "READER_14"
#define LABEL_15 "READER_15"
#define LABEL_16 "READER_16"
#define LABEL_17 "READER_17"
#define LABEL_18 "READER_18"
#define LABEL_19 "READER_19"
#define LABEL_20 "READER_20"

#define MODE_RFID  0x11
#define MODE_DUST  0x22

#endif /* APPLICATIONS_AT_UNB_CONFIG_H_ */
