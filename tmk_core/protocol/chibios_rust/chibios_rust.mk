# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later
CHIBIOS_RUST_DIR = protocol/chibios_rust
SRC += $(CHIBIOS_RUST_DIR)/chibios_rust.c
VPATH += $(TMK_PATH)/$(CHIBIOS_RUST_DIR)
OPT_DEFS += -DFIXED_CONTROL_ENDPOINT_SIZE=64 -DFIXED_NUM_CONFIGURATIONS=1
