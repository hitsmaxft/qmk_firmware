// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#include "keycodes.h"
#include "quantum.h"
#include "bheznz.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┬───┐
     * │ A │ S │ D │ F │
     * ├───┼───┼───┼───┤
     * │ Z │ X │ C │ V │
     * └───┴───┴───┴───┘
     */
    [0] = LAYOUT_ortho_4x4(
        KC_1,    KC_2,    KC_3,    KC_A,
        KC_4,    KC_5,    KC_6,    KC_B,
        KC_7,    KC_8,    KC_9,    KC_C,
        LT(1, KC_Z),    KC_0,    LT(2, KC_C),    KC_D
    ),
    [1] = LAYOUT_ortho_4x4(
        RGB_SPI,    RGB_VAI,    RGB_HUI,    RGB_HUD,
        RGB_SPD,    RGB_VAD,    RGB_HUD,    RGB_RMOD,
        RGB_MOD,    RGB_TOG,    _______,    KC_ZNZ_DEBUG,
        QK_BOOT,    RGB_TOG,    RGB_SPI,   QK_DEBUG_TOGGLE
    ),
    [2] = LAYOUT_ortho_4x4(
        _______,    _______,    _______,    KC_ZNZ_DEBUG,
        _______,    _______,    _______,    KC_ZNZ_DEBUG,
        _______,    _______,    _______,    KC_ZNZ_DEBUG,
        _______,    _______,    RGB_SPI,   RGB_SPD
    ),
};
