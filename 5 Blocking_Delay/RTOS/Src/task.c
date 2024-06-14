#include "task.h"

#define confgiMINIMAL_STACK_SIZE 128

StackType_t	IdleTasKStack[confgiMINIMAL_STACK_SIZE];

// 滴答定时器计数值
static volatile TickType_t xTickCount = (TickType_t)0U;

// 就绪链表
List_t pxReadyTasksLists;

// 当前 TCB 指针
TCB_t volatile *pxCurrentTCB = NULL;

// 空闲任务参数
TCB_t IdleTaskTCB;


// 在 main.c 中定义的两个任务声明
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;

// 使用的外部函数声明
extern StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
                                          TaskFunction_t pxCode, 
                                          void* pvParameters);
extern BaseType_t xPortStartScheduler(void);		

// 就绪列表初始化函数
void prvInitialiseTaskLists(void)
{
	vListInitialise(&pxReadyTasksLists);
}

// 空闲任务函数体
void prvIdleTask(void *p_arg)
{
    for(;;){}
}																		

// 真正的创建任务函数																 
static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,    // 任务函数
                            const char* const pcName,          // 任务名称
                            const uint32_t ulStackDepth,       // 任务栈深度
                            void* const pvParameters,          // 任务参数
                            TaskHandle_t* const pxCreatedTask, // 任务句柄
                            TCB_t* pxNewTCB)                   // 任务栈控制指针
{
	StackType_t *pxTopOfStack;
	UBaseType_t x;
	
	// 栈顶指针，本质是分配的一段空间的最低地址
	pxTopOfStack = pxNewTCB->pxStack + (ulStackDepth - (uint32_t)1);
	// 8 字节对齐
	pxTopOfStack = (StackType_t*)(((uint32_t)pxTopOfStack) 
	                             & (~((uint32_t)0x0007)));
	// 保存任务名称到TCB中
	for(x = (UBaseType_t)0;x < (UBaseType_t)configMAX_TASK_NAME_LEN;x++)
	{
		pxNewTCB->pcTaskName[x] = pcName[x];
		if(pcName[x] == 0x00)
			break;
	}
	pxNewTCB->pcTaskName[configMAX_TASK_NAME_LEN-1] = '\0';
	
	// 初始化链表项
	vListInitialiseItem(&(pxNewTCB->xStateListItem));
	
	// 设置该链表项的拥有者为 pxNewTCB
	listSET_LIST_ITEM_OWNER(&(pxNewTCB->xStateListItem), pxNewTCB);
	
	// 初始化任务栈
	pxNewTCB->pxTopOfStack = 
	          pxPortInitialiseStack(pxTopOfStack, pxTaskCode, pvParameters);
	
	if((void*)pxCreatedTask != NULL)
	{
	    *pxCreatedTask = (TaskHandle_t)pxNewTCB;
	}
}

// 静态创建任务函数
#if (configSUPPORT_STATIC_ALLOCATION == 1)
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,     // 任务函数
                            const char* const pcName,         // 任务名称
                            const uint32_t ulStackDepth,      // 任务栈深度
                            void* const pvParameters,         // 任务参数
                            StackType_t* const puxTaskBuffer, // 任务栈起始指针
                            TCB_t* const pxTaskBuffer)        // 任务栈控制指针
{
	TCB_t* pxNewTCB;
	TaskHandle_t xReturn;
	// 任务栈控制指针和任务栈起始指针不为空
	if((pxTaskBuffer != NULL) && (puxTaskBuffer != NULL))
	{
		pxNewTCB = (TCB_t*)pxTaskBuffer;
		pxNewTCB->pxStack = (StackType_t*)puxTaskBuffer;
		
		// 真正的创建任务函数
		prvInitialiseNewTask(pxTaskCode,
							 pcName,
							 ulStackDepth,
							 pvParameters,
							 &xReturn,
							 pxNewTCB);
	}
	else
	{
		xReturn = NULL;
	}
	// 任务创建成功后应该返回任务句柄，否则返回 NULL
	return xReturn;
}
#endif

// 启动任务调度器
void vTaskStartScheduler(void)
{
	// 创建空闲任务
	TaskHandle_t xIdleTaskHandle = xTaskCreateStatic((TaskFunction_t)prvIdleTask,
										(char *)"IDLE",
										(uint32_t)confgiMINIMAL_STACK_SIZE,
										(void *)NULL,
										(StackType_t *)IdleTasKStack,
										(TCB_t *)&IdleTaskTCB);
	// 将空闲任务插入到就绪链表中
	vListInsertEnd(&(pxReadyTasksLists), 
				         &(((TCB_t *)(&IdleTaskTCB))->xStateListItem));

	pxCurrentTCB = &Task1TCB;
	if(xPortStartScheduler() != pdFALSE){}
}

// 任务调度函数
void vTaskSwitchContext(void)
{
    if(pxCurrentTCB == &IdleTaskTCB)
    {
        if(Task1TCB.xTicksToDelay == 0)
        {
            pxCurrentTCB = &Task1TCB;
        }
        else if(Task2TCB.xTicksToDelay == 0)
        {
            pxCurrentTCB = &Task2TCB;
        }
        else
        {
            return;
        }
    }
    else
    {
        if(pxCurrentTCB == &Task1TCB)
        {
            if(Task2TCB.xTicksToDelay == 0)
            {
                pxCurrentTCB = &Task2TCB;
            }
            else if(pxCurrentTCB->xTicksToDelay != 0)
            {
                pxCurrentTCB = &IdleTaskTCB;
            }
            else
            {
                return;
            }
        }
        else if(pxCurrentTCB == &Task2TCB)
        {
            if(Task1TCB.xTicksToDelay == 0)
            {
                pxCurrentTCB = &Task1TCB;
            }
            else if(pxCurrentTCB->xTicksToDelay != 0)
            {
                pxCurrentTCB = &IdleTaskTCB;
            }
            else
            {
                return;
            }
        }
    }
}

// 阻塞延时函数
void vTaskDelay(const TickType_t xTicksToDelay)
{
    TCB_t *pxTCB = NULL;

    // 获取当前要延时的任务 TCB
    pxTCB = (TCB_t *)pxCurrentTCB;
    // 记录延时时间
    pxTCB->xTicksToDelay = xTicksToDelay;
    // 主动产生任务调度，让出 MCU 
    taskYIELD();
}

// 更新任务延时参数
void xTaskIncrementTick(void)
{
	TCB_t *pxTCB = NULL;
	ListItem_t *pxListItem = NULL;
	List_t *pxList = &pxReadyTasksLists;
	uint8_t xSwitchRequired = pdFALSE;
	
	// 更新 xTickCount 系统时基计数器
	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;
	
	// 检查就绪链表是否为空
	if(listLIST_IS_EMPTY(pxList) == pdFALSE) 
	{
		// 不为空获取链表头链表项
		pxListItem = listGET_HEAD_ENTRY(pxList);

		// 迭代就绪链表所有链表项
		while(pxListItem != (ListItem_t *)&(pxList->xListEnd)) 
		{
			// 获取每个链表项的任务控制块 TCB
			pxTCB = (TCB_t *)listGET_LIST_ITEM_OWNER(pxListItem);
			
			// 延时参数递减
			if(pxTCB->xTicksToDelay > 0){
				pxTCB->xTicksToDelay--;
			}
			else{
				xSwitchRequired = pdTRUE;
			}
			// 移动到下一个链表项
			pxListItem = listGET_NEXT(pxListItem);
		}
	}
	// 如果就绪链表中有任务从阻塞状态恢复就产生任务调度
	if(xSwitchRequired == pdTRUE){
		// 产生任务调度
		taskYIELD();
	}
}
