#ifndef TASK_H
#define TASK_H

#include "FreeRTOS.h"

/* task.h */
// ������ƿ�
typedef struct tskTaskControlBlock
{
    volatile StackType_t  *pxTopOfStack;                        // ջ��
    ListItem_t            xStateListItem;                       // ����ڵ�
    StackType_t           *pxStack;                             // ����ջ��ʼ��ַ
    char                  pcTaskName[configMAX_TASK_NAME_LEN];  // ��������
}tskTCB;
typedef tskTCB TCB_t;



// ������ָ��
typedef void* TaskHandle_t;
// ������ָ��
typedef void (*TaskFunction_t)(void *);

// ���������������
#define taskYIELD() portYIELD()


#define taskENTER_CRITICAL()           portENTER_CRITICAL()
#define taskEXIT_CRITICAL()            portEXIT_CRITICAL()

#define taskENTER_CRITICAL_FROM_ISR()  portSET_INTERRUPT_MASK_FROM_ISR()
#define taskEXIT_CRITICAL_FROM_ISR(x)  portCLEAR_INTERRUPT_MASK_FROM_ISR(x)


// ��������
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

