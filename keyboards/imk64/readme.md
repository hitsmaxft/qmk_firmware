# imk64 CH582 Rust-backed QMK

This keyboard target is for the CH582 imk64 EVT board. QMK owns key processing,
while `qingke-rs`, `ch58x-hal-rs`, and `embassy-ch58x-rs` own startup, matrix
GPIO, timing, interrupts, and USBFS. A small ChibiOS-compatible C facade keeps
QMK independent from CH582 register access and forwards reports through a
stable ABI into the non-blocking Rust USB task.

QMK's synchronous compatibility wait returns immediately on this target;
matrix settling uses Embassy timers, and delay-dependent send-string and
lighting drivers are intentionally disabled. This prevents a C helper from
starving USB and matrix futures.

The 5x14 matrix uses PA1 through PA5 as active-high rows and PB4, PB5, PB6,
PB7, PB14, PB15, PB16, PB17, PB8, PB9, PA8, PB18, PB19, and PB20 as pull-down
columns. The default two-layer keymap mirrors the rusted-ch5
`examples/rmk-ch582m` example.

Each keyboard is built from its own QMK branch. From any checkout of this fork,
select the keyboard and its branch explicitly with:

```sh
util/build-keyboard-branch.sh imk64 default origin/imk64
```

The selector defaults to `origin/<keyboard>`, creates a disposable worktree,
verifies exact submodule revisions, and calls the build contract stored on that
keyboard's branch. After checking out the imk64 branch directly, the same
complete Rust-linked firmware can be built with:

```sh
keyboards/imk64/tools/build.sh
```

The wrapper accepts `QMK_KEYBOARD` and `QMK_KEYMAP`, so the same branch-local
contract can be reused by another keyboard branch with its own Rust crate and
packager. `QMK_ARTIFACT_DIR` and `RUST_QMK_TARGET_DIR` relocate output and
cache directories.

The generated update follows `hitsmaxft/qmk_port_ch582` imk64: MCUboot slot
`0x00012000`, application address `0x00013000`, slot end `0x00070000`, and UF2
family ID `0x1aa143c7`. It is an imk64 bootloader update, not a WCH ROM-ISP
image.

The build and host tests prove source, host behavior, RISC-V linking, and
container structure. Flash, boot, USB enumeration, and HID behavior remain
separate hardware validation gates.
