// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>
void ch582_rust_wait_us(uint32_t us);
#define wait_ms(ms) ch582_rust_wait_us((uint32_t)(ms) * 1000u)
#define wait_us(us) ch582_rust_wait_us(us)
#define waitInputPinDelay() ch582_rust_wait_us(20)
