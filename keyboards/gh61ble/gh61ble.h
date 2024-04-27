#pragma once

#include "quantum.h"

void RCC_DeInit(void);

void Stm32_Rest2(void);

#ifndef STM32_BOOTLOADER_DUAL_BANK
#    define STM32_BOOTLOADER_DUAL_BANK FALSE
#endif

#define __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH()       do {SYSCFG->MEMRMP &= ~(SYSCFG_MEMRMP_MEM_MODE);\
                                                         SYSCFG->MEMRMP |= SYSCFG_MEMRMP_MEM_MODE_0;\
                                                        }while(0);
#define __HAL_RCC_DMA1_CLK_ENABLE()  do { \
                                        __IO uint32_t tmpreg = 0x00U; \
                                        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);\
                                        UNUSED(tmpreg); \
                                         } while(0U)
#define ENABLE_INT() __set_PRIMASK(0)  /* 使能全局中断 */

#define DISABLE_INT() __set_PRIMASK(1) /* 禁止全局中断 */

#define APP_ADDR    0x8004000          /* APP地址 */

void JumpToBootloader(void);

enum GH61BLEKeyCodes {
    GK_DEBUG = QK_KB_0
};
