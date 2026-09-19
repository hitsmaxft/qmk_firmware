// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdbool.h>
#include "bootloader.h"
void ch582_rust_reset(bool bootloader) __attribute__((noreturn));
void bootloader_jump(void) {
    ch582_rust_reset(true);
}
void mcu_reset(void) {
    ch582_rust_reset(false);
}
