#include "task.h"

// ������������
List_t pxReadyTasksLists[configMAX_PRIORITIES];

// ��ǰ TCB ָ��
TCB_t volatile *pxCurrentTCB = NULL;

// �����������
TCB_t IdleTaskTCB;
#define confgiMINIMAL_STACK_SIZE 128
StackType_t	IdleTasKStack[confgiMINIMAL_STACK_SIZE];

// �δ�ʱ������ֵ
static volatile TickType_t xTickCount = (TickType_t)0U;

// ȫ�����������
static volatile UBaseType_t uxCurrentNumberOfTasks = (UBaseType_t)0U;
static UBaseType_t uxTaskNumber = (UBaseType_t)0U;

// 32λ�����ȼ�λͼ��Ĭ��ȫ 0 ����¼�����д��ڵ����ȼ�
static volatile UBaseType_t uxTopReadyPriority = 0;

// �����������ָ��
static List_t xDelayed_Task_List1;
static List_t volatile *pxDelayed_Task_List;

// ��������������ָ��
static List_t xDelayed_Task_List2;
static List_t volatile *pxOverflow_Delayed_Task_List;

// ��¼�������
static volatile BaseType_t xNumOfOverflows = (BaseType_t)0;

// ��¼�¸�����������ʱ��
static volatile TickType_t xNextTaskUnblockTime = (TickType_t)portMAX_DELAY;

// �� main.c �ж����������������
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;

// ʹ�õ��ⲿ��������
extern StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
                                          TaskFunction_t pxCode, 
                                          void* pvParameters);
extern BaseType_t xPortStartScheduler(void);
static void prvResetNextTaskUnblockTime(void);

// �����������ȼ���λ���ȼ�λͼ
#define taskRECORD_READY_PRIORITY(uxPriority)	portRECORD_READY_PRIORITY(uxPriority, uxTopReadyPriority)

// �����������ȼ�������񵽶�Ӧ�ľ�������
#define prvAddTaskToReadyList(pxTCB) \
	taskRECORD_READY_PRIORITY((pxTCB)->uxPriority); \
	vListInsertEnd(&(pxReadyTasksLists[(pxTCB)->uxPriority]), \
	               &((pxTCB)->xStateListItem)); \

// �ҵ������б�������ȼ������񲢸��µ� pxCurrentTCB
#define taskSELECT_HIGHEST_PRIORITY_TASK() \
{ \
	UBaseType_t uxTopPriority; \
	/* Ѱ��������ȼ� */ \
	portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriority); \
	/* ��ȡ���ȼ���ߵľ�������� TCB��Ȼ����µ� pxCurrentTCB */ \
	listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, \
	                            &(pxReadyTasksLists[uxTopPriority])); \
}

// �����������ȼ�������ȼ�λͼ
#define taskRESET_READY_PRIORITY(uxPriority) \
{ \
	portRESET_READY_PRIORITY((uxPriority), (uxTopReadyPriority)); \
}

// ��ʱ��������������ʱ����������
#define taskSWITCH_DELAYED_LISTS()\
{\
	List_t volatile *pxTemp;\
	pxTemp = pxDelayed_Task_List;\
	pxDelayed_Task_List = pxOverflow_Delayed_Task_List;\
	pxOverflow_Delayed_Task_List = pxTemp;\
	xNumOfOverflows++;\
	prvResetNextTaskUnblockTime();\
}

// ������������
void prvIdleTask(void *p_arg)
{
    for(;;){}
}

// ��������ʼ������
void prvInitialiseTaskLists(void)
{
	UBaseType_t uxPriority;
	// ��ʼ��������������
	for(uxPriority = (UBaseType_t)0U;
	    uxPriority < (UBaseType_t)configMAX_PRIORITIES; uxPriority++)
	{
		vListInitialise(&(pxReadyTasksLists[uxPriority]));
	}
	
	// ��ʼ����ʱ��������
	vListInitialise(&xDelayed_Task_List1);
	vListInitialise(&xDelayed_Task_List2);
	
	// ��ʼ��ָ����ʱ���������ָ��
	pxDelayed_Task_List = &xDelayed_Task_List1;
	pxOverflow_Delayed_Task_List = &xDelayed_Task_List2;
}

// ������񵽾���������
static void prvAddNewTaskToReadyList(TCB_t* pxNewTCB)
{
	// �����ٽ��
	taskENTER_CRITICAL();
	{
		// ȫ�������������һ����
		uxCurrentNumberOfTasks++;
			
		// ��� pxCurrentTCB Ϊ�գ��� pxCurrentTCB ָ���´���������
		if(pxCurrentTCB == NULL)
		{
			pxCurrentTCB = pxNewTCB;
			// ����ǵ�һ�δ�����������Ҫ��ʼ��������ص��б�
			if(uxCurrentNumberOfTasks == (UBaseType_t)1)
			{
				// ��ʼ��������ص��б�
				prvInitialiseTaskLists();
			}
		}
		else 
		// ���pxCurrentTCB��Ϊ��
		// �������������ȼ��� pxCurrentTCB ָ��������ȼ������ TCB 
		{
			if(pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority)
			{
				pxCurrentTCB = pxNewTCB;
			}
		}
		// ����������һ
		uxTaskNumber++;
		
		// ��������ӵ������б�
		prvAddTaskToReadyList(pxNewTCB);
	}
	// �˳��ٽ��
	taskEXIT_CRITICAL();
}

// ����ǰ������ӵ�����������
static void prvAddCurrentTaskToDelayedList(TickType_t xTicksToWait)
{
	TickType_t xTimeToWake;
	// ��ǰ�δ�ʱ���жϴ���
	const TickType_t xConstTickCount = xTickCount;
	// �ɹ��Ӿ����������Ƴ�����������
	if(uxListRemove((ListItem_t *)&(pxCurrentTCB->xStateListItem)) == 0)
	{
		// ����ǰ��������ȼ������ȼ�λͼ��ɾ��
		portRESET_READY_PRIORITY(pxCurrentTCB->uxPriority, uxTopReadyPriority);
	}
	// ������ʱ����ʱ��
	xTimeToWake = xConstTickCount + xTicksToWait;
	// ����ʱ����ֵ����Ϊ���������нڵ������ֵ
	listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xStateListItem), xTimeToWake);
	// �����ʱ����ʱ������
	if(xTimeToWake < xConstTickCount)
	{
		// ��������������������
		vListInsert((List_t *)pxOverflow_Delayed_Task_List,
		           (ListItem_t *)&(pxCurrentTCB->xStateListItem));
	}
	// û�����
	else
	{
		// ���뵽����������
		vListInsert((List_t *)pxDelayed_Task_List,
		           (ListItem_t *) &( pxCurrentTCB->xStateListItem));
		
		// ������һ���������ʱ�̱��� xNextTaskUnblockTime ��ֵ
		if(xTimeToWake < xNextTaskUnblockTime)
		{
			xNextTaskUnblockTime = xTimeToWake;
		}
	}
}

// �����Ĵ���������																 
static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,    // ������
                            const char* const pcName,          // ��������
                            const uint32_t ulStackDepth,       // ����ջ���
                            void* const pvParameters,          // �������
														UBaseType_t uxPriority,            // ���ȼ�
                            TaskHandle_t* const pxCreatedTask, // ������
                            TCB_t* pxNewTCB)                   // ����ջ����ָ��
{
	StackType_t *pxTopOfStack;
	UBaseType_t x;
	
	// ջ��ָ�룬�����Ƿ����һ�οռ����͵�ַ
	pxTopOfStack = pxNewTCB->pxStack + (ulStackDepth - (uint32_t)1);
	// 8 �ֽڶ���
	pxTopOfStack = (StackType_t*)(((uint32_t)pxTopOfStack) 
	                             & (~((uint32_t)0x0007)));
	// �����������Ƶ�TCB��
	for(x = (UBaseType_t)0;x < (UBaseType_t)configMAX_TASK_NAME_LEN;x++)
	{
		pxNewTCB->pcTaskName[x] = pcName[x];
		if(pcName[x] == 0x00)
			break;
	}
	pxNewTCB->pcTaskName[configMAX_TASK_NAME_LEN-1] = '\0';
	
	// ��ʼ��������
	vListInitialiseItem(&(pxNewTCB->xStateListItem));
	
	// ���ø��������ӵ����Ϊ pxNewTCB
	listSET_LIST_ITEM_OWNER(&(pxNewTCB->xStateListItem), pxNewTCB);

	// ��ʼ�����ȼ�
	if(uxPriority >= (UBaseType_t)configMAX_PRIORITIES)
	{
		uxPriority = (UBaseType_t)configMAX_PRIORITIES - (UBaseType_t)1U;
	}
	pxNewTCB->uxPriority = uxPriority;
	
	// ��ʼ������ջ
	pxNewTCB->pxTopOfStack = 
	          pxPortInitialiseStack(pxTopOfStack, pxTaskCode, pvParameters);
	
	if((void*)pxCreatedTask != NULL)
	{
	    *pxCreatedTask = (TaskHandle_t)pxNewTCB;
	}
}

// ��̬����������
#if (configSUPPORT_STATIC_ALLOCATION == 1)
TaskHandle_t xTaskCreateStatic(TaskFunction_t pxTaskCode,     // ������
                            const char* const pcName,         // ��������
                            const uint32_t ulStackDepth,      // ����ջ���
                            void* const pvParameters,         // �������
														UBaseType_t uxPriority,           // ���ȼ�
                            StackType_t* const puxTaskBuffer, // ����ջ��ʼָ��
                            TCB_t* const pxTaskBuffer)        // ����ջ����ָ��
{
	TCB_t* pxNewTCB;
	TaskHandle_t xReturn;
	// ����ջ����ָ�������ջ��ʼָ�벻Ϊ��
	if((pxTaskBuffer != NULL) && (puxTaskBuffer != NULL))
	{
		pxNewTCB = (TCB_t*)pxTaskBuffer;
		pxNewTCB->pxStack = (StackType_t*)puxTaskBuffer;
		
		// �����Ĵ���������
		prvInitialiseNewTask(pxTaskCode,
							 pcName,
							 ulStackDepth,
							 pvParameters,
							 uxPriority,
							 &xReturn,
							 pxNewTCB);
	
		// �����������Զ���������ӵ���������
		prvAddNewTaskToReadyList(pxNewTCB);
	}
	else
	{
		xReturn = NULL;
	}
	// ���񴴽��ɹ���Ӧ�÷��������������򷵻� NULL
	return xReturn;
}
#endif

// �������������
void vTaskStartScheduler(void)
{
  // ������������
	TaskHandle_t xIdleTaskHandle = xTaskCreateStatic((TaskFunction_t)prvIdleTask,
                                      (char *)"IDLE",
                                      (uint32_t)confgiMINIMAL_STACK_SIZE,
                                      (void *)NULL,
                                      (UBaseType_t)taskIDLE_PRIORITY,
                                      (StackType_t *)IdleTasKStack,
                                      (TCB_t *)&IdleTaskTCB);
	// ��ʼ���δ�ʱ������ֵ
	xTickCount = (TickType_t)0U;
	
	if(xPortStartScheduler() != pdFALSE){}
}

// ������Ⱥ���
void vTaskSwitchContext(void)
{
	taskSELECT_HIGHEST_PRIORITY_TASK();
}

// ������ʱ����
void vTaskDelay(const TickType_t xTicksToDelay)
{
	// ����ǰ������뵽��������
	prvAddCurrentTaskToDelayedList(xTicksToDelay);
	
	// �����л�
	taskYIELD();
}

/* task.c */
// ����������ʱ����
BaseType_t xTaskIncrementTick(void)
{
	TCB_t *pxTCB = NULL;
	TickType_t xItemValue;
	BaseType_t xSwitchRequired = pdFALSE;
	
	// ����ϵͳʱ�������� xTickCount
	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;

	// ��� xConstTickCount ��������л���ʱ�б�
	if(xConstTickCount == (TickType_t)0U)
	{
		taskSWITCH_DELAYED_LISTS();
	}
	
	// �������ʱ������ʱ����
	if(xConstTickCount >= xNextTaskUnblockTime)
	{
		for(;;)
		{
			// ��ʱ��������Ϊ�������� for ѭ��
			if(listLIST_IS_EMPTY(pxDelayed_Task_List) != pdFALSE)
			{
				// �����¸�����������ʱ��Ϊ���ֵ��Ҳ�������������
				xNextTaskUnblockTime = portMAX_DELAY;
				break;
			}
			else
			{
				// ���λ�ȡ��ʱ��������ͷ�ڵ�
				pxTCB=(TCB_t *)listGET_OWNER_OF_HEAD_ENTRY(pxDelayed_Task_List);
				// ���λ�ȡ��ʱ�������������нڵ���������ʱ��
				xItemValue = listGET_LIST_ITEM_VALUE(&(pxTCB->xStateListItem));
				
				// ������������������ʱ���ڵ����񶼱��Ƴ������� for ѭ��
				if(xConstTickCount < xItemValue)
				{
					xNextTaskUnblockTime = xItemValue;
					break;
				}
				
				// ���������ʱ�б��Ƴ��������ȴ�״̬
				(void)uxListRemove(&(pxTCB->xStateListItem));
				
				// ������ȴ���������ӵ������б�
				prvAddTaskToReadyList(pxTCB);
#if(configUSE_PREEMPTION == 1)
				// ����������״̬���������ȼ��ȵ�ǰ�������ȼ��ߣ�����Ҫ�����������
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

// ���� xNextTaskUnblockTime ����ֵ
static void prvResetNextTaskUnblockTime(void)
{
	TCB_t *pxTCB;
	// �л������������������Ϊ��
	if(listLIST_IS_EMPTY(pxDelayed_Task_List) != pdFALSE )
	{
		// �´ν����ʱ��ʱ��Ϊ���ܵ����ֵ
		xNextTaskUnblockTime = portMAX_DELAY;
	}
	else
	{
		// �����������Ϊ�գ��´ν����ʱ��ʱ��Ϊ����ͷ���������ʱ��
		(pxTCB) = (TCB_t *)listGET_OWNER_OF_HEAD_ENTRY(pxDelayed_Task_List);
		xNextTaskUnblockTime=listGET_LIST_ITEM_VALUE(&((pxTCB)->xStateListItem));
	}
}

