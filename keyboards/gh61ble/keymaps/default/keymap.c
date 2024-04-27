// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rgb_matrix.h"
#include QMK_KEYBOARD_H


// clang-format off
/* QWERTY
 * .----00---------01---------02---------03---------04---------05---------06---------07---------08---------09---------10---------11---------12----.
 * .----------------------------------------------------------------------------------------------------------------------------------------------.
 * | ESC      | 1        | 2        | 3        | 4        | 5        | 6        | 7        | 8        | 9        | 0        | -        | =        |
 * |----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------|
 * | TAB      | Q        | W        | E        | R        | T        | Y        | U        | I        | O        | P        | [        | ]        |
 * |----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------|
 * | CAP LK   | A        | S        | D        | F        | G        | H        | J        | K        | L        | ;        | "        | ENT      |
 * |----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------+----------|
 * | LSHIFT   | Z        | X        | C        | V        | B        | N        | M        | <        | >        | ?        | UP       | SHT      |
 * |----------+----------+----------+----------+----------+---------------------+----------+----------+----------+----------+----------+----------|
 * | LCTRL    | LGUI     | LALT     | SPACE    | ALT      | FN       | CTR      | LEFT     | RIGHT    | BS       | \        | SHIFT    |          |
 * '----------------------------------------------------------------------------------------------------------------------------------------------'
 */

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [0] = LAYOUT_60_ansi(
         KC_ESC,     KC_1,       KC_2,       KC_3,      KC_4,       KC_5,     KC_6,      KC_7,      KC_8,     KC_9,      KC_0,     KC_MINS,   KC_EQL,KC_BSPC ,

         KC_TAB,     KC_Q,       KC_W,       KC_E,      KC_R,       KC_T,     KC_Y,      KC_U,      KC_I,     KC_O,      KC_P,     KC_LBRC,   KC_RBRC, KC_BSLS,

         KC_LCTL,    KC_A,       KC_S,       KC_D,      KC_F,       KC_G,     KC_H,      KC_J,      KC_K,     KC_L,      KC_SCLN,  KC_QUOT,   KC_ENT,

         KC_LSFT,    KC_Z,       KC_X,       KC_C,      KC_V,       KC_B,     KC_N,      KC_M,      KC_COMM,  KC_DOT,    KC_SLSH,   KC_DEL,

         KC_LCTL,    KC_LGUI,    KC_LALT,    KC_SPC,    KC_RALT,    MO(2),    MO(1), KC_RCTL
    ),
    [1] = LAYOUT_60_ansi(
         /*reset*/QK_BOOTLOADER,  GK_DEBUG,   KC_NO,   KC_NO,   KC_NO,   KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO, QK_DEBUG_TOGGLE,
         KC_NO,    KC_F23,  KC_F19,  KC_F20,  KC_F18,  /*BLE_DEL*/KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,
         KC_NO,    KC_NO,   RGB_MODE_FORWARD,   RGB_TOG,   SAFE_RANGE,    KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  QK_REBOOT,
         KC_NO,    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,
         KC_NO,    KC_NO,   KC_NO,   RGB_TOG,   GK_DEBUG,   RGB_MODE_FORWARD,  KC_NO,  KC_NO
    ),
    [2] = LAYOUT_60_ansi(
         QK_BOOT,  KC_1,   KC_2,   KC_3,   KC_4,      KC_5,   KC_6,  KC_7,   KC_8,   KC_9,    KC_0,    KC_MINS, KC_EQL, KC_BSPC,
         KC_TAB,  KC_Q,   KC_W,   KC_E,   KC_R,      KC_T,  KC_Y,   KC_U,   KC_I,   KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,
         KC_CAPS, KC_A,   KC_S,   KC_D,   KC_F,      KC_G,  KC_H,   KC_J,   KC_K,   KC_L,    KC_SCLN, KC_QUOT, KC_ENT,
         KC_LSFT, KC_Z,   KC_X,   KC_C,   KC_V,      KC_B,  KC_N,   KC_M,   KC_COMM,KC_DOT,  KC_SLSH, KC_RSFT,
         _______ ,_______,_______,RGB_MODE_FORWARD, _______,   _______, _______, _______
    ),
};

