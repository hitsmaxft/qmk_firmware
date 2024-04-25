#pragma once
#include "quantum.h"
#include <stdint.h>

void pre_kb_init_user(void);
void setup_znz_usb(void);

enum ZNZKeyCodes {
    KC_ZNZ_DEBUG = QK_KB_0
};
