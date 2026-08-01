/*
 * Copyright (c) 2018 Charlie Waters
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include "pin_defs.h"

#define ANNEPRO2_LED_MCU_ENABLE

/* C18D is the C18-layout target permanently paired with BLE 2.13. */
#define ANNEPRO2_VENDOR_HID_ENABLE
#define ANNEPRO2_BLE_ASYNC_STARTUP
#define RAW_EPSIZE 64
#define ANNEPRO2_KEY_FW_VERSION_MAJOR 2
#define ANNEPRO2_KEY_FW_VERSION_MINOR 37
#define ANNEPRO2_LED_FW_VERSION_MAJOR 2
#define ANNEPRO2_LED_FW_VERSION_MINOR 33
#define ANNEPRO2_BLE_FW_VERSION_MINOR 13
#define ANNEPRO2_BLE_EECONFIG_TAG 0x1D

#define LINE_UART_TX B0
#define LINE_UART_RX B1

#define LINE_BT_UART_TX A4 // Master TX, BLE RX
#define LINE_BT_UART_RX A5 // Master RX, BLE TX

#define PERMISSIVE_HOLD

#define SPI_DRIVER SPID1
#define SPI_SCK_PIN A0
#define SPI_MOSI_PIN A1
#define SPI_MISO_PIN A2

#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN A3
#define EXTERNAL_FLASH_SPI_CLOCK_DIVISOR 16
#define EXTERNAL_FLASH_PAGE_SIZE 256
#define EXTERNAL_FLASH_SECTOR_SIZE 4096
#define EXTERNAL_FLASH_BLOCK_SIZE 4096
#define EXTERNAL_FLASH_SIZE (256 * 1024)
