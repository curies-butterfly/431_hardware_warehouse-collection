/*
 * online_cluster.c
 *
 *  Created on: 2025年11月13日
 *      Author: 20107
 */
#include "online_cluster.h"
#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"
#include "I2cEeprom.h"  // 您的EEPROM头文件
#include "SCI.h"

// 循环变量放在外面
//uint8 i, j, k;
uint8 merged;
uint16 min_dist;
uint16 center_i, center_j;
uint16 distance;
//uint8 nearest_idx;
uint8 write_idx;
//uint8 range_count;
online_cluster_t oc;


// 初始化聚类器
void online_cluster_init(online_cluster_t* oc, uint16 merge_threshold,
                        uint8 min_cluster_size, uint16 margin) {
    oc->cluster_count = 0;
    oc->merge_threshold = merge_threshold;
    oc->min_cluster_size = min_cluster_size;
    oc->max_clusters = 6;  // 减少到6个以节省EEPROM空间
    oc->margin = margin;
    uint8 i;
    // 初始化所有聚类
    for (i = 0; i < oc->max_clusters; i++) {
        oc->clusters[i].count = 0;
        oc->clusters[i].sum = 0;
        oc->clusters[i].sum_sq = 0;
    }
}

// 寻找最近的聚类
static uint8 find_nearest_cluster(online_cluster_t* oc, uint16 value, uint16* distance) {
    uint8 best_idx = 0;
    uint16 min_dist = 0xFFFF;
    uint16 center;
    uint16 dist;
    uint8 i;

    for (i = 0; i < oc->cluster_count; i++) {
        if (oc->clusters[i].count > 0) {
            center = (uint16)(oc->clusters[i].sum / oc->clusters[i].count);
            dist = (value > center) ? (value - center) : (center - value);

            if (dist < min_dist) {
                min_dist = dist;
                best_idx = i;
            }
        }
    }

    *distance = min_dist;
    return best_idx;
}

// 合并值到聚类
static void merge_to_cluster(cluster_t* cluster, uint16 value) {
    if (cluster->count == 0) {
        cluster->min_val = value;
        cluster->max_val = value;
    } else {
        if (value < cluster->min_val) cluster->min_val = value;
        if (value > cluster->max_val) cluster->max_val = value;
    }
    cluster->count++;
    cluster->sum += value;
    cluster->sum_sq += (uint32)value * value;
}

// 合并两个聚类
static void combine_clusters(cluster_t* dest, cluster_t* src) {
    if (src->min_val < dest->min_val) dest->min_val = src->min_val;
    if (src->max_val > dest->max_val) dest->max_val = src->max_val;
    dest->count += src->count;
    dest->sum += src->sum;
    dest->sum_sq += src->sum_sq;
}

// 合并相近的聚类
static uint8 merge_close_clusters(online_cluster_t* oc) {
    if (oc->cluster_count < 2) return 0;

    uint8 merged_count = 0;

    do {
        merged = 0;
        uint8 i,j;
        for (i = 0; i < oc->cluster_count && !merged; i++) {
            for (j = i + 1; j < oc->cluster_count && !merged; j++) {
                center_i = (uint16)(oc->clusters[i].sum / oc->clusters[i].count);
                center_j = (uint16)(oc->clusters[j].sum / oc->clusters[j].count);

                distance = (center_i > center_j) ? (center_i - center_j) : (center_j - center_i);
                if (distance <= oc->merge_threshold) {
                    combine_clusters(&oc->clusters[i], &oc->clusters[j]);

                    // 移动最后一个聚类到j的位置
                    if (j != oc->cluster_count - 1) {
                        oc->clusters[j] = oc->clusters[oc->cluster_count - 1];
                    }
                    oc->cluster_count--;
                    merged = 1;
                    merged_count++;
                }
            }
        }
    } while (merged && oc->cluster_count > 1);

    return merged_count;
}

// 清理过小的聚类
// static void online_cluster_cleanup(online_cluster_t* oc) {
//     if (oc->cluster_count <= 1) return;

//     write_idx = 0;
//     uint8 i;
//     // 压缩数组，移除过小的聚类
//     for (i = 0; i < oc->cluster_count; i++) {
//         if (oc->clusters[i].count >= oc->min_cluster_size || oc->cluster_count == 1) {
//             if (write_idx != i) {
//                 oc->clusters[write_idx] = oc->clusters[i];
//             }
//             write_idx++;
//         }
//     }

//     oc->cluster_count = write_idx;

//     // 如果全部被清除了，恢复最大的一个
//     if (oc->cluster_count == 0 && oc->max_clusters > 0) {
//         oc->cluster_count = 1;
//     }
// }

// 更新聚类器 - 输入float，内部转换为uint16
// void online_cluster_update(online_cluster_t* oc, float new_value_float) {
//     // float转uint16，四舍五入，限制在0-1000范围内
//     uint16 new_value;
//     new_value_float = new_value_float;
//     uint8 nearest_idx;
//     if (new_value_float < 0) new_value_float = 0;
//     if (new_value_float > 1500) new_value_float = 1500;
//     new_value = (uint16)(new_value_float + 0.5f);
//     SCI_Printf("学习新数据%d",new_value);
//     // 如果还没有聚类，直接创建第一个
//     if (oc->cluster_count == 0) {
//         oc->clusters[0].min_val = new_value;
//         oc->clusters[0].max_val = new_value;
//         oc->clusters[0].count = 1;
//         oc->clusters[0].sum = new_value;
//         oc->clusters[0].sum_sq = (uint32)new_value * new_value;
//         oc->cluster_count = 1;
//         return;
//     }

//     // 寻找最近的聚类
//     nearest_idx = find_nearest_cluster(oc, new_value, &min_dist);

//     // 如果距离在阈值内，合并到现有聚类
//     if (min_dist <= oc->merge_threshold) {
//         merge_to_cluster(&oc->clusters[nearest_idx], new_value);
//     } else {
//         // 创建新聚类（如果有空间）
//         if (oc->cluster_count < oc->max_clusters) {
//             oc->clusters[oc->cluster_count].min_val = new_value;
//             oc->clusters[oc->cluster_count].max_val = new_value;
//             oc->clusters[oc->cluster_count].count = 1;
//             oc->clusters[oc->cluster_count].sum = new_value;
//             oc->clusters[oc->cluster_count].sum_sq = (uint32)new_value * new_value;
//             oc->cluster_count++;
//         } else {
//             // 没有空间时，合并到最近的聚类
//             merge_to_cluster(&oc->clusters[nearest_idx], new_value);
//         }
//     }

//     // 清理过小的聚类
//     online_cluster_cleanup(oc);

//     // 合并相近的聚类
//     merge_close_clusters(oc);
// }

// // 获取数据范围
// uint8 online_cluster_get_ranges(online_cluster_t* oc,
//                               uint16* ranges_min, uint16* ranges_max,
//                               uint8 max_ranges) {
// 	uint8 range_count = 0;  // 在这里定义
//     range_count = 0;
//     uint8 i;
//     for (i = 0; i < oc->cluster_count && range_count < max_ranges; i++) {
//         if (oc->clusters[i].count >= oc->min_cluster_size) {
//             // 计算范围，考虑边界限制
//             ranges_min[range_count] = (oc->clusters[i].min_val > oc->margin) ?
//                                      (oc->clusters[i].min_val - oc->margin) : 0;
//             ranges_max[range_count] = (oc->clusters[i].max_val + oc->margin < 1000) ?
//                                      (oc->clusters[i].max_val + oc->margin) : 1000;
//             range_count++;
//         }
//     }

//     return range_count;
// }



// 更新聚类器 - 移除清理过小聚类的逻辑1113
void online_cluster_update(online_cluster_t* oc, float new_value_float) {
    // float转uint16，四舍五入，限制在0-1500范围内
    uint16 new_value;
    uint8 nearest_idx;
    if (new_value_float < 0) new_value_float = 0;
    if (new_value_float > 1500) new_value_float = 1500;
    new_value = (uint16)(new_value_float + 0.5f);
    
    SCI_Printf("学习新数据%d", new_value);

    // 如果还没有聚类，直接创建第一个
    if (oc->cluster_count == 0) {
        oc->clusters[0].min_val = new_value;
        oc->clusters[0].max_val = new_value;
        oc->clusters[0].count = 1;
        oc->clusters[0].sum = new_value;
        oc->clusters[0].sum_sq = (uint32)new_value * new_value;
        oc->cluster_count = 1;
        return;
    }

    // 寻找最近的聚类
    nearest_idx = find_nearest_cluster(oc, new_value, &min_dist);

    // 如果距离在阈值内，合并到现有聚类
    if (min_dist <= oc->merge_threshold) {
        merge_to_cluster(&oc->clusters[nearest_idx], new_value);
    } else {
        // 创建新聚类（如果有空间）
        if (oc->cluster_count < oc->max_clusters) {
            oc->clusters[oc->cluster_count].min_val = new_value;
            oc->clusters[oc->cluster_count].max_val = new_value;
            oc->clusters[oc->cluster_count].count = 1;
            oc->clusters[oc->cluster_count].sum = new_value;
            oc->clusters[oc->cluster_count].sum_sq = (uint32)new_value * new_value;
            oc->cluster_count++;
        } else {
            // 没有空间时，合并到最近的聚类
            merge_to_cluster(&oc->clusters[nearest_idx], new_value);
        }
    }

    // 移除清理过小聚类的逻辑
    // online_cluster_cleanup(oc);

    // 保留合并相近聚类的逻辑
    merge_close_clusters(oc);
}
// ==================== EEPROM存储功能 ====================

// 写入uint16到EEPROM（需要2个地址）
static void write_uint16_to_eeprom(uint16 start_addr, uint16 value) {
    AT24CXX_WriteData(start_addr, (value >> 8) & 0xFF);    // 高8位
    AT24CXX_WriteData(start_addr + 1, value & 0xFF);       // 低8位
}

// 从EEPROM读取uint16
static uint16 read_uint16_from_eeprom(uint16 start_addr) {
    uint16 high = AT24CXX_ReadData(start_addr);
    uint16 low = AT24CXX_ReadData(start_addr + 1);
    return (high << 8) | low;
}

// 写入uint32到EEPROM（需要4个地址）
static void write_uint32_to_eeprom(uint16 start_addr, uint32 value) {
    write_uint16_to_eeprom(start_addr, (value >> 16) & 0xFFFF);
    write_uint16_to_eeprom(start_addr + 2, value & 0xFFFF);
}

// 从EEPROM读取uint32
static uint32 read_uint32_from_eeprom(uint16 start_addr) {
    uint32 high = read_uint16_from_eeprom(start_addr);
    uint32 low = read_uint16_from_eeprom(start_addr + 2);
    return (high << 16) | low;
}

// // 保存聚类数据到EEPROM
// void online_cluster_save_to_eeprom(online_cluster_t* oc) {
//     uint16 addr = CLUSTER_DATA_START_ADDR;

//     // 保存配置信息
//     AT24CXX_WriteData(CLUSTER_COUNT_ADDR, oc->cluster_count);
//     write_uint16_to_eeprom(CLUSTER_MARGIN_ADDR1, oc->margin);
//     write_uint16_to_eeprom(CLUSTER_MERGE_THRESH_ADDR1, oc->merge_threshold);
//     uint8 i;
//     // 保存每个聚类的数据
//     for (i = 0; i < oc->cluster_count; i++) {
//         write_uint16_to_eeprom(addr, oc->clusters[i].min_val);
//         addr += 2;
//         write_uint16_to_eeprom(addr, oc->clusters[i].max_val);
//         addr += 2;
//         write_uint16_to_eeprom(addr, oc->clusters[i].count);
//         addr += 2;
//         write_uint32_to_eeprom(addr, oc->clusters[i].sum);
//         addr += 4;

//         // 检查地址是否超出范围
//         if (addr > 250) {
//             SCI_Printf("警告: EEPROM地址即将溢出，停止保存更多数据\n");
//             break;  // 防止地址溢出
//         }
//         SCI_Printf("聚类%d: min=%d, max=%d, count=%d, sum=%lu, 地址范围:%d-%d\n",
//                i,
//                oc->clusters[i].min_val,
//                oc->clusters[i].max_val,
//                oc->clusters[i].count,
//                oc->clusters[i].sum,
//                CLUSTER_DATA_START_ADDR + i * 10,  // 起始地址
//                addr - 1);

//     }
// }


// 保存聚类数据到EEPROM - 只保存聚类数和边界
void online_cluster_save_to_eeprom(online_cluster_t* oc) {
    uint16 addr = CLUSTER_DATA_START_ADDR;

    // 1. 保存总聚类数
    AT24CXX_WriteData(CLUSTER_COUNT_ADDR, oc->cluster_count);
    
    uint8 i;
    // 2. 保存每个聚类的上下限
    for (i = 0; i < oc->cluster_count; i++) {
        // 保存最小值
        write_uint16_to_eeprom(addr, oc->clusters[i].min_val);
        addr += 2;
        
        // 保存最大值
        write_uint16_to_eeprom(addr, oc->clusters[i].max_val);
        addr += 2;

        // 打印保存的数据
        SCI_Printf("保存聚类%d: min=%d, max=%d\n", 
               i, oc->clusters[i].min_val, oc->clusters[i].max_val);

        // 检查地址是否超出范围
        if (addr > 250) {
            SCI_Printf("警告: EEPROM地址即将溢出\n");
            break;
        }
    }
    
    SCI_Printf("保存完成，聚类总数: %d\n", oc->cluster_count);
}

// 从EEPROM加载聚类数据 - 只加载聚类数和边界
uint8 online_cluster_load_from_eeprom(online_cluster_t* oc) {
    uint16 addr = CLUSTER_DATA_START_ADDR;
    uint8 i;
    ServiceDog();		//喂狗
    // 1. 读取总聚类数（用于循环上限）
    oc->cluster_count = AT24CXX_ReadData(CLUSTER_COUNT_ADDR);
    
    // 限制聚类数量，防止数据损坏
    if (oc->cluster_count > oc->max_clusters) {
        oc->cluster_count = oc->max_clusters;
    }

    // 2. 读取每个聚类的上下限
    for (i = 0; i < oc->cluster_count; i++) {
        // 读取最小值
        oc->clusters[i].min_val = read_uint16_from_eeprom(addr);
        addr += 2;
        
        // 读取最大值
        oc->clusters[i].max_val = read_uint16_from_eeprom(addr);
        addr += 2;
        
        // 其他字段设为默认值（因为不需要）
        oc->clusters[i].count = 1;  // 设为1保证该聚类有效
        oc->clusters[i].sum = oc->clusters[i].min_val;  // 简单初始化
        oc->clusters[i].sum_sq = 0;

        // 检查地址是否超出范围
        if (addr > 250) {
            break;
        }
    }
    
    SCI_Printf("加载完成，聚类总数: %d\n", oc->cluster_count);
    return 1;
}



// // 从EEPROM加载聚类数据
// uint8 online_cluster_load_from_eeprom(online_cluster_t* oc) {
//     uint16 addr = CLUSTER_DATA_START_ADDR;
//     uint8 i;
//     // 读取配置信息
//     oc->cluster_count = AT24CXX_ReadData(CLUSTER_COUNT_ADDR);
//     oc->margin = read_uint16_from_eeprom(CLUSTER_MARGIN_ADDR1);
//     oc->merge_threshold = read_uint16_from_eeprom(CLUSTER_MERGE_THRESH_ADDR1);

//     // 限制聚类数量，防止数据损坏
//     if (oc->cluster_count > oc->max_clusters) {
//         oc->cluster_count = oc->max_clusters;
//     }

//     // 读取每个聚类的数据
//     for (i = 0; i < oc->cluster_count; i++) {
//         oc->clusters[i].min_val = read_uint16_from_eeprom(addr);
//         addr += 2;
//         oc->clusters[i].max_val = read_uint16_from_eeprom(addr);
//         addr += 2;
//         oc->clusters[i].count = read_uint16_from_eeprom(addr);
//         addr += 2;
//         oc->clusters[i].sum = read_uint32_from_eeprom(addr);
//         addr += 4;

//         // 重新计算sum_sq（近似值）
//         if (oc->clusters[i].count > 0) {
//             uint16 center = (uint16)(oc->clusters[i].sum / oc->clusters[i].count);
//             oc->clusters[i].sum_sq = (uint32)center * center * oc->clusters[i].count;
//         }

//         // 检查地址是否超出范围
//         if (addr > 250) {
//             break;
//         }
//     }

//     return 1;  // 成功加载
// }

// // 清除EEPROM中的聚类数据
// void online_cluster_clear_eeprom(void) {
//     AT24CXX_WriteData(CLUSTER_COUNT_ADDR, 0);
//     write_uint16_to_eeprom(CLUSTER_MARGIN_ADDR1, 0);
//     write_uint16_to_eeprom(CLUSTER_MERGE_THRESH_ADDR1, 0);
//     uint8 i;
//     // 清除聚类数据区域
//     for (i = 0; i < 50; i++) {
//         AT24CXX_WriteData(CLUSTER_DATA_START_ADDR + i, 0);
//     }
// }
// 清除EEPROM中的聚类数据
void online_cluster_clear_eeprom(void) {
    AT24CXX_WriteData(CLUSTER_COUNT_ADDR, 0);
    uint8 i;
    // 清除聚类边界数据区域
    for (i = 0; i < 30; i++) {
        AT24CXX_WriteData(CLUSTER_DATA_START_ADDR + i, 0);
    }
}
// 异常检测函数
// uint8 online_cluster_is_abnormal(online_cluster_t* oc, float test_value_float) {
//     // 转换为uint16
//     uint16 test_value;
//     uint8 i;
//     if (test_value_float < 0) test_value_float = 0;
//     if (test_value_float > 1500) test_value_float = 1500;
//     test_value = (uint16)(test_value_float + 0.5f);

//     uint16 ranges_min[6], ranges_max[6];
//     uint8 range_count = online_cluster_get_ranges(oc, ranges_min, ranges_max, 6);

//     // 检查是否在任何正常范围内
//     for (i = 0; i < range_count; i++) {
//         if (test_value >= ranges_min[i] && test_value <= ranges_max[i]) {
//             return 0;  // 正常
//         }
//     }

//     return 1;  // 异常
// }

// 异常检测函数 - 直接使用cluster_count
uint8 online_cluster_is_abnormal(online_cluster_t* oc, float test_value_float) {
    // 转换为uint16
    uint16 test_value;
    uint8 i;
    if (test_value_float < 0) test_value_float = 0;
    if (test_value_float > 1500) test_value_float = 1500;
    test_value = (uint16)(test_value_float + 0.5f);

    // 直接使用cluster_count作为循环上限
    for (i = 0; i < oc->cluster_count; i++) {
        // 计算当前聚类的范围（考虑边界扩展）
        uint16 range_min = (oc->clusters[i].min_val > oc->margin) ?
                          (oc->clusters[i].min_val - oc->margin) : 0;
        uint16 range_max = (oc->clusters[i].max_val + oc->margin < 1500) ?
                          (oc->clusters[i].max_val + oc->margin) : 1500;
        
        // 检查是否在当前聚类的范围内
        if (test_value >= range_min && test_value <= range_max) {
            return 0;  // 正常：在某个聚类范围内
        }
    }

    return 1;  // 异常：不在任何聚类范围内
}


