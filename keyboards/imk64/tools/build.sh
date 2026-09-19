#!/usr/bin/env bash
# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
keyboard=${QMK_KEYBOARD:-imk64}
keymap=${QMK_KEYMAP:-default}
keyboard_id=${keyboard//\//_}
keymap_id=${keymap//\//_}
qmk_target="${keyboard_id}_${keymap_id}"
rust_dir="$repo_root/keyboards/$keyboard/rust"
qmk_archive="$repo_root/lib${qmk_target}.a"
target_dir=${RUST_QMK_TARGET_DIR:-"$repo_root/.build/rust-target/$qmk_target"}
output_dir=${QMK_ARTIFACT_DIR:-"$repo_root/.build/${qmk_target}-rust-qmk"}
rust_binary=${RUST_QMK_BINARY:-imk64-ch582-qmk}
artifact_prefix=${RUST_QMK_ARTIFACT_PREFIX:-imk64-ch582-qmk}
packager=${RUST_QMK_PACKAGER:-"$repo_root/keyboards/$keyboard/tools/ch582-imk64-uf2.py"}

if [[ ! -d "$rust_dir" ]]; then
    echo "selected QMK keyboard has no Rust backend: $rust_dir" >&2
    exit 1
fi

bash "$repo_root/keyboards/$keyboard/tools/test.sh"
if [[ "${CH582_SKIP_QMK_ARCHIVE:-0}" != 1 ]]; then
    make "$keyboard:$keymap:lib" -j"${QMK_JOBS:-8}" VERBOSE=false
fi

if [[ ! -f "$qmk_archive" ]]; then
    echo "QMK archive was not generated: $qmk_archive" >&2
    exit 1
fi

mkdir -p "$target_dir" "$output_dir"
rust_toolchain=${CH582_RUST_TOOLCHAIN:-1.87}
rustc_path=$(rustup which --toolchain "$rust_toolchain" rustc)
QMK_CH582_ARCHIVE="$qmk_archive" \
CARGO_INCREMENTAL=0 \
CARGO_TARGET_DIR="$target_dir" \
RUSTC="$rustc_path" \
RUSTFLAGS="-C link-arg=-Tlink.x" \
    rustup run "$rust_toolchain" \
        cargo build --locked --release \
            --target riscv32imc-unknown-none-elf \
            --manifest-path "$rust_dir/Cargo.toml"

elf="$target_dir/riscv32imc-unknown-none-elf/release/$rust_binary"
test -f "$elf"
riscv32-none-elf-objcopy -O binary --gap-fill 0xff \
    "$elf" "$output_dir/$artifact_prefix.bin"
riscv32-none-elf-objcopy -O ihex "$elf" "$output_dir/$artifact_prefix.hex"
cp "$elf" "$output_dir/$artifact_prefix.elf"
python3 "$packager" \
    "$output_dir/$artifact_prefix.bin" \
    "$output_dir/$artifact_prefix-mcuboot.bin" \
    "$output_dir/$artifact_prefix.uf2"

start_address=$(riscv32-none-elf-nm -n "$elf" | awk '$3 == "_start" { print $1 }')
if [[ "$start_address" != 00013000 ]]; then
    echo "unexpected CH582 application entry address: ${start_address:-missing}" >&2
    exit 1
fi

symbols=$(riscv32-none-elf-nm "$elf" | awk '{print $3}')
for symbol in _start __EXTERNAL_INTERRUPTS USB keyboard_task usbStartTransmitI \
              ch582_rust_usb_start ch582_rust_usb_try_write \
              matrix_scan_custom CH582_RUST_QMK_DIAGNOSTICS; do
    rg -x "$symbol" <<<"$symbols" >/dev/null || {
        echo "final ELF is missing required symbol: $symbol" >&2
        exit 1
    }
done

riscv32-none-elf-nm "$elf" | rg ' T matrix_scan_custom$' >/dev/null || {
    echo "final ELF selected QMK's weak default instead of the imk64 matrix backend" >&2
    exit 1
}

riscv32-none-elf-size "$elf"
shasum -a 256 "$output_dir/$artifact_prefix.elf" \
    "$output_dir/$artifact_prefix.bin" \
    "$output_dir/$artifact_prefix.hex" \
    "$output_dir/$artifact_prefix-mcuboot.bin" \
    "$output_dir/$artifact_prefix.uf2" \
    "$qmk_archive"
