# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later
CC := riscv32-none-elf-gcc
AR := riscv32-none-elf-ar
OBJCOPY := riscv32-none-elf-objcopy
OBJDUMP := riscv32-none-elf-objdump
SIZE := riscv32-none-elf-size
NM := riscv32-none-elf-nm
MCUFLAGS := -march=rv32imc -mabi=ilp32
COMPILEFLAGS += $(MCUFLAGS) -ffreestanding -fshort-enums -ffunction-sections -fdata-sections
CFLAGS += $(COMPILEFLAGS) -std=gnu11 -Os -fno-strict-aliasing
CXXFLAGS += $(COMPILEFLAGS) -fno-exceptions -fno-rtti
LDFLAGS += $(MCUFLAGS) -nostdlib
OPT_DEFS += -DPROTOCOL_CHIBIOS_RUST -DNO_USB_STARTUP_CHECK

# qingke-rs owns reset/startup and the Rust executor owns the main loop.
SRC := $(filter-out $(QUANTUM_DIR)/main.c,$(SRC))
.SECONDEXPANSION:
lib$(TARGET).a: $$(OBJ)
	$(AR) rcs $@ $(call uniq,$(OBJ))
cpfirmware:
	@true
check-size:
	@true
