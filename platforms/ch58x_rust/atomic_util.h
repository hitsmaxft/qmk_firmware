// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>
uintptr_t ch582_rust_critical_enter(void);
void      ch582_rust_critical_exit(uintptr_t state);
#define CH582_RUST_ATOMIC_BLOCK(state_name) for (uintptr_t state_name = ch582_rust_critical_enter(), _ch582_once = 1; _ch582_once; ch582_rust_critical_exit(state_name), _ch582_once = 0)
#define ATOMIC_BLOCK_RESTORESTATE CH582_RUST_ATOMIC_BLOCK(_ch582_irq_state)
#define ATOMIC_BLOCK_FORCEON CH582_RUST_ATOMIC_BLOCK(_ch582_irq_state)
