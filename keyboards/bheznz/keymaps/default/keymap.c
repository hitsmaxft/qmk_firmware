// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┬───┐
     * │ A │ S │ D │ F │
     * ├───┼───┼───┼───┤
     * │ Z │ X │ C │ V │
     * └───┴───┴───┴───┘
     */
    [0] = LAYOUT_ortho_2x4(
        LT(2, KC_Z),    KC_S,    KC_D,    KC_F,
        LT(1, KC_Z),    KC_X,    KC_C,    KC_V
    ),
    [1] = LAYOUT_ortho_2x4(
        RGB_MOD,    RGB_RMOD,    RGB_HUI,    RGB_HUD,
        RGB_SAI,    RGB_SAD,    RGB_VAI,   RGB_VAD
    ),
    [2] = LAYOUT_ortho_2x4(
        RGB_MOD,    RGB_RMOD,    RGB_HUI,    RGB_HUD,
        RGB_SAI,    RGB_SAD,    RGB_VAI,   RGB_VAD
    ),
};
