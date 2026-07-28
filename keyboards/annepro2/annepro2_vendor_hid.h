/*
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdint.h>

#define ANNEPRO2_VENDOR_HID_REPORT_SIZE 64

typedef struct {
    uint8_t key_major;
    uint8_t key_minor;
    uint8_t led_major;
    uint8_t led_minor;
    uint8_t ble_major;
    uint8_t ble_minor;
} annepro2_vendor_hid_versions_t;

typedef enum {
    ANNEPRO2_VENDOR_HID_UNHANDLED,
    ANNEPRO2_VENDOR_HID_REPLY,
    ANNEPRO2_VENDOR_HID_REPLY_ENTER_IAP,
} annepro2_vendor_hid_result_t;

/*
 * Decode one report as received by raw_hid_receive(), without the host-side
 * Report ID byte. Handled commands always produce a complete 64-byte response.
 */
annepro2_vendor_hid_result_t annepro2_vendor_hid_handle(const uint8_t *request, uint8_t length, const annepro2_vendor_hid_versions_t *versions, uint8_t response[ANNEPRO2_VENDOR_HID_REPORT_SIZE]);
