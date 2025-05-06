#ifndef __KEY_H
#define __KEY_H

#include "stm32g0xx_hal.h"

/* ����״̬���� */
typedef enum {
    KEY_UP   = 0,  /* ����δ���� */
    KEY_DOWN = 1   /* �������� */
} KEY_STATE;

/* �������� */
void KEY_Init(void);
KEY_STATE KEY_Scan(void);

#endif /* __KEY_H */
