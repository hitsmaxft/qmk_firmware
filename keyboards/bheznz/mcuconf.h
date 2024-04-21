
#pragma once


#include_next "mcuconf.h"

#undef STM32_PWM_USE_TIM3
#define STM32_PWM_USE_TIM3  TRUE

#undef STM32_PWM_USE_ADVANCED
#define STM32_PWM_USE_ADVANCED TRUE

/*
//for pb7 tim4_ch2
#undef STM32_PWM_USE_TIM4
#define STM32_PWM_USE_TIM4  false

*/
