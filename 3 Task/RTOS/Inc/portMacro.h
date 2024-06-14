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


#define portNVIC_INT_CTRL_REG       (*((volatile uint32_t*)0xE000ED04))
#define portNVIC_PENDSVSET_BIT      (1UL << 28UL)
// 触发 PendSV，产生上下文切换
#define portYIELD() \
{ \
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \
    __DSB(); \
    __ISB(); \
};


#endif //PORTMACRO_H

