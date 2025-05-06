#ifndef FLASH__H
#define FLASH__H

#include "stm32g0xx_hal.h"
#include <string.h>

// FLASH配置
#define FLASH_BASE_ADDR 0x0801F800 // FLASH最后一页起始地址 (128KB - 2KB)
#define PAGE_SIZE 2048             // STM32G071页面大小为2KB
#define STATE_SIZE 16             // 结构体大小（填充到24字节）

typedef struct {
    unsigned int  color;                  // 颜色
    unsigned int  seconds;                // 秒数
    unsigned char mode;                   // 模式
    unsigned char number;                 // 序号
    unsigned char padinng[5];             // 预留5
	  unsigned char checksum;               // 1字节，校验和
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












