// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#define CH58X_PIN(port, pin) (((port) * 32u) + (pin))
#define B23 CH58X_PIN(1, 23)
