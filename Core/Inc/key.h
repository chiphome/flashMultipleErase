#ifndef __KEY_H
#define __KEY_H

#include "stm32g0xx_hal.h"

/* 按键状态定义 */
typedef enum {
    KEY_UP   = 0,  /* 按键未按下 */
    KEY_DOWN = 1   /* 按键按下 */
} KEY_STATE;

/* 函数声明 */
void KEY_Init(void);
KEY_STATE KEY_Scan(void);

#endif /* __KEY_H */
