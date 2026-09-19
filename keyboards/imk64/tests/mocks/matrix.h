// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

#define MATRIX_ROWS 5
#define MATRIX_COLS 14

typedef uint16_t matrix_row_t;

void matrix_init_custom(void);
bool matrix_scan_custom(matrix_row_t current_matrix[]);
