#include "bheznz.h"

#include "ws2812.h"
#include "debug.h"
#include "print.h"
#include "rgb_matrix.h"

void _znz_eeconfig_debug_rgb_matrix(void) {
    dprintf("rgb_matrix_config EEPROM\n");
    dprintf("rgb_matrix_config.enable = %d\n", rgb_matrix_config.enable);
    dprintf("rgb_matrix_config.mode = %d\n", rgb_matrix_config.mode);
    dprintf("rgb_matrix_config.hsv.h = %d\n", rgb_matrix_config.hsv.h);
    dprintf("rgb_matrix_config.hsv.s = %d\n", rgb_matrix_config.hsv.s);
    dprintf("rgb_matrix_config.hsv.v = %d\n", rgb_matrix_config.hsv.v);
    dprintf("rgb_matrix_config.speed = %d\n", rgb_matrix_config.speed);
    dprintf("rgb_matrix_config.flags = %d\n", rgb_matrix_config.flags);
}

// show debug
rgb_led_t leds[4] = {
    {
        .g=255,
        .r=0,
        .b=0,
    }
    , {
        .g=0,
        .r=255,
        .b=0,
    }
    , {
        .g=0,
        .r=0,
        .b=255,
    }
    , {
        .g=0,
        .r=0,
        .b=0,
    }

};
void keyboard_post_init_user(void) {
  // Customise these values to desired behaviour
  debug_enable=true;
  debug_matrix=true;
  //debug_keyboard=true;
  //debug_mouse=true;
}

__attribute__((weak)) bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_ZNZ_DEBUG:
            if (record->event.pressed) {
                _znz_eeconfig_debug_rgb_matrix();
            }
            return false;
        default:
            return true; // Process all other keycodes normally
    }
    return true;
}

