#ifndef TASK_H
#define TASK_H

#include "FreeRTOS.h"

/* task.h */
// 任务控制块
typedef struct tskTaskControlBlock
{
    volatile StackType_t  *pxTopOfStack;                        // 栈顶
    ListItem_t            xStateListItem;                       // 任务节点
    StackType_t           *pxStack;                             // 任务栈起始地址
    char                  pcTaskName[configMAX_TASK_NAME_LEN];  // 任务名称
}tskTCB;
typedef tskTCB TCB_t;



// 任务句柄指针
typedef void* TaskHandle_t;
// 任务函数指针
typedef void (*TaskFunction_t)(void *);

// 主动产生任务调度
#define taskYIELD() portYIELD()


#define taskENTER_CRITICAL()           portENTER_CRITICAL()
#define taskEXIT_CRITICAL()            portEXIT_CRITICAL()

#define taskENTER_CRITICAL_FROM_ISR()  portSET_INTERRUPT_MASK_FROM_ISR()
#define taskEXIT_CRITICAL_FROM_ISR(x)  portCLEAR_INTERRUPT_MASK_FROM_ISR(x)


// 函数声明
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,
                            const char* const pcName,
                            const uint32_t ulStackDepth,
                            void* const pvParameters,
                            StackType_t* const puxTaskBuffer,
                            TCB_t* const pxTaskBuffer);
														
void prvInitialiseTaskLists(void);
void vTaskStartScheduler(void);
void vTaskSwitchContext(void);




#endif //TASK_H

