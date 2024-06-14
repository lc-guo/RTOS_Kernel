#include "task.h"

// 空闲任务参数
TCB_t IdleTaskTCB;
#define confgiMINIMAL_STACK_SIZE 128
StackType_t	IdleTasKStack[confgiMINIMAL_STACK_SIZE];

// 就绪链表数组
List_t pxReadyTasksLists[configMAX_PRIORITIES];

// 当前 TCB 指针
TCB_t volatile *pxCurrentTCB = NULL;


// 滴答定时器计数值
static volatile TickType_t xTickCount = (TickType_t)0U;

// 全局任务计数器
static volatile UBaseType_t uxCurrentNumberOfTasks = (UBaseType_t)0U;
static UBaseType_t uxTaskNumber = (UBaseType_t)0U;

// 32位的优先级位图，默认全 0 ，记录了所有存在的优先级
static volatile UBaseType_t uxTopReadyPriority = 0;

// 在 main.c 中定义的两个任务声明
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;

// 使用的外部函数声明
extern StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
                                          TaskFunction_t pxCode, 
                                          void* pvParameters);
extern BaseType_t xPortStartScheduler(void);

// 根据任务优先级置位优先级位图
#define taskRECORD_READY_PRIORITY(uxPriority)	portRECORD_READY_PRIORITY(uxPriority, uxTopReadyPriority)

// 根据任务优先级添加任务到对应的就绪链表
#define prvAddTaskToReadyList(pxTCB) \
	taskRECORD_READY_PRIORITY((pxTCB)->uxPriority); \
	vListInsertEnd(&(pxReadyTasksLists[(pxTCB)->uxPriority]), \
	               &((pxTCB)->xStateListItem)); \

// 找到就绪列表最高优先级的任务并更新到 pxCurrentTCB
#define taskSELECT_HIGHEST_PRIORITY_TASK() \
{ \
	UBaseType_t uxTopPriority; \
	/* 寻找最高优先级 */ \
	portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriority); \
	/* 获取优先级最高的就绪任务的 TCB，然后更新到 pxCurrentTCB */ \
	listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, \
	                            &(pxReadyTasksLists[uxTopPriority])); \
}

// 根据任务优先级清除优先级位图
#define taskRESET_READY_PRIORITY(uxPriority) \
{ \
	portRESET_READY_PRIORITY((uxPriority), (uxTopReadyPriority)); \
}

// 空闲任务函数体
void prvIdleTask(void *p_arg)
{
    for(;;){}
}

// 就绪链表始化函数
void prvInitialiseTaskLists(void)
{
	UBaseType_t uxPriority;
	// 初始化就绪任务链表
	for(uxPriority = (UBaseType_t)0U;
	    uxPriority < (UBaseType_t)configMAX_PRIORITIES; uxPriority++)
	{
		vListInitialise(&(pxReadyTasksLists[uxPriority]));
	}
}

// 添加任务到就绪链表中
static void prvAddNewTaskToReadyList(TCB_t* pxNewTCB)
{
	// 进入临界段
	taskENTER_CRITICAL();
	{
		// 全局任务计数器加一操作
		uxCurrentNumberOfTasks++;
			
		// 如果 pxCurrentTCB 为空，则将 pxCurrentTCB 指向新创建的任务
		if(pxCurrentTCB == NULL)
		{
			pxCurrentTCB = pxNewTCB;
			// 如果是第一次创建任务，则需要初始化任务相关的列表
			if(uxCurrentNumberOfTasks == (UBaseType_t)1)
			{
				// 初始化任务相关的列表
				prvInitialiseTaskLists();
			}
		}
		else 
		// 如果pxCurrentTCB不为空
		// 则根据任务的优先级将 pxCurrentTCB 指向最高优先级任务的 TCB 
		{
			if(pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority)
			{
				pxCurrentTCB = pxNewTCB;
			}
		}
		// 任务数量加一
		uxTaskNumber++;
		
		// 将任务添加到就绪列表
		prvAddTaskToReadyList(pxNewTCB);
	}
	// 退出临界段
	taskEXIT_CRITICAL();
}

// 真正的创建任务函数																 
static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,    // 任务函数
                            const char* const pcName,          // 任务名称
                            const uint32_t ulStackDepth,       // 任务栈深度
                            void* const pvParameters,          // 任务参数
														UBaseType_t uxPriority,            // 优先级
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

	// 初始化优先级
	if(uxPriority >= (UBaseType_t)configMAX_PRIORITIES)
	{
		uxPriority = (UBaseType_t)configMAX_PRIORITIES - (UBaseType_t)1U;
	}
	pxNewTCB->uxPriority = uxPriority;
	
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
														UBaseType_t uxPriority,           // 优先级
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
							 uxPriority,                      // 优先级
							 &xReturn,
							 pxNewTCB);
	
		// 创建完任务自动将任务添加到就绪链表
		prvAddNewTaskToReadyList(pxNewTCB);
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
                                      (UBaseType_t)taskIDLE_PRIORITY,
                                      (StackType_t *)IdleTasKStack,
                                      (TCB_t *)&IdleTaskTCB);
	
	if(xPortStartScheduler() != pdFALSE){}
}

// 任务调度函数
void vTaskSwitchContext(void)
{
	taskSELECT_HIGHEST_PRIORITY_TASK();
}

// 阻塞延时函数
void vTaskDelay(const TickType_t xTicksToDelay)
{
    TCB_t *pxTCB = NULL;

    // 获取当前要延时的任务 TCB
    pxTCB = (TCB_t *)pxCurrentTCB;
    // 记录延时时间
    pxTCB->xTicksToDelay = xTicksToDelay;
    // 将任务从优先级位图上清除，这样调度时就不会找到该任务
    taskRESET_READY_PRIORITY(pxTCB->uxPriority);
    // 主动产生任务调度，让出 MCU 
    taskYIELD();
}

// 更新任务延时参数
void xTaskIncrementTick(void)
{
	TCB_t *pxTCB = NULL;
	uint8_t i =0;
	uint8_t xSwitchRequired = pdFALSE;
	
	// 更新 xTickCount 系统时基计数器
	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;
	
	// 扫描就绪列表中所有任务,如果延时时间不为 0 则减 1 
	for(i=0; i<configMAX_PRIORITIES; i++)
	{
		pxTCB = (TCB_t *)listGET_OWNER_OF_HEAD_ENTRY((&pxReadyTasksLists[i]));
		if(pxTCB->xTicksToDelay > 0)
		{
			pxTCB->xTicksToDelay--;
		}
		// 延时时间到，将任务就绪
		else 
		{
			taskRECORD_READY_PRIORITY(pxTCB->uxPriority);
			xSwitchRequired = pdTRUE;
		}
	}
	// 如果就绪链表中有任务从阻塞状态恢复就产生任务调度
	if(xSwitchRequired == pdTRUE){
		// 产生任务调度
		portYIELD();
	}
}
