#include "FreeRTOS.h"

// �ж�Ƕ�׼�����
static UBaseType_t uxCriticalNesting = 0xAAAAAAAA;

// ��������
void prvStartFirstTask(void);

// �������
static void prvTaskExitError(void)
{
    for(;;);
}

// ��ʼ��ջ�ڴ�
StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
								   TaskFunction_t pxCode, 
								   void* pvParameters)
{
	// �쳣����ʱ���Զ����ص�CPU������
	pxTopOfStack --;
	*pxTopOfStack = portINITIAL_XPSR;
	pxTopOfStack --;
	*pxTopOfStack = ((StackType_t)pxCode) & portSTART_ADDRESS_MASK;
	pxTopOfStack --;
	*pxTopOfStack = (StackType_t)prvTaskExitError;

	// r12��r3��r2��r1Ĭ�ϳ�ʼ��Ϊ 0
	pxTopOfStack -= 5;
	*pxTopOfStack = (StackType_t)pvParameters;

	// �쳣����ʱ���ֶ����ص�CPU������
	pxTopOfStack -= 8;

	// ����ջ��ָ�룬��ʱpxTopOfStackָ�����ջ
	return pxTopOfStack;
}

// ����������
BaseType_t xPortStartScheduler(void)
{
	// ���� PendSV �� SysTick �ж����ȼ�Ϊ���
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;
	
	// ��ʼ���δ�ʱ��
	
	// ������һ�����񣬲��ٷ���
	prvStartFirstTask();
	
	// �����������е�����
	return 0;
}

// ������һ������,ʵ�����Ǵ���SVC�ж�
__asm void prvStartFirstTask(void)
{
	PRESERVE8

	ldr r0,=0xE000ED08
	ldr r0,[r0]
	ldr r0,[r0]
	msr msp,r0

	cpsie i
	cpsie f
	dsb
	isb
	
	svc 0
	nop
	nop
}

// SVC �жϷ�����
__asm void vPortSVCHandler(void)
{
	extern pxCurrentTCB;

	PRESERVE8
	
	ldr r3,=pxCurrentTCB
	ldr r1,[r3]
	ldr r0,[r1]
	ldmia r0!,{r4-r11}
	msr psp,r0
	isb
	mov r0,#0
	msr basepri,r0
	orr r14,#0xd

	bx r14
}

// PendSV �жϷ�����������ʵ�������л��ĺ���
__asm void xPortPendSVHandler(void)
{
	extern pxCurrentTCB;
	extern vTaskSwitchContext;

	PRESERVE8

	mrs r0,psp
	isb

	ldr r3,=pxCurrentTCB
	ldr r2,[r3]

	stmdb r0!,{r4-r11}
	str r0,[r2]

	stmdb sp!,{r3,r14}
	mov r0,#configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri,r0
	dsb
	isb
	bl vTaskSwitchContext
	mov r0,#0
	msr basepri,r0
	ldmia sp!,{r3,r14}

	ldr r1,[r3]
	ldr r0,[r1]
	ldmia r0!,{r4-r11}
	msr psp,r0
	isb
	bx r14
	nop
}

// �����ٽ���
void vPortEnterCritical(void)
{
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;
	if(uxCriticalNesting==1)
	{
		// configASSERT((portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK) == 0);
	}
}

// �˳��ٽ���
void vPortExitCritical(void)
{
	// configASSERT(uxCriticalNesting);
	uxCriticalNesting--;
	if(uxCriticalNesting == 0)
	{
		portENABLE_INTERRUPTS();
	}
}

// SysTick �жϷ�����
void xPortSysTickHandler(void)
{
	// ���ж�
	vPortRaiseBASEPRI();
	// ����������ʱ����
	xTaskIncrementTick();
	// ���ж�
	vPortSetBASEPRI(0);
}
