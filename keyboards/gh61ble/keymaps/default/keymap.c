// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

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
         KC_CAPS,    KC_A,       KC_S,       KC_D,      KC_F,       KC_G,     KC_H,      KC_J,      KC_K,     KC_L,      KC_SCLN,  KC_QUOT,   KC_ENT,
         KC_LSFT,    KC_Z,       KC_X,       KC_C,      KC_V,       KC_B,     KC_N,      KC_M,      KC_COMM,  KC_DOT,    KC_SLSH,   KC_DEL,
         KC_LCTL,    KC_LGUI,    KC_LALT,    KC_SPC,    KC_RALT,    MO(1),    MO(2), KC_RCTL
    ),
    [1] = LAYOUT_60_ansi(
         QK_BOOT,  KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO, KC_NO,
         KC_NO,    KC_F23,  KC_F19,  KC_F20,  KC_F18,  /*BLE_DEL*/KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,
         KC_NO,    KC_NO,   RGB_MODE_FORWARD,   RGB_TOG,   SAFE_RANGE,    KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,
         KC_NO,    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,  KC_NO,
         KC_NO,    KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,  KC_NO,  KC_NO
    ),
    [2] = LAYOUT_60_ansi(
         KC_ESC,  KC_1,   KC_2,   KC_3,   KC_4,      KC_5,   KC_6,  KC_7,   KC_8,   KC_9,    KC_0,    KC_MINS, KC_EQL, KC_BSPC,
         KC_TAB,  KC_Q,   KC_W,   KC_E,   KC_R,      KC_T,  KC_Y,   KC_U,   KC_I,   KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,
         KC_CAPS, KC_A,   KC_S,   KC_D,   KC_F,      KC_G,  KC_H,   KC_J,   KC_K,   KC_L,    KC_SCLN, KC_QUOT, KC_ENT,
         KC_LSFT, KC_Z,   KC_X,   KC_C,   KC_V,      KC_B,  KC_N,   KC_M,   KC_COMM,KC_DOT,  KC_SLSH, KC_RSFT,
         KC_TRNS ,KC_TRNS,KC_TRNS,KC_TRNS, KC_TRNS,   KC_TRNS, KC_TRNS, KC_TRNS
    ),
};


void Stm32_Rest2(void) {
    __set_FAULTMASK(1);
    NVIC_SystemReset();
};
#ifdef RGB_MATRIX_ENABLE
led_config_t g_led_config = {
    {
        { 60, //  0   Esc
          59, //  1   1
          58, //  2   2
          57, //  3   3
          56, //  4   4
          55, //  5   5
          54, //  6   6
          53, //  7   7
          52, //  8   8
          51, //  9   9
          50, //  10  0
          49, //  11  -
          48  //  12  =
              // 50  //  13  Backspace
          },
        { 46, //  14  TAB
          45, //  15  Q
          44, //  16  W
          43, //  17  E
          42, //  18  R
          41, //  19  T
          40, //  20  Y
          39, //  21  U
          38, //  22  I
          37, //  23  O
          36, //  24  P
          35, //  25  [
          34 //  26  ]

         // 36, //  27  |
       },
        { 32, //  14  CPS
          31, //  15  A
          30, //  16  S
          29, //  17  D
          28, //  18  F
          27, //  19  G
          26, //  20  H
          25, //  21  J
          24, //  22  K
          23, //  23  L
          22, //  24  ;
          21, //  25  '
          20 //  26  ENT

        },
        { 19, //   0  SHT
          18, //   1  Z
          17, //   2  X
          16, //   3  C
          15, //   4  V
          14, //   5  B
          13, //   6  N
          12, //   7  M
          11, //   8  <
          10, //   9  >
           9, //  10  /
           8, //  12  SHT
           NO_LED
        },
        {  7, //   0  CTR
           6, //   1  WIN
           5, //   2  ALT
           4, //   3  SPA
           3, //   4  ALT
           2, //   5  FN
           1, //   6  GUI
           0, //   7  CTR
           NO_LED,
           47, //   9  Backspace
           33, //  10  |
           NO_LED,
           NO_LED
           }
    }, {
        { 0,  0 }, //  0  Esc
        {17,  0 }, //  1  1
        {34,  0 }, //  2  2
        {52,  0 }, //  3  3
        {69,  0 }, //  4  4
        {86,  0 }, //  5  5
        {103, 0 }, //  6  6
        {121, 0 }, //  7  7
        {138, 0 }, //  8  8
        {155, 0 }, //  9  9
        {172, 0 }, //  10 0
        {190, 0 }, //  11 -
        {207, 0 }, //  12 =
        {224, 0 }, //  13 Backspace
        { 0,  16}, //  14  TAB
        {17,  16}, //  15  Q
        {34,  16}, //  16  W
        {52,  16}, //  17  E
        {69,  16}, //  18  R
        {86,  16}, //  19  T
        {103, 16}, //  20  Y
        {121, 16}, //  21  U
        {138, 16}, //  22  I
        {155, 16}, //  23  O
        {172, 16}, //  24  P
        {190, 16}, //  25  [
        {207, 16}, //  26  ]
        {224, 16}, //  27  |
        { 0,  32}, //  14  CPS
        {17,  32}, //  15  A
        {34,  32}, //  16  S
        {52,  32}, //  17  D
        {69,  32}, //  18  F
        {86,  32}, //  19  G
        {103, 32}, //  20  H
        {121, 32}, //  21  J
        {138, 32}, //  22  K
        {155, 32}, //  23  L
        {172, 32}, //  24  ;
        //{190, 32}, //  25  '
        {207, 32}, //  25  '
        {224, 32}, //  27  ENT
        { 0,  48}, //  14  LSHT
        {20,  48}, //  15  Z
        {41,  48}, //  16  X
        {61,  48}, //  17  C
        {81,  48}, //  18  V
        {102, 48}, //  19  B
        {122, 48}, //  20  N
        {143, 48}, //  21  M
        {163, 48}, //  22  <
        {183, 48}, //  23  >
        {203, 48}, //  24  /
        {224, 48}, //  25  RSHT
        { 0,  64}, //  14  CTR
        {17,  64}, //  15  WIN
        {34,  64}, //  16  ALT
        {95,  64}, //  17  SPE
        {155, 64}, //  18  ALT
        {172, 64}, //  19  FN
        {190, 32}, //  20  GUI
        {207, 32}, //  21  CTR
    }, {
        1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    }
};

#endif
