#pragma once

#include_next <halconf.h>

//config here
#undef HAL_USE_PWM
#define HAL_USE_PWM TRUE


/**
#define WS2812_PWM_DRIVER PWMD3 // TIMx
#define WS2812_PWM_CHANNEL      3    // default: 2
                                     //
#define WS2812_PWM_PAL_MODE 2 // DI Pin's alternate function value

//noused in gpiov1
//#define WS2812_PWM_PAL_MODE      2                    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 2
#define WS2812_PWM_DMA_STREAM        STM32_DMA1_STREAM2     // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
                                                            // always map to zero under dmav1
#define WS2812_PWM_DMA_CHANNEL       5                      // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
                                                            //
//use af_pp
#undef WS2812_EXTERNAL_PULLUP
#define WS2812_EXTERNAL_PULLUP

//#define WS2812_PWM_DMAMUX_ID         STM32_DMAMUX1_TIM3_UP  // DMAMUX configuration for TIMx_UP -- only required if your MCU has a DMAMUX peripheral, see the respective reference manual for the appropriate values for your MCU.

**/

#undef STM32_DMA_REQUIRED
#define STM32_DMA_REQUIRED TRUE

