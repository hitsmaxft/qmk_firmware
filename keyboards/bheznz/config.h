#pragma once


/**
 * pb7
 * TIM4_CH2 Channel 4
 * TIM4_UP channel 7
 */

#define WS2812_DI_PIN B0

/*
#    define WS2812_PWM_DRIVER        PWMD4                  // default: PWMD2
                                                            // dma channel

#    define WS2812_PWM_CHANNEL       1                      // default: 2

#    define WS2812_PWM_PAL_MODE      2                      // Pin "alternate function", see the respective datasheet for the appropriate values for your MCU. default: 2
                                                            //
#    define WS2812_PWM_DMA_STREAM        STM32_DMA1_STREAM7     // DMA Stream for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
#    define WS2812_PWM_DMA_CHANNEL       7                      // DMA Channel for TIMx_UP, see the respective reference manual for the appropriate values for your MCU.
                                                                //
#    define WS2812_PWM_DMAMUX_ID         STM32_DMAMUX1_TIM4_UP  // DMAMUX configuration for TIMx_UP -- only required if your MCU has a DMAMUX peripheral, see the respective reference manual for the appropriate values for your MCU.

*/

#define RGBLIGHT_LED_COUNT 16

#define RGB_MATRIX_LED_COUNT 16
