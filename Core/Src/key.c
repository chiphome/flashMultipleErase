#include "key.h"

/* �������Ŷ��� */
#define KEY_PORT        GPIOC
#define KEY_PIN         GPIO_PIN_13
#define KEY_READ()      HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN)  /* ��ȡ��������״̬ */

/* ȥ����ʱ����λ�����룩 */
#define DEBOUNCE_DELAY  20

/**
 * @brief  ��ʼ����������
 * @param  None
 * @retval None
 */
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* ʹ�� GPIOC ʱ�� */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* ���� PC13 Ϊ���룬���� */
    GPIO_InitStruct.Pin = KEY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(KEY_PORT, &GPIO_InitStruct);
}

/**
 * @brief  ɨ�谴��״̬����ȥ������
 * @param  None
 * @retval KEY_STATE ����״̬��KEY_UP �� KEY_DOWN��
 */
KEY_STATE KEY_Scan(void)
{
    /* ��⵽�͵�ƽ���������£� */
    if (KEY_READ() == GPIO_PIN_RESET)
    {
        /* ��ʱȥ�� */
        HAL_Delay(DEBOUNCE_DELAY);

        /* �ٴ�ȷ�ϰ���״̬ */
        if (KEY_READ() == GPIO_PIN_RESET)
        {
            /* �ȴ������ͷ� */
            while (KEY_READ() == GPIO_PIN_RESET);
            return KEY_DOWN;
        }
    }
    return KEY_UP;
}
