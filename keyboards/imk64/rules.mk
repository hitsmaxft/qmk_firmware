# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later

MCU = CH582
PROTOCOL = CHIBIOS_RUST
BOOTLOADER = custom
EEPROM_DRIVER = transient
CUSTOM_MATRIX = lite

SRC += chibios_usb_facade.c matrix.c

RGBLIGHT_ENABLE = no
RGB_MATRIX_ENABLE = no
AUDIO_ENABLE = no
