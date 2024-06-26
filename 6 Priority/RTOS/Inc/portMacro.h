#ifndef PORTMACRO_H
#define PORTMACRO_H

#include "FreeRTOS.h"

/* portMacro.h */
#include <stdint.h>

#define port_CHAR                   char
#define port_FLOAT                  float
#define port_DOUBLE                 double
#define port_LONG                   long
#define port_SHORT                  short
#define port_STACK_TYPE             unsigned int
#define port_BASE_TYPE              long

typedef port_STACK_TYPE             StackType_t;
typedef long                        BaseType_t;
typedef unsigned long               UBaseType_t;

typedef port_STACK_TYPE*            StackType_p;
typedef long*                       BaseType_p;
typedef unsigned long*              UBaseType_p;


#if(configUSE_16_BIT_TICKS == 1)
		typedef uint16_t                TickType_t;
		#define portMAX_DELAY           (TickType_t) 0xffff
#else
		typedef uint32_t                TickType_t;
		#define portMAX_DELAY           (TickType_t) 0xffffffffUL
#endif

#define pdFALSE                     ((BaseType_t) 0)
#define pdTRUE                      ((BaseType_t) 1)
#define pdPASS                      (pdTRUE)
#define pdFAIL                      (pdFALSE)


#define portINITIAL_XPSR            (0x01000000)
#define portSTART_ADDRESS_MASK      ((StackType_t) 0xFFFFFFFEUL)

#define portNVIC_SYSPRI2_REG        (*((volatile uint32_t *) 0xE000ED20))
#define portNVIC_PENDSV_PRI         (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 16UL)
#define portNVIC_SYSTICK_PRI        (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 24UL)

#define vPortSVCHandler             SVC_Handler
#define xPortPendSVHandler          PendSV_Handler
#define xPortSysTickHandler         SysTick_Handler

#define portNVIC_INT_CTRL_REG       (*((volatile uint32_t*)0xE000ED04))
#define portNVIC_PENDSVSET_BIT      (1UL << 28UL)
// 触发 PendSV，产生上下文切换
#define portYIELD() \
{ \
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \
    __DSB(); \
    __ISB(); \
};


#define portSET_INTERRUPT_MASK_FROM_ISR()       ulPortRaiseBASEPRI()
// 带返回值关中断，将当前中断状态作为返回值返回
static __inline uint32_t ulPortRaiseBASEPRI(void)
{
    uint32_t ulReturn,ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm
    {
        mrs ulReturn,basepri
        msr basepri,ulNewBASEPRI
        dsb
        isb
    }
    return ulReturn;
}

#define portDISABLE_INTERRUPTS()                vPortRaiseBASEPRI()
// 不带返回值关中断
static __inline void vPortRaiseBASEPRI(void)
{
    uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;
    __asm
    {
        msr basepri,ulNewBASEPRI
        dsb
        isb
    }
}

#define portENABLE_INTERRUPTS()                 vPortSetBASEPRI(0)
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)    vPortSetBASEPRI(x)
// 开中断
static __inline void vPortSetBASEPRI(uint32_t ulBASEPRI)
{
    __asm
    {
        msr basepri,ulBASEPRI
    }
}


extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);
#define portENTER_CRITICAL()         vPortEnterCritical()
#define portEXIT_CRITICAL()          vPortExitCritical()


/* protMacro.h */
#define portRECORD_READY_PRIORITY(uxPriority, uxReadyPriorities) (uxReadyPriorities) |= (1UL << (uxPriority))
#define portGET_HIGHEST_PRIORITY(uxTopPriority, uxReadyPriorities) uxTopPriority = (31UL - (uint32_t) __clz((uxReadyPriorities)))
#define portRESET_READY_PRIORITY(uxPriority, uxReadyPriorities) (uxReadyPriorities) &= ~(1UL << (uxPriority))



#endif //PORTMACRO_H

