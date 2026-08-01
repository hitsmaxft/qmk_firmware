# MCU
MCU = cortex-m0plus
ARMV = 6
USE_FPU = no
MCU_FAMILY = HT32
MCU_SERIES = HT32F523xx
MCU_LDSCRIPT = HT32F52352_ANNEPRO2_C18
MCU_STARTUP = ht32f523xx

BOARD = ANNEPRO2_C18

# Bootloader selection
BOOTLOADER = custom
PROGRAM_CMD = annepro2_tools --boot $(BUILD_DIR)/$(TARGET).bin

# Anne Pro 2
RAW_ENABLE = yes

SRC = \
	annepro2_ble_v2.c \
	annepro2_ble_parser.c \
	annepro2_ble_slot_config.c \
	annepro2_ble_state.c \
	c18/annepro2_ble_protocol.c \
	annepro2_vendor_hid.c \
	ap2_led.c \
	protocol.c \
	rgb_driver.c \
