#!/usr/bin/env bash
# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
keyboard_name=${QMK_KEYBOARD:-imk64}
keyboard="$repo_root/keyboards/$keyboard_name"
test_binary=$(mktemp /tmp/ch582-rust-qmk-facade.XXXXXX)
matrix_test_binary=$(mktemp /tmp/ch582-rust-qmk-matrix.XXXXXX)
trap 'rm -f "$test_binary" "$matrix_test_binary"' EXIT

cc -std=c11 -Wall -Wextra -Werror \
    -I "$keyboard" \
    "$keyboard/chibios_usb_facade.c" \
    "$keyboard/tests/chibios_usb_facade_test.c" \
    -o "$test_binary"
"$test_binary"

cc -std=c11 -Wall -Wextra -Werror \
    -I "$keyboard/tests/mocks" \
    "$keyboard/matrix.c" \
    "$keyboard/tests/matrix_snapshot_test.c" \
    -o "$matrix_test_binary"
"$matrix_test_binary"

# Hardware ownership crosses the C/Rust boundary only through the explicit
# ABI. No QMK C source may take over the CH582 USB registers.
if rg -n '0x4000[_]?8000|R8_USB|UEP[0-9]' \
    "$keyboard" "$repo_root/platforms/ch58x_rust" \
    "$repo_root/tmk_core/protocol/chibios_rust" --glob '*.{c,h}'; then
    echo "CH582 USB register access escaped the Rust backend" >&2
    exit 1
fi

for symbol in ch582_rust_usb_start ch582_rust_usb_stop ch582_rust_usb_try_write \
              ch582_rust_usb_keyboard_leds ch582_rust_matrix_snapshot; do
    rg -q "fn $symbol" "$keyboard/rust/src/main.rs" || {
        echo "Rust ABI export is missing: $symbol" >&2
        exit 1
    }
done

rg -q 'void ch582_imk64_matrix_backend\(void\)' "$keyboard/matrix.c" || {
    echo "QMK archive matrix anchor is missing" >&2
    exit 1
}

rg -q 'request_handler: Some\(' "$keyboard/rust/src/main.rs" || {
    echo "HID control-path request handler is missing" >&2
    exit 1
}

rg -q '#\[qingke_rs::entry\]' "$keyboard/rust/src/main.rs" || {
    echo "qingke-rs is not the selected Rust runtime" >&2
    exit 1
}

rg -q 'extern crate qingke_rs as qingke_rt;' "$keyboard/rust/src/main.rs" || {
    echo "embassy interrupt macro compatibility alias is missing" >&2
    exit 1
}

wait_impl=$(sed -n '/fn ch582_rust_wait_us/,/^}/p' "$keyboard/rust/src/main.rs")
if rg -n 'spin_loop|while|loop' <<<"$wait_impl"; then
    echo "a synchronous compatibility path can still starve the Embassy executor" >&2
    exit 1
fi
rg -q 'for _ in 0\.\.2' "$keyboard/rust/src/main.rs" || {
    echo "matrix snapshot retry must remain bounded" >&2
    exit 1
}

echo "CH582 Rust/QMK host and source gates: PASS"
