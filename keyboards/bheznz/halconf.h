
// for w2812b using pwm
#ifndef NOZNZ

#   define HAL_USE_PWM TRUE
//#define HAL_USE_UART TRUE

// eeprom use i2c1
// oled use i2c2
#   define HAL_USE_I2C TRUE

#endif

#include_next "halconf.h"
