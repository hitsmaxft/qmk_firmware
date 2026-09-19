// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "matrix.h"

_Static_assert(MATRIX_ROWS == 5, "imk64 requires five matrix rows");
_Static_assert(MATRIX_COLS == 14, "imk64 requires fourteen matrix columns");
_Static_assert(sizeof(matrix_row_t) >= sizeof(uint16_t), "matrix_row_t is too narrow");

extern bool ch582_rust_matrix_snapshot(uint16_t *rows, size_t row_count);

/* Unique archive anchor: forces this strong custom-matrix object into the
 * final Rust-linked image instead of allowing matrix_common.c's weak defaults
 * to satisfy the archive extraction pass first. */
void ch582_imk64_matrix_backend(void) {}

void matrix_init_custom(void) {}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    uint16_t snapshot[MATRIX_ROWS];
    if (!ch582_rust_matrix_snapshot(snapshot, MATRIX_ROWS)) {
        return false;
    }

    bool changed = false;
    for (size_t row = 0; row < MATRIX_ROWS; ++row) {
        const matrix_row_t next = (matrix_row_t)snapshot[row];
        changed |= current_matrix[row] != next;
        current_matrix[row] = next;
    }
    return changed;
}
