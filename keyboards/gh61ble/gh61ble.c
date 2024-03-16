#include "gh61ble.h"


void Stm32_Rest2(void) {
    __set_FAULTMASK(1);
    NVIC_SystemReset();
};

