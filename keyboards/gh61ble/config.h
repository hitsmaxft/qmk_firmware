#pragma once


#define MATRIX_ROWS 5
#define MATRIX_COLS 13

#define RGB_MATRIX_LED_COUNT 61

#define EARLY_INIT_PERFORM_BOOTLOADER_JUMP TRUE

#define WS2812_PWM_DRIVER PWMD3 // TIM3
#define WS2812_PWM_CHANNEL      2// default: 2
                                     //
#define WS2812_PWM_PAL_MODE 2 // DI Pin's alternate function value

//noused in gpiov1
//#define WS2812_PWM_PAL_MODE      2                    // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 2
#define WS2812_PWM_DMA_STREAM        STM32_DMA1_STREAM2     // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#define WS2812_PWM_DMA_CHANNEL       5                      // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.

//#define WS2812_PWM_DMAMUX_ID         STM32_DMAMUX1_TIM3_UP  // DMAMUX configuration for TIMx_UP -- only required if your MCU has a DMAMUX peripheral, see the respective reference manual for the appropriate values for your MCU.
