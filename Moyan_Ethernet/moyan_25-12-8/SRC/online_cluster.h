// /*
//  * online_cluster.h
//  *
//  *  Created on: 2025年11月13日
//  *      Author: 20107
//  */

// #ifndef ONLINE_CLUSTER_H_
// #define ONLINE_CLUSTER_H_


// #include "types.h"  // 包含您的类型定义

// // 聚类结构体 - 使用uint16类型
// typedef struct {
//     uint16 min_val;
//     uint16 max_val;
//     uint16 count;
//     uint32 sum;
//     uint32 sum_sq;
// } cluster_t;

// // 在线聚类器结构体
// typedef struct {
//     cluster_t clusters[6];  // 减少到6个聚类以节省空间
//     uint8 cluster_count;
//     uint16 merge_threshold;
//     uint8 min_cluster_size;
//     uint8 max_clusters;
//     uint16 margin;  // 边界扩展值
// } online_cluster_t;

// // EEPROM存储地址定义


// // 函数声明
// void online_cluster_init(online_cluster_t* oc, uint16 merge_threshold,
//                         uint8 min_cluster_size, uint16 margin);
// void online_cluster_update(online_cluster_t* oc, float new_value_float);
// uint8 online_cluster_get_ranges(online_cluster_t* oc,
//                               uint16* ranges_min, uint16* ranges_max,
//                               uint8 max_ranges);

// // EEPROM存储相关函数
// void online_cluster_save_to_eeprom(online_cluster_t* oc);
// uint8 online_cluster_load_from_eeprom(online_cluster_t* oc);
// void online_cluster_clear_eeprom(void);

// // 异常检测函数
// uint8 online_cluster_is_abnormal(online_cluster_t* oc, float test_value_float);


// #endif /* ONLINE_CLUSTER_H_ */



#ifndef ONLINE_CLUSTER_H
#define ONLINE_CLUSTER_H
#define Max_clusters 10 //结构体cluster的长度和初始化最大聚类数oc->max_clusters
#include "types.h"

// 聚类结构体 - 简化版
typedef struct {
    uint16 min_val;
    uint16 max_val;
    uint16 count;
    // uint32 sum;
    // uint32 sum_sq;
} cluster_t;

// 在线聚类器结构体
typedef struct {
    cluster_t clusters[Max_clusters];
    uint8 cluster_count;
    uint16 merge_threshold;
    uint8 min_cluster_size;
    uint8 max_clusters;
    uint16 margin;
    uint8 learning_mode;  // 实现断电重启仍保留学习/检测状态
} online_cluster_t;



// 函数声明
void online_cluster_init(online_cluster_t* oc, uint16 merge_threshold,
                        uint8 min_cluster_size, uint16 margin);
void online_cluster_update(online_cluster_t* oc, float new_value_float);
uint8 online_cluster_get_ranges(online_cluster_t* oc,
                              uint16* ranges_min, uint16* ranges_max,
                              uint8 max_ranges);

// EEPROM存储相关函数
void online_cluster_save_to_eeprom(online_cluster_t* oc);
uint8 online_cluster_load_from_eeprom(online_cluster_t* oc);
void online_cluster_clear_eeprom(void);

// 异常检测函数
uint8 online_cluster_is_abnormal(online_cluster_t* oc, float test_value_float);

uint8 online_cluster_get_learning_mode(online_cluster_t* oc);  // ← 添加这一行
extern online_cluster_t oc;
extern uint16 judge_nearest_dist;
extern uint8 judge_nearest_idx;
#endif
