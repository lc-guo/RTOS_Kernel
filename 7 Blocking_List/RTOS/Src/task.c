#include "task.h"

// 就绪链表数组
List_t pxReadyTasksLists[configMAX_PRIORITIES];

// 当前 TCB 指针
TCB_t volatile *pxCurrentTCB = NULL;

// 空闲任务参数
TCB_t IdleTaskTCB;
#define confgiMINIMAL_STACK_SIZE 128
StackType_t	IdleTasKStack[confgiMINIMAL_STACK_SIZE];

// 滴答定时器计数值
static volatile TickType_t xTickCount = (TickType_t)0U;

// 全局任务计数器
static volatile UBaseType_t uxCurrentNumberOfTasks = (UBaseType_t)0U;
static UBaseType_t uxTaskNumber = (UBaseType_t)0U;

// 32位的优先级位图，默认全 0 ，记录了所有存在的优先级
static volatile UBaseType_t uxTopReadyPriority = 0;

// 阻塞链表和其指针
static List_t xDelayed_Task_List1;
static List_t volatile *pxDelayed_Task_List;

// 溢出阻塞链表和其指针
static List_t xDelayed_Task_List2;
static List_t volatile *pxOverflow_Delayed_Task_List;

// 记录溢出次数
static volatile BaseType_t xNumOfOverflows = (BaseType_t)0;

// 记录下个任务解除阻塞时间
static volatile TickType_t xNextTaskUnblockTime = (TickType_t)portMAX_DELAY;

// 在 main.c 中定义的两个任务声明
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;

// 使用的外部函数声明
extern StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
                                          TaskFunction_t pxCode, 
                                          void* pvParameters);
extern BaseType_t xPortStartScheduler(void);
static void prvResetNextTaskUnblockTime(void);

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

// 延时阻塞链表和溢出延时阻塞链表交换
#define taskSWITCH_DELAYED_LISTS()\
{\
	List_t volatile *pxTemp;\
	pxTemp = pxDelayed_Task_List;\
	pxDelayed_Task_List = pxOverflow_Delayed_Task_List;\
	pxOverflow_Delayed_Task_List = pxTemp;\
	xNumOfOverflows++;\
	prvResetNextTaskUnblockTime();\
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
	
	// 初始化延时阻塞链表
	vListInitialise(&xDelayed_Task_List1);
	vListInitialise(&xDelayed_Task_List2);
	
	// 初始化指向延时阻塞链表的指针
	pxDelayed_Task_List = &xDelayed_Task_List1;
	pxOverflow_Delayed_Task_List = &xDelayed_Task_List2;
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

// 将当前任务添加到阻塞链表中
static void prvAddCurrentTaskToDelayedList(TickType_t xTicksToWait)
{
	TickType_t xTimeToWake;
	// 当前滴答定时器中断次数
	const TickType_t xConstTickCount = xTickCount;
	// 成功从就绪链表中移除该阻塞任务
	if(uxListRemove((ListItem_t *)&(pxCurrentTCB->xStateListItem)) == 0)
	{
		// 将当前任务的优先级从优先级位图中删除
		portRESET_READY_PRIORITY(pxCurrentTCB->uxPriority, uxTopReadyPriority);
	}
	// 计算延时到期时间
	xTimeToWake = xConstTickCount + xTicksToWait;
	// 将延时到期值设置为阻塞链表中节点的排序值
	listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xStateListItem), xTimeToWake);
	// 如果延时到期时间会溢出
	if(xTimeToWake < xConstTickCount)
	{
		// 将其插入溢出阻塞链表中
		vListInsert((List_t *)pxOverflow_Delayed_Task_List,
		           (ListItem_t *)&(pxCurrentTCB->xStateListItem));
	}
	// 没有溢出
	else
	{
		// 插入到阻塞链表中
		vListInsert((List_t *)pxDelayed_Task_List,
		           (ListItem_t *) &( pxCurrentTCB->xStateListItem));
		
		// 更新下一个任务解锁时刻变量 xNextTaskUnblockTime 的值
		if(xTimeToWake < xNextTaskUnblockTime)
		{
			xNextTaskUnblockTime = xTimeToWake;
		}
	}
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
							 uxPriority,
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
	// 初始化滴答定时器计数值
	xTickCount = (TickType_t)0U;
	
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
	// 将当前任务加入到阻塞链表
	prvAddCurrentTaskToDelayedList(xTicksToDelay);
	
	// 任务切换
	taskYIELD();
}

/* task.c */
// 任务阻塞延时处理
BaseType_t xTaskIncrementTick(void)
{
	TCB_t *pxTCB = NULL;
	TickType_t xItemValue;
	BaseType_t xSwitchRequired = pdFALSE;
	
	// 更新系统时基计数器 xTickCount
	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;

	// 如果 xConstTickCount 溢出，则切换延时列表
	if(xConstTickCount == (TickType_t)0U)
	{
		taskSWITCH_DELAYED_LISTS();
	}
	
	// 最近的延时任务延时到期
	if(xConstTickCount >= xNextTaskUnblockTime)
	{
		for(;;)
		{
			// 延时阻塞链表为空则跳出 for 循环
			if(listLIST_IS_EMPTY(pxDelayed_Task_List) != pdFALSE)
			{
				// 设置下个任务解除阻塞时间为最大值，也即永不解除阻塞
				xNextTaskUnblockTime = portMAX_DELAY;
				break;
			}
			else
			{
				// 依次获取延时阻塞链表头节点
				pxTCB=(TCB_t *)listGET_OWNER_OF_HEAD_ENTRY(pxDelayed_Task_List);
				// 依次获取延时阻塞链表中所有节点解除阻塞的时间
				xItemValue = listGET_LIST_ITEM_VALUE(&(pxTCB->xStateListItem));
				
				// 当阻塞链表中所有延时到期的任务都被移除则跳出 for 循环
				if(xConstTickCount < xItemValue)
				{
					xNextTaskUnblockTime = xItemValue;
					break;
				}
				
				// 将任务从延时列表移除，消除等待状态
				(void)uxListRemove(&(pxTCB->xStateListItem));
				
				// 将解除等待的任务添加到就绪列表
				prvAddTaskToReadyList(pxTCB);
#if(configUSE_PREEMPTION == 1)
				// 如果解除阻塞状态的任务优先级比当前任务优先级高，则需要进行任务调度
				if(pxTCB->uxPriority >= pxCurrentTCB->uxPriority)
				{
					xSwitchRequired = pdTRUE;
				}
#endif
			}
		}
	}
	return xSwitchRequired;
}

// 重设 xNextTaskUnblockTime 变量值
static void prvResetNextTaskUnblockTime(void)
{
	TCB_t *pxTCB;
	// 切换阻塞链表后，阻塞链表为空
	if(listLIST_IS_EMPTY(pxDelayed_Task_List) != pdFALSE )
	{
		// 下次解除延时的时间为可能的最大值
		xNextTaskUnblockTime = portMAX_DELAY;
	}
	else
	{
		// 如果阻塞链表不为空，下次解除延时的时间为链表头任务的阻塞时间
		(pxTCB) = (TCB_t *)listGET_OWNER_OF_HEAD_ENTRY(pxDelayed_Task_List);
		xNextTaskUnblockTime=listGET_LIST_ITEM_VALUE(&((pxTCB)->xStateListItem));
	}
}

