/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-12     pc       the first version
 */
#include <stdio.h>
#include <board.h>
#include <flashdb.h>
#include "at_unb_config.h"
#define FDB_LOG_TAG "flashdb"
static uint32_t boot_count = 0;
struct fdb_kvdb _global_kvdb = {0};
struct fdb_tsdb _global_tsdb = {0};

extern void kvdb_basic_sample(fdb_kvdb_t kvdb);

static struct fdb_default_kv_node default_kv_table[] = {{"boot_count", &boot_count, sizeof(boot_count)}}; /* int type KV */
static void kvdb_labels_data(fdb_kvdb_t kvdb);
static int flashdb(void)
{
    fdb_err_t result;
    struct fdb_default_kv default_kv;
    default_kv.kvs = default_kv_table;
    default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
    result = fdb_kvdb_init(&_global_kvdb, "env", "db", &default_kv, NULL);
    //init函数的第三个参数，是用于存储数据的fal分区，大家根据自己的情况选择

    if (result != FDB_NO_ERR)
    {
        return -1;
    }
    else
    {
        kvdb_labels_data(&_global_kvdb);
    }
    return RT_EOK;
}
INIT_ENV_EXPORT(flashdb);



// 定义标签名数组
const char *labels_name[20] = {
    LABEL_1, LABEL_2, LABEL_3, LABEL_4, LABEL_5,
    LABEL_6, LABEL_7, LABEL_8, LABEL_9, LABEL_10,
    LABEL_11, LABEL_12, LABEL_13, LABEL_14, LABEL_15,
    LABEL_16, LABEL_17, LABEL_18, LABEL_19, LABEL_20
};

static uint8_t default_labels_nums[20]={18,12, 3, 9,21,\
                                6, 3, 3, 9, 9,\
                                9, 9, 6, 9, 6,\
                                3, 3, 3,};
uint8_t current_labels_nums[20]={0};

static void kvdb_labels_data(fdb_kvdb_t kvdb)
{
    struct fdb_blob blob;
    int boot_count = 0;

    FDB_INFO("==================== load ====================\n");

    { /* GET the KV value */
        /* get the "boot_count" KV value */
        fdb_kv_get_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            FDB_INFO("get the 'boot_count' value is %d\n", boot_count);
        } else {
            FDB_INFO("get the 'boot_count' failed\n");
        }
        for(uint8_t i=0;i<20;i++)
        {
            fdb_kv_get_blob(kvdb, labels_name[i], fdb_blob_make(&blob, &current_labels_nums[i], sizeof(current_labels_nums[i])));
           /* the blob.saved.len is more than 0 when get the value successful */
           if (blob.saved.len > 0) {
               FDB_INFO("get the '%s' value is %d\n", labels_name[i],current_labels_nums[i]);
               if(current_labels_nums[i] != default_labels_nums[i])
               {
                   fdb_kv_set_blob(kvdb, labels_name[i], fdb_blob_make(&blob, &default_labels_nums[i], sizeof(default_labels_nums[i])));
                   FDB_INFO("set the '%s' value to %d\n",labels_name[i], default_labels_nums[i]);
               }
           } else {
               FDB_INFO("get the '%s' failed\n",labels_name[i]);
           }
        }
    }

    { /* CHANGE the KV value */
        /* increase the boot count */
        boot_count ++;
        /* change the "boot_count" KV's value */
        fdb_kv_set_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        if(1==boot_count){//1==boot_count
            for(uint8_t i=0;i<20;i++)
            {
                fdb_kv_set_blob(kvdb, labels_name[i], fdb_blob_make(&blob, &default_labels_nums[i], sizeof(default_labels_nums[i])));
                FDB_INFO("set the '%s' value to %d\n",labels_name[i], default_labels_nums[i]);
            }

        }
    }

    FDB_INFO("===========================================================\n");
}
