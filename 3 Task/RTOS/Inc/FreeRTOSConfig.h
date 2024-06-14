// XXX �滻Ϊ��Ӧͷ�ļ�����
#ifndef FREERTOSCONFIG_H
#define FREERTOSCONFIG_H

#include "FreeRTOS.h"

/* FreeRTOSConfig.h */
// ���� TickType_t ����λ 16 λ 
#define configUSE_16_BIT_TICKS                  0
// ���������ַ�������
#define configMAX_TASK_NAME_LEN                 24
// �Ƿ�֧�־�̬��ʽ��������
#define configSUPPORT_STATIC_ALLOCATION         1
// �����ں��ж����ȼ���������ȼ���
#define configKERNEL_INTERRUPT_PRIORITY         15
// �����ں˿ɹ��������ж����ȼ�
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    11<<4


#endif //FREERTOSCONFIG_H

