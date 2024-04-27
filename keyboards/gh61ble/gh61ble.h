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
    KC_AP2_BT1 = QK_KB_0,
    /*兼容 ap2 keymap*/
    KC_AP2_BT2,
    KC_AP2_BT3,
    KC_AP2_BT4,
    KC_AP2_BT_UNPAIR,
    KC_AP2_USB,
    KC_AP_LED_ON,
    KC_AP_LED_OFF,
    KC_AP_LED_TOG,
    KC_AP_LED_NEXT_PROFILE,
    KC_AP_LED_PREV_PROFILE,
    KC_AP_LED_NEXT_INTENSITY,
    KC_AP_LED_SPEED,
    KC_AP_RGB_VAI,
    KC_AP_RGB_VAD,
    KC_AP_RGB_TOG,
    KC_AP_RGB_MOD,
    KC_AP2_IAP,
    GK_DEBUG
};
