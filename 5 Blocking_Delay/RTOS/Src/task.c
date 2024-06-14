#include "task.h"

#define confgiMINIMAL_STACK_SIZE 128

StackType_t	IdleTasKStack[confgiMINIMAL_STACK_SIZE];

// �δ�ʱ������ֵ
static volatile TickType_t xTickCount = (TickType_t)0U;

// ��������
List_t pxReadyTasksLists;

// ��ǰ TCB ָ��
TCB_t volatile *pxCurrentTCB = NULL;

// �����������
TCB_t IdleTaskTCB;


// �� main.c �ж����������������
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;

// ʹ�õ��ⲿ��������
extern StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
                                          TaskFunction_t pxCode, 
                                          void* pvParameters);
extern BaseType_t xPortStartScheduler(void);		

// �����б��ʼ������
void prvInitialiseTaskLists(void)
{
	vListInitialise(&pxReadyTasksLists);
}

// ������������
void prvIdleTask(void *p_arg)
{
    for(;;){}
}																		

// �����Ĵ���������																 
static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,    // ������
                            const char* const pcName,          // ��������
                            const uint32_t ulStackDepth,       // ����ջ���
                            void* const pvParameters,          // �������
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
							 &xReturn,
							 pxNewTCB);
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
										(StackType_t *)IdleTasKStack,
										(TCB_t *)&IdleTaskTCB);
	// ������������뵽����������
	vListInsertEnd(&(pxReadyTasksLists), 
				         &(((TCB_t *)(&IdleTaskTCB))->xStateListItem));

	pxCurrentTCB = &Task1TCB;
	if(xPortStartScheduler() != pdFALSE){}
}

// ������Ⱥ���
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

// ������ʱ����
void vTaskDelay(const TickType_t xTicksToDelay)
{
    TCB_t *pxTCB = NULL;

    // ��ȡ��ǰҪ��ʱ������ TCB
    pxTCB = (TCB_t *)pxCurrentTCB;
    // ��¼��ʱʱ��
    pxTCB->xTicksToDelay = xTicksToDelay;
    // ��������������ȣ��ó� MCU 
    taskYIELD();
}

// ����������ʱ����
void xTaskIncrementTick(void)
{
	TCB_t *pxTCB = NULL;
	ListItem_t *pxListItem = NULL;
	List_t *pxList = &pxReadyTasksLists;
	uint8_t xSwitchRequired = pdFALSE;
	
	// ���� xTickCount ϵͳʱ��������
	const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;
	
	// �����������Ƿ�Ϊ��
	if(listLIST_IS_EMPTY(pxList) == pdFALSE) 
	{
		// ��Ϊ�ջ�ȡ����ͷ������
		pxListItem = listGET_HEAD_ENTRY(pxList);

		// ����������������������
		while(pxListItem != (ListItem_t *)&(pxList->xListEnd)) 
		{
			// ��ȡÿ���������������ƿ� TCB
			pxTCB = (TCB_t *)listGET_LIST_ITEM_OWNER(pxListItem);
			
			// ��ʱ�����ݼ�
			if(pxTCB->xTicksToDelay > 0){
				pxTCB->xTicksToDelay--;
			}
			else{
				xSwitchRequired = pdTRUE;
			}
			// �ƶ�����һ��������
			pxListItem = listGET_NEXT(pxListItem);
		}
	}
	// ������������������������״̬�ָ��Ͳ����������
	if(xSwitchRequired == pdTRUE){
		// �����������
		taskYIELD();
	}
}
