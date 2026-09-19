// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "matrix.h"

static bool     ready;
static uint16_t rust_rows[MATRIX_ROWS];

bool ch582_rust_matrix_snapshot(uint16_t *rows, size_t row_count) {
    assert(row_count == MATRIX_ROWS);
    if (!ready) {
        return false;
    }
    memcpy(rows, rust_rows, sizeof(rust_rows));
    return true;
}

int main(void) {
    matrix_row_t qmk_rows[MATRIX_ROWS] = {0};

    matrix_init_custom();
    assert(!matrix_scan_custom(qmk_rows));

    ready        = true;
    rust_rows[0] = 1u << 13;
    rust_rows[2] = 1u << 1;
    rust_rows[4] = 1u << 5;
    assert(matrix_scan_custom(qmk_rows));
    assert(qmk_rows[0] == (1u << 13));
    assert(qmk_rows[2] == (1u << 1));
    assert(qmk_rows[4] == (1u << 5));
    assert(!matrix_scan_custom(qmk_rows));

    rust_rows[2] = 0;
    assert(matrix_scan_custom(qmk_rows));
    assert(qmk_rows[2] == 0);
    return 0;
}
