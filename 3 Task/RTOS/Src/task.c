#include "task.h"

// ��������
List_t pxReadyTasksLists;

// ��ǰ TCB ָ��
TCB_t volatile *pxCurrentTCB = NULL;

// �� main.c �ж����������������
extern TCB_t Task1TCB;
extern TCB_t Task2TCB;

// ʹ�õ��ⲿ��������
extern StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
                                          TaskFunction_t pxCode, 
                                          void* pvParameters);
extern BaseType_t xPortStartScheduler(void);

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

// �����б��ʼ������
void prvInitialiseTaskLists(void)
{
	vListInitialise(&pxReadyTasksLists);
}

// ��������������
void vTaskStartScheduler(void)
{
	// �ֶ�ָ����һ�����е�����
	pxCurrentTCB = &Task1TCB;
	// ����������
	if(xPortStartScheduler() != pdFALSE)
	{
		// �����������ɹ��򲻻ᵽ����
	}
}

// ������Ⱥ���
void vTaskSwitchContext(void)
{
	// �������������л�
	if(pxCurrentTCB == &Task1TCB)
	{
		pxCurrentTCB = &Task2TCB;
	}
	else
	{
		pxCurrentTCB = &Task1TCB;
	}
}


