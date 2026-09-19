// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>
uint32_t ch582_rust_millis(void);
#define timer_read_internal ch582_rust_millis
