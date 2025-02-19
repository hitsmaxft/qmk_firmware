#pragma once
#include "quantum.h"
#include <stdint.h>

void pre_kb_init_user(void);
void setup_znz_usb(void);
void add_keycode_to_history(uint16_t keycode);
void print_recent_keycodes(void);
void sprint_recent_keycodes(char * buffer, uint8_t size);

/**
 * append mod char in front of keycode , for mod taps
 * prefix_str(buffer, "C","1")
 */
char* prefix_str(char* str, char* kc_str, char c);

char* keycode_to_ascii(uint16_t keycode);

enum ZNZKeyCodes {
    KC_ZNZ_DEBUG = QK_KB_0
};

#undef STM32_HSECLK
#define STM32_HSECLK                16000000U
