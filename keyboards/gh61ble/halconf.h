#pragma once

//config here
#undef HAL_USE_PWM
#define HAL_USE_PWM TRUE

#define WS2812_PWM_DRIVER PWMD3 // TIMx
#define WS2812_PWM_CHANNEL 2 // Channel
#define WS2812_PWM_PAL_MODE 2 // DI Pin's alternate function value
#define WS2812_PWM_DMA_STREAM STM32_DMA1_STREAM5 // DMA Stream for TIMx_UP
#include_next <halconf.h>


