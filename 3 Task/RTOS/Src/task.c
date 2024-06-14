#include "task.h"

// 就绪链表
List_t pxReadyTasksLists;

// 当前 TCB 指针
TCB_t volatile *pxCurrentTCB = NULL;

// 在 main.c 中定义的两个任务声明
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;

// 使用的外部函数声明
extern StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
                                          TaskFunction_t pxCode, 
                                          void* pvParameters);
extern BaseType_t xPortStartScheduler(void);

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

// 就绪列表初始化函数
void prvInitialiseTaskLists(void)
{
	vListInitialise(&pxReadyTasksLists);
}

// 启动调度器函数
void vTaskStartScheduler(void)
{
	// 手动指定第一个运行的任务
	pxCurrentTCB = &Task1TCB;
	// 启动调度器
	if(xPortStartScheduler() != pdFALSE)
	{
		// 调度器启动成功则不会到这里
	}
}

// 任务调度函数
void vTaskSwitchContext(void)
{
	// 两个任务轮流切换
	if(pxCurrentTCB == &Task1TCB)
	{
		pxCurrentTCB = &Task2TCB;
	}
	else
	{
		pxCurrentTCB = &Task1TCB;
	}
}


