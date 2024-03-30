#pragma once

#include "quantum.h"

void RCC_DeInit(void);

void Stm32_Rest2(void);

#define __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH()       do {SYSCFG->MEMRMP &= ~(SYSCFG_MEMRMP_MEM_MODE);\
                                                         SYSCFG->MEMRMP |= SYSCFG_MEMRMP_MEM_MODE_0;\
                                                        }while(0);
#define ENABLE_INT() __set_PRIMASK(0)  /* 使能全局中断 */

#define DISABLE_INT() __set_PRIMASK(1) /* 禁止全局中断 */

#define APP_ADDR    0x8004000          /* APP地址 */

void JumpToBootloader(void);
