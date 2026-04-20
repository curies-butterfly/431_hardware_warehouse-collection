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
// uint8 merged;
// uint16 min_dist;
// uint16 center_i, center_j;
// uint16 distance;

// uint8 write_idx;
//uint8 range_count;
online_cluster_t oc;
uint16 judge_nearest_dist;//返回异常时的偏移最近中心的距离，全局变量为了方便传输和调用
uint8 judge_nearest_idx;//返回异常时的偏移最近中心点
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

// 在文件末尾添加这个新函数：
uint8 online_cluster_get_learning_mode(online_cluster_t* oc) {
    return oc->learning_mode;
}
// ==================== EEPROM存储功能 ====================

// 初始化聚类器
void online_cluster_init(online_cluster_t* oc, uint16 merge_threshold,
                        uint8 min_cluster_size, uint16 margin) {
    oc->merge_threshold = merge_threshold;
    oc->min_cluster_size = min_cluster_size;
    oc->max_clusters = Max_clusters;//设置最大聚类数,从h文件的这个Max量改
    oc->margin = margin;
    
    // 先尝试从EEPROM加载聚类数据
    uint8 cluster_count = AT24CXX_ReadData(CLUSTER_COUNT_ADDR);
    
    if (cluster_count > 0 && cluster_count <= oc->max_clusters) {
        // 有已有聚类数据，在此基础上继续学习
        SCI_Printf("检测到已有聚类数据%d个，在此基础上继续学习\n", cluster_count);
        online_cluster_load_from_eeprom(oc);  // 加载完整数据
    } else {
        // 没有聚类数据或数据异常，重新初始化
        SCI_Printf("无有效聚类数据，重新初始化\n");
        oc->cluster_count = 0;
        uint8 i;
        for (i = 0; i < oc->max_clusters; i++) {
            oc->clusters[i].count = 0;
            // oc->clusters[i].sum = 0;
            // oc->clusters[i].sum_sq = 0;
        }
    }
    
    // 加载学习模式状态
    uint8 saved_mode = AT24CXX_ReadData(LEARNING_MODE_ADDR);
    if (saved_mode == 0xFF || (saved_mode != 0x00 && saved_mode != 0x01)) {
        oc->learning_mode = 1;  // 默认开启学习模式
        SCI_Printf("EEPROM学习标志未写入或异常，默认进入学习模式: %d\n", oc->learning_mode);
    } else {
        oc->learning_mode = saved_mode;
        SCI_Printf("加载学习模式: %d\n", oc->learning_mode);
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
            center = (oc->clusters[i].min_val + oc->clusters[i].max_val) / 2;
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
    // cluster->sum += value;
    // cluster->sum_sq += (uint32)value * value;
}

// 合并两个聚类
static void combine_clusters(cluster_t* dest, cluster_t* src) {
    if (src->min_val < dest->min_val) dest->min_val = src->min_val;
    if (src->max_val > dest->max_val) dest->max_val = src->max_val;
    dest->count += src->count;
    // dest->sum += src->sum;
    // dest->sum_sq += src->sum_sq;
}

// 合并相近的聚类
static uint8 merge_close_clusters(online_cluster_t* oc) {
    if (oc->cluster_count < 2) return 0;

    uint8 merged_count = 0;
    uint8 merged;  // 局部变量
    uint16 center_i, center_j, distance;  // 局部变量    
    ServiceDog();		//喂狗
    SCI_Printf("开始合并聚类计算...");
    do {
        merged = 0;
        uint8 i,j;
        for (i = 0; i < oc->cluster_count && !merged; i++) {
            for (j = i + 1; j < oc->cluster_count && !merged; j++) {
                //之前转过(uint16)
                center_i = (oc->clusters[i].min_val + oc->clusters[i].max_val) / 2;
                center_j = (oc->clusters[j].min_val + oc->clusters[j].max_val) / 2;

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
    SCI_Printf("合并计算完成...");
    return merged_count;
}


// 更新聚类器 - 移除清理过小聚类的逻辑1113
void online_cluster_update(online_cluster_t* oc, float new_value_float) {
    // float转uint16，四舍五入，限制在0-60000范围内
    uint16 min_dist;  // 改为局部变量
    uint16 new_value;
    uint8 nearest_idx;
    if (new_value_float < 0) new_value_float = 0;
    if (new_value_float > 60000) new_value_float = 60000;
    new_value = (uint16)(new_value_float + 0.5f);
    
    ServiceDog();		//喂狗
    SCI_Printf("学习新数据%d", new_value);

    // 如果还没有聚类，直接创建第一个
    if (oc->cluster_count == 0) {
        oc->clusters[0].min_val = new_value;
        oc->clusters[0].max_val = new_value;
        oc->clusters[0].count = 1;
        // oc->clusters[0].sum = new_value;
        // oc->clusters[0].sum_sq = (uint32)new_value * new_value;
        oc->cluster_count = 1;
        return;
    }

    // 寻找最近的聚类  nearest_idx为最近的簇id
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
            // oc->clusters[oc->cluster_count].sum = new_value;
            // oc->clusters[oc->cluster_count].sum_sq = (uint32)new_value * new_value;
            oc->cluster_count++;
        } else {
            // 没有空间时，合并到最近的聚类
            nearest_idx = find_nearest_cluster(oc, new_value, &min_dist);
            merge_to_cluster(&oc->clusters[nearest_idx], new_value);
        }
    }

    // 移除清理过小聚类的逻辑
    // online_cluster_cleanup(oc);

    // 保留合并相近聚类的逻辑
    merge_close_clusters(oc);
    ServiceDog();		//喂狗
}




// 保存聚类数据到EEPROM - 只保存聚类数和边界
void online_cluster_save_to_eeprom(online_cluster_t* oc) {
    uint16 addr = CLUSTER_DATA_START_ADDR;

	ServiceDog();		//喂狗
    // 1. 保存总聚类数
    AT24CXX_WriteData(CLUSTER_COUNT_ADDR, oc->cluster_count);
    
    // 2. 保存学习模式状态 
    AT24CXX_WriteData(LEARNING_MODE_ADDR, oc->learning_mode);    
    uint8 i;
    // 3. 保存每个聚类的上下限
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
// 从EEPROM加载聚类数据 - 恢复合理的统计信息
uint8 online_cluster_load_from_eeprom(online_cluster_t* oc) {
    uint16 addr = CLUSTER_DATA_START_ADDR;
    uint8 i;
    
    ServiceDog();		//喂狗
    // 1. 读取总聚类数
    oc->cluster_count = AT24CXX_ReadData(CLUSTER_COUNT_ADDR);
    
    // 限制聚类数量，防止数据损坏
    if (oc->cluster_count > oc->max_clusters) {
        oc->cluster_count = oc->max_clusters;
    }
    // 2. 读取当前学习状态  为自动切换作保障 
    oc->learning_mode = AT24CXX_ReadData(LEARNING_MODE_ADDR);
    // 3. 读取每个聚类的上下限并恢复统计信息
    for (i = 0; i < oc->cluster_count; i++) {
        // 读取边界数据
        oc->clusters[i].min_val = read_uint16_from_eeprom(addr);
        addr += 2;
        oc->clusters[i].max_val = read_uint16_from_eeprom(addr);
        addr += 2;
        
        // 关键：恢复合理的统计信息！
        uint16 estimated_center = (oc->clusters[i].min_val + oc->clusters[i].max_val) / 2;
        uint16 range_size = oc->clusters[i].max_val - oc->clusters[i].min_val;
        
        // 根据范围大小估算合理的数据点数
        // 范围越大，说明包含的数据点可能越多
        if (range_size <= 20) {
            oc->clusters[i].count = 3;  // 小范围，数据点较少
        } else if (range_size <= 50) {
            oc->clusters[i].count = 5;  // 中等范围
        } else {
            oc->clusters[i].count = 8;  // 大范围，数据点较多
        }
        
        // // 计算合理的sum和sum_sq
        // oc->clusters[i].sum = estimated_center * oc->clusters[i].count;
        // oc->clusters[i].sum_sq = (uint32)estimated_center * estimated_center * oc->clusters[i].count;
        
        SCI_Printf("恢复聚类%d: [%d,%d] count=%d center=%d\n", 
                   i, oc->clusters[i].min_val, oc->clusters[i].max_val,
                   oc->clusters[i].count, estimated_center);

        // 检查地址是否超出范围
        if (addr > 250) {
            SCI_Printf("溢出");
            break;
        }
    }
    
    SCI_Printf("加载完成，聚类总数: %d\n", oc->cluster_count);
    return 1;
}




void online_cluster_clear_eeprom(void) {
    AT24CXX_WriteData(CLUSTER_COUNT_ADDR, 0);
    AT24CXX_WriteData(LEARNING_MODE_ADDR, 0);  
    uint8 i;
    // 清除聚类边界数据区域
    for (i = 0; i < 30; i++) {
        AT24CXX_WriteData(CLUSTER_DATA_START_ADDR + i, 0);
    }
}


// 异常检测函数 - 直接使用cluster_count
uint8 online_cluster_is_abnormal(online_cluster_t* oc, float test_value_float) {
    // 转换为uint16
    uint16 test_value;
    uint8 i;

    uint16 min_dist;  // 局部变量和updata取名一样

    if (test_value_float < 0) test_value_float = 0;
    if (test_value_float > 60000) test_value_float = 60000;
    test_value = (uint16)(test_value_float + 0.5f);
    
    ServiceDog();		//喂狗
    // 直接使用cluster_count作为循环上限
    for (i = 0; i < oc->cluster_count; i++) {
        // 计算当前聚类的范围（考虑边界扩展）
        uint16 range_min = (oc->clusters[i].min_val > oc->margin) ?
                          (oc->clusters[i].min_val - oc->margin) : 0;
        uint16 range_max = (oc->clusters[i].max_val + oc->margin < 60000) ?
                          (oc->clusters[i].max_val + oc->margin) : 60000;
        
        // 检查是否在当前聚类的范围内
        if (test_value >= range_min && test_value <= range_max) {
            return i;  // 正常：在某个聚类范围内
        }
    }
    judge_nearest_idx = find_nearest_cluster(oc, test_value, &min_dist);
    judge_nearest_dist = ((oc->clusters[judge_nearest_idx].max_val+oc->clusters[judge_nearest_idx].min_val)/2 > test_value) ? ((oc->clusters[judge_nearest_idx].max_val+oc->clusters[judge_nearest_idx].min_val)/2 - test_value) : (test_value-(oc->clusters[judge_nearest_idx].max_val+oc->clusters[judge_nearest_idx].min_val)/2);
    ServiceDog();		//喂狗    
        
    return -1;  // 异常：不在任何聚类范围内
}





