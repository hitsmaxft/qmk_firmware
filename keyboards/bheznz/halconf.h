#include_next "halconf.h"



// for w2812b using pwm
#undef HAL_USE_PWM
#define HAL_USE_PWM TRUE

#undef STM32_DMA_REQUIRED
#define STM32_DMA_REQUIRED TRUE

// for eeprom use i2c1
#undef HAL_USE_I2C
#define HAL_USE_I2C TRUE

#undef STM32_I2C_USE_I2C1
#define STM32_I2C_USE_I2C1 TRUE
