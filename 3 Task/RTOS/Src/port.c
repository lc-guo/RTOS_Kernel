#include "FreeRTOS.h"

// 函数声明
void prvStartFirstTask(void);

// 错误出口
static void prvTaskExitError(void)
{
    for(;;);
}

// 初始化栈内存
StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, 
								   TaskFunction_t pxCode, 
								   void* pvParameters)
{
    // 异常发生时，自动加载到CPU的内容
    pxTopOfStack --;
    *pxTopOfStack = portINITIAL_XPSR;
    pxTopOfStack --;
    *pxTopOfStack = ((StackType_t)pxCode) & portSTART_ADDRESS_MASK;
    pxTopOfStack --;
    *pxTopOfStack = (StackType_t)prvTaskExitError;
	
    // r12、r3、r2和r1默认初始化为 0
    pxTopOfStack -= 5;
    *pxTopOfStack = (StackType_t)pvParameters;
	
    // 异常发生时，手动加载到CPU的内容
    pxTopOfStack -= 8;
	
    // 返回栈顶指针，此时pxTopOfStack指向空闲栈
    return pxTopOfStack;
}

// 启动调度器
BaseType_t xPortStartScheduler(void)
{
	// 设置 PendSV 和 SysTick 中断优先级为最低
	portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;
	
	// 初始化滴答定时器
	
	// 启动第一个任务，不再返回
	prvStartFirstTask();
	
	// 正常不会运行到这里
	return 0;
}

// 启动第一个任务,实际上是触发SVC中断
__asm void prvStartFirstTask(void)
{
	// 8字节对齐
    PRESERVE8
	
    /* 
	在 cortex-M 中，0xE000ED08 是 SCB_VTOR 这个寄存器的地址，
	里面放的是向量表的向量表的起始地址，即 msp 的地址 
	*/
    ldr r0,=0xE000ED08
    ldr r0,[r0]
    ldr r0,[r0]
	
	// 设置主栈指针msp的值
    msr msp,r0
	
    // 启用全局中断
    cpsie i
    cpsie f
    dsb
    isb
    // 调用SVC启动第一个任务
    svc 0
    nop
    nop
}

// SVC 中断服务函数
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

// PendSV 中断服务函数，真正实现任务切换的函数
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

