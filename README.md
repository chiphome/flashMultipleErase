# flashMultipleErase
如何用单片机FLASH模拟EEPROM，并且通过算法优化实现高达100万次以上的存储次数！

在单片机开发中，`数据存储`是一个绕不开的话题。EEPROM因其非易失性存储特性，常用于保存配置参数等数据。

然而，EEPROM的擦写次数通常有限，以STM32为例，STM32L0、STM32L4自带的EEPROM一般`10万次`左右，而很多单片机并不内置EEPROM，这时候，利用单片机的FLASH存储器来模拟EEPROM就成为了一个高性价比的解决方案，但，FLASH的擦写次数一般在`1万次`左右，这个我们可以通过ST的官方数据手册看到：

![img](https://mmbiz.qpic.cn/mmbiz_png/tGxRKz6VNHXWFm0pZCiaofDiaQVb26ICW55cSGlicY0LbFsoGAIXRiblGzYBYPicsmibPDAT6R25rL5ejZkT44ufeOLA/640?wx_fmt=png&from=appmsg)

**1万次，在很多场景下并不够。**

今天，老宇哥跟大家一起探讨，如何用单片机FLASH模拟EEPROM，并且通过算法优化实现高达100万次以上的存储次数！

我们都知道，独立的EEPROM芯片是可以直接写字节的，即使覆盖写也无须擦除，单片机的FLASH不同，STM32必须`按页来擦除`。

手头刚好有个STM32G071RBT6的开发板，就以这个芯片做测试，后续可以很方便的移植到其它芯片上。

![img](https://mmbiz.qpic.cn/mmbiz_jpg/tGxRKz6VNHUBRTM7Pt5k6d77gFSwopFnFV5H22UiczfsghaxOyMvPGJnbBdrMJmRzHPpGKSV4sLvDlatx3DqTrw/640?wx_fmt=jpeg&from=appmsg)

要求是，程序一共有16个字节的内容需要断电保存，每改变其中一个字节就需要保存一次，做到100万次的一个存储次数。

我们的核心存储算法是轮询存储，STM32G071RBT6的FLASH一共128KB，从0x08000000到0x0801FFFF，一共64页，每页2KB。

我们的数据就存储到最后一页，也就是0x0801F800到0x0801FFFF。

Flash主要是擦写次数受限，所以我们的思想是，一共16个字节，第一次就写入前16个字节，然后`更新写入地址索引`到第17个字节，下一次就写入17到32个字节，继续更新写入地址索引到33，以此类推。

这里还需要做的一个就是`重新上电`的时候，需要找到`最新的索引地址`，也就是如果已经写入了两次，需要自动找到索引地址为第33个字节，这个也是核心。

故，一页写满可以存储2048/16=128次，写满一页擦除一次，也就是`理论存储次数能达到128 × 1万次/页 = 128万次`。

下面上代码，头文件flash.h

```
#ifndef FLASH__H
#define FLASH__H

#include "stm32g0xx_hal.h"
#include <string.h>

// FLASH配置
#define FLASH_BASE_ADDR 0x0801F800 // FLASH最后一页起始地址 (128KB - 2KB)
#define PAGE_SIZE 2048             // STM32G071页面大小为2KB
#define STATE_SIZE 16             // 结构体大小（填充到24字节）

typedef struct {
 unsigned int  color;                  // 颜色
 unsigned int  seconds;                // 秒数
 unsigned char mode;                   // 模式
 unsigned char number;                 // 序号
 unsigned char padinng[5];             // 预留5
 unsigned char checksum;               // 1字节，校验和
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
```

头文件中定义了一个结构体，简单几个宏定义与函数声明，这里的结构体我们增加了一个字节的校验。

接下来看flash.c

```
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
```

第一步，先从第一个地址读取第一个16字节，然后判断是不是全部等于0xFF，如果第一次是就证明是第一次，下一步flash_addr就不需要增加STATE_SIZE，写入地址索引就是FLASH_BASE_ADDR。

第二步，如果不全是0xFF并且校验字节通过，证明这是一组有效数据，我们先将此数据更新到current_state，但是这里`还不能证明是最后一组有效数据`，因为最后一组有效数据才是我们要找到的数据。

就继续检查下一组数据，直到检查到一组数据是全0xFF，证明上一组数据就是最后一组有效数据，就跳出，此时我们也就`找到了最后一组`有效数据的起始地址。

接着就此地址增加STATE_SIZE就是最新可以存储数据的地址索引了。

如果flash_addr超出了空间，需要复位擦除一下，正常应该不会到这一步。

第三步，如果没找到有效数据，证明是第一次，就写入默认数值并保存，更新索引。

以上，上电的时候最新的写地址索引就找好了。

接下来是保存数据save_state函数：

// 保存状态到FLASH

```
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
```

函数就比较简单了，首先将要保存的数据与最新存储的数据做对比，如果没变化，就不操作；`如果地址超出范围了，就先擦除整个页`，更新写索引到FLASH_BASE_ADDR，接着保存数据到当下最新写地址索引即可。

重要的就是这两个函数了，其它函数都很普通没必要解释。

**实际测试数据：**

刚下在进去代码，最后一页全部为0XFF，然后按下5次按键，number数据每次加1存储。

![img](https://mmbiz.qpic.cn/mmbiz_png/tGxRKz6VNHUBRTM7Pt5k6d77gFSwopFnfUsGZY2ZNqxzoxn74XvfZhN3icSvy5nvmaGTF1co48ezEkoQLUBaePw/640?wx_fmt=png&from=appmsg)

下面这张是按了128次，存储了128次的结果，整个页都写满了。

![img](https://mmbiz.qpic.cn/mmbiz_png/tGxRKz6VNHUBRTM7Pt5k6d77gFSwopFn6LL9ormicHX6ibiajOEBJSKQmUHJrRKI2hnksCrphpbdX2LkzJlKTXZ9g/640?wx_fmt=png&from=appmsg)

最后一张是写满之后再按一次，Flash进行了擦除，并保存在第一组位置中。

![img](https://mmbiz.qpic.cn/mmbiz_png/tGxRKz6VNHUBRTM7Pt5k6d77gFSwopFn5icbJDekjHhqicl1TE4iaZA57cDyzPrXE9SBnyo3XkaDnPIyicVibwhUqMw/640?wx_fmt=png&from=appmsg)

整个代码逻辑有它的应用场景，也可能有一些bug，非常欢迎大家指正，代码整体老宇哥会上传到GitHub，欢迎大家留言Star！

现在很多独立的`EEPROM芯片`都性价比很高了，直接IIC协议进行读写，可以按字节直接修改，`轻松达到100W次`的擦写次数，具体大家根据项目的应用场景，不同的要求高度进行选择。 
