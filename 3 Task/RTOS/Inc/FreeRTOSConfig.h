// XXX 替换为对应头文件名称
#ifndef FREERTOSCONFIG_H
#define FREERTOSCONFIG_H

#include "FreeRTOS.h"

/* FreeRTOSConfig.h */
// 设置 TickType_t 类型位 16 位 
#define configUSE_16_BIT_TICKS                  0
// 任务名称字符串长度
#define configMAX_TASK_NAME_LEN                 24
// 是否支持静态方式创建任务
#define configSUPPORT_STATIC_ALLOCATION         1
// 设置内核中断优先级（最低优先级）
#define configKERNEL_INTERRUPT_PRIORITY         15
// 设置内核可管理的最大中断优先级
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    11<<4


#endif //FREERTOSCONFIG_H

