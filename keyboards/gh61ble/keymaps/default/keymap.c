// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "rgb_matrix.h"
#include QMK_KEYBOARD_H


#include "rgb_matrix_types.h"



// Function to revert an array of Position
void GH61_revertArray(led_point_t arr[60], int size) {
    int start = 0;
    int end = size - 1;
    while (start < end) {
        // Swap the start and end elements
        led_point_t temp = arr[start];
        arr[start] = arr[end];
        arr[end] = temp;

        start++;
        end--;
    }
}


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
    }
    , {
        {224, 64}, //  21  CTR
            {210, 64}, //  20  GUI
            {172, 64}, //  19  FN
            {180, 64}, //  18  ALT
            {102,  64}, //  17  SPE
            {34,  64}, //  16  ALT
            {17,  64}, //  15  WIN
            { 0,  64}, //  14  CTR
                                 //
                                 //
            {220, 48}, //  25  RSHT
            {203, 48}, //  24  /
            {183, 48}, //  23  >
            {163, 48}, //  22  <
            {143, 48}, //  21  M
            {122, 48}, //  20  N
            {102, 48}, //  19  B
            {81,  48}, //  18  V
            {61,  48}, //  17  C
            {41,  48}, //  16  X
            {20,  48}, //  15  Z
            { 0,  48}, //  14  LSHT
                                  //
            {224, 32}, //  27  ENT
            {207, 32}, //  25  '
                                  //{190, 32}, //  25  '
            {172, 32}, //  24  ;
            {155, 32}, //  23  L
            {138, 32}, //  22  K
            {121, 32}, //  21  J
            {103, 32}, //  20  H
            {86,  32}, //  19  G
            {69,  32}, //  18  F
            {52,  32}, //  17  D
            {34,  32}, //  16  S
            {17,  32}, //  15  A
            { 0,  32}, //  14  CPS
                       //
            {224, 16}, //  27  |
            {207, 16}, //  26  ]
            {190, 16}, //  25  [
            {172, 16}, //  24  P
            {155, 16}, //  23  O
            {138, 16}, //  22  I
            {121, 16}, //  21  U
            {103, 16}, //  20  Y
            {86,  16}, //  19  T
            {69,  16}, //  18  R
            {52,  16}, //  17  E
            {34,  16}, //  16  W
            {17,  16}, //  15  Q
            { 0,  16}, //  14  TAB
                       //
            {224, 0 }, //  13 Backspace
            {207, 0 }, //  12 =
            {190, 0 }, //  11 -
            {172, 0 }, //  10 0
            {155, 0 }, //  9  9
            {138, 0 }, //  8  8
            {121, 0 }, //  7  7
            {103, 0 }, //  6  6
            {86,  0 }, //  5  5
            {69,  0 }, //  4  4
            {52,  0 }, //  3  3
            {34,  0 }, //  2  2
            {17,  0 }, //  1  1
            { 0,  0 }, //  0  Esc
    }
    , {
        1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    }
};


#endif
