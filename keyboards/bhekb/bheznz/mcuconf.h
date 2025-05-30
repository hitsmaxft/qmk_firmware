#pragma once

#include_next "mcuconf.h"

#ifndef NOZNZ

#undef STM32_PWM_USE_TIM3
#define STM32_PWM_USE_TIM3  TRUE


#undef STM32_PWM_USE_ADVANCED
#define STM32_PWM_USE_ADVANCED TRUE

#undef STM32_DMA_REQUIRED
#define STM32_DMA_REQUIRED TRUE

#undef STM32_I2C_USE_I2C1
#define STM32_I2C_USE_I2C1 TRUE

// for stm32f103

#define I2C1_CLOCK_SPEED 400000 /* 400000 */
#define I2C1_DUTY_CYCLE 2

#endif
