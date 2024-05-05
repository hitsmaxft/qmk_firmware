#pragma once

#include_next "mcuconf.h"


//enable tim3 for pwm on pa07
#undef STM32_PWM_USE_TIM3
#define STM32_PWM_USE_TIM3 TRUE

#undef STM32_PWM_USE_ADVANCED
#define STM32_PWM_USE_ADVANCED TRUE

//disable LSE for use osc32_in as pc14
#undef STM32_LSE_ENABLED
#define STM32_LSE_ENABLED FALSE

//enable dma
#undef STM32_DMA_REQUIRED
#define STM32_DMA_REQUIRED TRUE
