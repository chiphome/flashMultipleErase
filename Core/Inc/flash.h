#ifndef FLASH__H
#define FLASH__H

#include "stm32g0xx_hal.h"
#include <string.h>

// FLASH����
#define FLASH_BASE_ADDR 0x0801F800 // FLASH���һҳ��ʼ��ַ (128KB - 2KB)
#define PAGE_SIZE 2048             // STM32G071ҳ���СΪ2KB
#define STATE_SIZE 16             // �ṹ���С����䵽24�ֽڣ�

typedef struct {
    unsigned int  color;                  // ��ɫ
    unsigned int  seconds;                // ����
    unsigned char mode;                   // ģʽ
    unsigned char number;                 // ���
    unsigned char padinng[5];             // Ԥ��5
	  unsigned char checksum;               // 1�ֽڣ�У���
}dataState;


extern dataState old_state;
extern dataState current_state;
void printState(dataState *state);
HAL_StatusTypeDef flash_program(unsigned int addr, unsigned char* data, unsigned int len);
void read_flash(unsigned int addr, unsigned char* data, unsigned int len);
void init_flash_addr(void);
void save_state(dataState* state);
void get_state(dataState* state);
void update_state(dataState* state);

#endif












