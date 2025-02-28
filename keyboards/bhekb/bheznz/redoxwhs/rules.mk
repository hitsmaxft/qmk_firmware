MCU = STM32F103
BOARD = STM32_F103_STM32DUINO

RGB_MATRIX_ENABLE = no
OLED_ENABLE = no
CUSTOM_MATRIX = lite

# project specific files
SRC += matrix.c
UART_DRIVER_REQUIRED = yes

OPT_DEFS += -DNOZNZ=1

