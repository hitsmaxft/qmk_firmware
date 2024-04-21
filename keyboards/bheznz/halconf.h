#include_next "halconf.h"


#undef HAL_USE_PWM
#define HAL_USE_PWM TRUE

#undef HAL_USE_I2C
#define HAL_USE_I2C TRUE

#undef STM32_I2C_USE_I2C1
#define STM32_I2C_USE_I2C1 TRUE
