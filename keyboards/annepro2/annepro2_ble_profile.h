/*
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    ANNEPRO2_BLE_PROFILE_C18_205 = 0,
    ANNEPRO2_BLE_PROFILE_AP2D_213,
} annepro2_ble_profile_t;

#ifndef ANNEPRO2_BLE_DEFAULT_PROFILE
#    define ANNEPRO2_BLE_DEFAULT_PROFILE ANNEPRO2_BLE_PROFILE_C18_205
#endif

#define ANNEPRO2_BLE_CONSUMER_205_SIZE 4
#define ANNEPRO2_BLE_CONSUMER_213_SIZE 8
#define ANNEPRO2_BLE_CONSUMER_MAX_SIZE ANNEPRO2_BLE_CONSUMER_213_SIZE

typedef struct {
    uint8_t command;
    uint8_t action;
} annepro2_ble_slot_state_t;

/*
 * Encode QMK Consumer usages for the selected BLE HID report map.
 * AP2D BLE 2.13 accepts up to four 16-bit usages; BLE 2.05 accepts only
 * the eight usages represented by its bitmap.
 */
bool annepro2_ble_encode_consumer(annepro2_ble_profile_t profile, const uint16_t *usages, size_t usage_count, uint8_t out[ANNEPRO2_BLE_CONSUMER_MAX_SIZE], uint8_t *out_size);

/*
 * Encode the one-shot slot-state notification sent before the primary
 * 0x40/0x01 (broadcast) or 0x40/0x04 (connect) command. BLE 2.13 uses a
 * distinct command and action for connect; BLE 2.05 retains the established
 * QMK protocol.
 */
bool annepro2_ble_encode_slot_state(annepro2_ble_profile_t profile, bool broadcast, annepro2_ble_slot_state_t *state);

/*
 * Decode the BLE MCU's C18-compatible Caps Lock state-sync frame:
 *   7B 12 35 00 03 00 00 7D 20 07 VV
 * Both BLE 2.05 and 2.13 use VV as a normalized boolean. Unknown values and
 * frames from other protocol routes must not alter QMK's host LED state.
 */
bool annepro2_ble_decode_caps_lock(const uint8_t *frame, size_t size, bool *caps_lock);

/* Versioned, checksummed 32-bit eeconfig record. Slots are -1 (none) or 0..3. */
bool annepro2_ble_encode_config(annepro2_ble_profile_t profile, int8_t slot, uint32_t *config);
bool annepro2_ble_decode_config(uint32_t config, annepro2_ble_profile_t *profile, int8_t *slot);
