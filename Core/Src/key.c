#include "key.h"

/* 按键引脚定义 */
#define KEY_PORT        GPIOC
#define KEY_PIN         GPIO_PIN_13
#define KEY_READ()      HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN)  /* 读取按键引脚状态 */

/* 去抖延时（单位：毫秒） */
#define DEBOUNCE_DELAY  20

/**
 * @brief  初始化按键引脚
 * @param  None
 * @retval None
 */
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 GPIOC 时钟 */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* 配置 PC13 为输入，上拉 */
    GPIO_InitStruct.Pin = KEY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(KEY_PORT, &GPIO_InitStruct);
}

/**
 * @brief  扫描按键状态（带去抖动）
 * @param  None
 * @retval KEY_STATE 按键状态（KEY_UP 或 KEY_DOWN）
 */
KEY_STATE KEY_Scan(void)
{
    /* 检测到低电平（按键按下） */
    if (KEY_READ() == GPIO_PIN_RESET)
    {
        /* 延时去抖 */
        HAL_Delay(DEBOUNCE_DELAY);

        /* 再次确认按键状态 */
        if (KEY_READ() == GPIO_PIN_RESET)
        {
            /* 等待按键释放 */
            while (KEY_READ() == GPIO_PIN_RESET);
            return KEY_DOWN;
        }
    }
    return KEY_UP;
}
