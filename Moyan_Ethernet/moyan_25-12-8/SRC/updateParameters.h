//
// Created by starry on 2024/8/21.
//

#ifndef MAGICEYEPARAMETERSETTING_UPDATEPARAMETERS_H
#define MAGICEYEPARAMETERSETTING_UPDATEPARAMETERS_H



#include <stdint.h>
#include <stdbool.h>
#include "types.h"


#define  magicEye_event_t  uint8_t

enum {
    SAVING_DEV_ID =0,
    SAVING_APP_IP =1,
    SAVINF_APP_PORT=2,
    SAVING_BOOT_IP =3,
    SAVINF_BOOT_PORT=4
};
typedef struct
{
    /* setup by user */
    magicEye_event_t (*delay_ms)(uint32_t t);
    magicEye_event_t (*inputBUttons)(void);
    magicEye_event_t (*save_paramets_handle)(uint8_t index);
    magicEye_event_t (*set_check_mode_paramets)(void);

    const char * err_msg;
    uint8_t input_str_length;
    char  input_instruct_str[128];
    char * amend_APP_IP_PORT_instrcut;
    char * amend_BOOT_IP_PORT_instrcut;
    char * amend_BOOT_devID_instruct;

    magicEye_event_t current_state;
    magicEye_event_t running_check;

}_Board;
#endif //MAGICEYEPARAMETERSETTING_UPDATEPARAMETERS_H
