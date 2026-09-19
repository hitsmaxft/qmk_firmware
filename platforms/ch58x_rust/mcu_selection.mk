# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later
ifeq ($(MCU),CH582)
    PLATFORM_KEY := ch58x_rust
    MCU_ARCH := riscv32imc
    MCU_FAMILY := CH58x
endif
