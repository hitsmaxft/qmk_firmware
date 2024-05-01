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
rgb_led_t leds[16] = {
    [0 ... 15] = {
        .g=255,
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
  rgb_matrix_set_color_all(0,0,0);
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

#ifdef OLED_ENABLE
bool oled_task_user(void) {
    // Host Keyboard Layer Status
    oled_write_P(PSTR("Layer: "), false);

    switch (get_highest_layer(layer_state)) {
        case 0:
            oled_write_P(PSTR("Default\n[A] [S] [D] [F]\n[Z] [X] [C] [V]\n"), false);
            break;
        case 1:
            oled_write_P(PSTR("Layer 1\n__ RV+ RH+ RH-\n__ RV- RM+ RM-\n"), false);
            break;
        case 2:
            oled_write_P(PSTR("Layer 2\n___ RTG ___ DGB\nQBT DSW RV+ RV-\n"), false);
            break;
        default:
            // Or use the write_ln shortcut over adding '\n' to the end of your string
            oled_write_ln_P(PSTR("Undefined"), false);
    }

    // Host Keyboard LED Status
    led_t led_state = host_keyboard_led_state();
    oled_write_P(led_state.num_lock ? PSTR("NUM ") : PSTR("    "), false);
    oled_write_P(led_state.caps_lock ? PSTR("CAP ") : PSTR("    "), false);
    oled_write_P(led_state.scroll_lock ? PSTR("SCR ") : PSTR("    "), false);

    return false;
}
#endif
