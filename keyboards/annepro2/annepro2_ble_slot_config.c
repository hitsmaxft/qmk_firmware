/*
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "annepro2_ble_slot_config.h"

#define AP2_BLE_EECONFIG_MAGIC 0xA2
#define AP2_BLE_EECONFIG_CHECK_XOR 0x5A

bool annepro2_ble_slot_config_encode(uint8_t target_tag, int8_t slot, uint32_t *config) {
    if (target_tag == 0 || slot < -1 || slot > 3 || config == NULL) {
        return false;
    }

    const uint8_t payload  = slot < 0 ? 0 : (uint8_t)slot + 1;
    const uint8_t checksum = AP2_BLE_EECONFIG_MAGIC ^ target_tag ^ payload ^ AP2_BLE_EECONFIG_CHECK_XOR;
    *config                = ((uint32_t)AP2_BLE_EECONFIG_MAGIC << 24) | ((uint32_t)target_tag << 16) | ((uint32_t)checksum << 8) | payload;
    return true;
}

bool annepro2_ble_slot_config_decode(uint8_t target_tag, uint32_t config, int8_t *slot) {
    if (target_tag == 0 || slot == NULL) {
        return false;
    }

    const uint8_t magic      = config >> 24;
    const uint8_t stored_tag = config >> 16;
    const uint8_t checksum   = config >> 8;
    const uint8_t payload    = config;

    if (magic != AP2_BLE_EECONFIG_MAGIC || stored_tag != target_tag || payload > 4 || checksum != (uint8_t)(magic ^ stored_tag ^ payload ^ AP2_BLE_EECONFIG_CHECK_XOR)) {
        return false;
    }

    *slot = payload == 0 ? -1 : (int8_t)payload - 1;
    return true;
}
