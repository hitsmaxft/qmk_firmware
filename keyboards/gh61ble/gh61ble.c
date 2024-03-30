#include "gh61ble.h"
#include "gpio.h"
#include "print.h"

void Stm32_Rest2(void) {
    __set_FAULTMASK(1);
    NVIC_SystemReset();
};

void bootloader_jump(void) {
    Stm32_Rest2();
};


void keyboard_pre_init_user(void) {
    print("keyboard init");
    palSetPad(GPIOC, 14);
}
