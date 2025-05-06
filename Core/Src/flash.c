#include "flash.h"
#include "stdio.h"

// 全局变量
unsigned int flash_addr = FLASH_BASE_ADDR; // 当前写入位置
dataState current_state;              // RAM中的当前状态
dataState old_state;


// 打印 current_state 结构体内容
void printState(dataState *state) {
	printf("current addr: %X\r\n",flash_addr - STATE_SIZE);
	printf("color: %u\r\n",state->color);
  printf("seconds: %u\r\n",state->seconds);
  printf("mode: %u\r\n", state->mode);
  printf("number: %u\r\n", state->number);
  printf("Checksum: %02X\r\n", state->checksum);
}

// 计算校验和
uint8_t calculate_checksum(dataState* state) {
    uint8_t checksum = state->color ^ state->seconds ^ state->mode ^ state->number ;
    return checksum;
}

// 解锁FLASH
HAL_StatusTypeDef flash_unlock(void) {
    return HAL_FLASH_Unlock();
}

// 锁定FLASH
HAL_StatusTypeDef flash_lock(void) {
    return HAL_FLASH_Lock();
}

// 擦除FLASH页面
HAL_StatusTypeDef erase_page(uint32_t page_addr) {
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t page_error = 0;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Page = (page_addr - FLASH_BASE) / PAGE_SIZE; // 计算页面编号
    erase_init.NbPages = 1;
		
		printf("erase page: %X\r\n",erase_init.Page);

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }
    HAL_FLASH_Lock();
		return HAL_OK;
}


// 写入FLASH（按64位对齐）
HAL_StatusTypeDef flash_program(uint32_t addr, uint8_t* data, uint32_t len) {
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t i;

    if (len % 8 != 0) {
        return HAL_ERROR; // 长度必须是8的倍数
    }

    flash_unlock();
    for (i = 0; i < len; i += 8) {
        uint64_t data64 = *(uint64_t*)(data + i);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i, data64);
        if (status != HAL_OK) {
            break;
        }
    }
    flash_lock();

    return status;
}

// 读取FLASH
void read_flash(uint32_t addr, uint8_t* data, uint32_t len) {
    memcpy(data, (void*)addr, len);
}

// 初始化：查找最新有效数据
void init_flash_addr(void) {
    dataState temp_state;
    uint32_t addr = FLASH_BASE_ADDR;
    uint32_t last_valid_addr = FLASH_BASE_ADDR;
    int found_valid_data = 0;

    while (addr < FLASH_BASE_ADDR + PAGE_SIZE) {
        read_flash(addr, (uint8_t*)&temp_state, STATE_SIZE);

        // 检查是否全0xFF
        uint8_t all_ff[STATE_SIZE];
        memset(all_ff, 0xFF, STATE_SIZE);
        int is_all_ff = (memcmp(&temp_state, all_ff, STATE_SIZE) == 0);

        if (!is_all_ff && temp_state.checksum == calculate_checksum(&temp_state)) {
            last_valid_addr = addr;
            found_valid_data = 1;
            memcpy(&current_state, &temp_state, STATE_SIZE);								

        } else {					
          break;
        }
        addr += STATE_SIZE;
    }

    flash_addr = last_valid_addr + (found_valid_data ? STATE_SIZE : 0);
		
		if(found_valid_data)
			  printf("init first,found valid data,last_valid_addr:%X\r\n",last_valid_addr);
		else
			  printf("init first,it is all ff,it is the first data\r\n");
					
    if (flash_addr > FLASH_BASE_ADDR + PAGE_SIZE) {
			  printf("init erase page 2KB\r\n");
        erase_page(FLASH_BASE_ADDR);
        flash_addr = FLASH_BASE_ADDR;
    }

    if (!found_valid_data) {
			  printf("not found valid data\r\n");
        current_state.color = 100;
        current_state.seconds = 200;
        current_state.mode = 1;
        current_state.number = 1;
			  current_state.checksum = calculate_checksum(&current_state);
        __disable_irq(); 
        flash_program(FLASH_BASE_ADDR, (uint8_t*)&current_state, STATE_SIZE);
        __enable_irq(); 
        flash_addr = FLASH_BASE_ADDR + STATE_SIZE;       
    }
    printState(&current_state);
}

// 保存状态到FLASH
void save_state(dataState* state) {
    dataState last_state;

    if (flash_addr > FLASH_BASE_ADDR) {
        read_flash(flash_addr - STATE_SIZE, (uint8_t*)&last_state, STATE_SIZE);
        if (memcmp(&last_state, state, STATE_SIZE) == 0) {
          printf("数据没有变化，直接返回");
          return; 
        }
    }
     __disable_irq(); 
    if (flash_addr + STATE_SIZE > FLASH_BASE_ADDR + PAGE_SIZE) {
			  printf("erase page 2KB\r\n");
        erase_page(FLASH_BASE_ADDR);
        flash_addr = FLASH_BASE_ADDR;	  
    }
    
    state->checksum = calculate_checksum(state);
    flash_program(flash_addr, (uint8_t*)state, STATE_SIZE);
    flash_addr += STATE_SIZE; 
     __enable_irq(); 
}

// 获取当前状态
void get_state(dataState* state) {
    memcpy(state, &current_state, STATE_SIZE);
}

// 更新状态
void update_state(dataState* state) {
    state->color += 2;
    state->seconds += 1;
    state->mode += 1;
    state->number += 1;
    save_state(state);
    printState(state);
}
