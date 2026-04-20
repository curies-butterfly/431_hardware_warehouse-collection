# AT32F403ARGT7 RS485-UNB && UNB-UNB

# 使用BOOT的MSD_IAP U盘更新固件的办法

将右侧的4个拨码开关全部拨下来(B4-B7)
文件名改为 `A005000.BIN `
拖到U盘中
升级完成后拨码开关复原

# 当前工程为APP部分
相比较与正常工程的改变如下
- main.c

```
//偏移地址长度为0x5000  20KB（BOOT大小）
#include "at32f403a_407.h"
static int ota_app_vtor_reconfig(void)
{
    nvic_vector_table_set(0x08000000,0x5000);
    return 0;
}

INIT_BOARD_EXPORT(ota_app_vtor_reconfig);
```

- linkscripts/AT32F403ARG/link.lds

ROM 起始地址 大小

- board.h

```
#define ROM_START              ((uint32_t)0x08005000)
#define ROM_SIZE               (1004 * 1024)
#define ROM_END                ((uint32_t)(ROM_START + ROM_SIZE))
```


# 踩坑记录

RTT 自带的内部flash驱动有问题



# 参考

https://github.com/loogg/agile_modbus_mcu_demos
https://blog.csdn.net/qq_44902027/article/details/143718337


