#include "bheznz.h"

#include "ws2812.h"

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
        case QK_DEBUG_TOGGLE:
            if (record->event.pressed) {
                ws2812_setleds(leds, 4);
                return true;
            }            return true; // Let QMK send the enter press/release events
        default:
            return true; // Process all other keycodes normally
    }
    return true;
}

