/*
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "annepro2_ble_profile.h"

#define HID_USAGE_BRIGHTNESS_UP 0x006F
#define HID_USAGE_BRIGHTNESS_DOWN 0x0070
#define HID_USAGE_TRANSPORT_NEXT_TRACK 0x00B5
#define HID_USAGE_TRANSPORT_PREV_TRACK 0x00B6
#define HID_USAGE_TRANSPORT_PLAY_PAUSE 0x00CD
#define HID_USAGE_AUDIO_MUTE 0x00E2
#define HID_USAGE_AUDIO_VOL_UP 0x00E9
#define HID_USAGE_AUDIO_VOL_DOWN 0x00EA

#define AP2_BLE_EECONFIG_MAGIC 0xA2
#define AP2_BLE_EECONFIG_VERSION 0x01
#define AP2_BLE_EECONFIG_CHECK_XOR 0x5A
#define AP2_BLE_EECONFIG_SLOT_MASK 0x07
#define AP2_BLE_EECONFIG_PROFILE_BIT 0x08

static bool annepro2_ble_consumer_205_bit(uint16_t usage, uint8_t *bit) {
    switch (usage) {
        case HID_USAGE_AUDIO_MUTE:
            *bit = 0;
            return true;
        case HID_USAGE_AUDIO_VOL_UP:
            *bit = 1;
            return true;
        case HID_USAGE_AUDIO_VOL_DOWN:
            *bit = 2;
            return true;
        case HID_USAGE_TRANSPORT_PLAY_PAUSE:
            *bit = 3;
            return true;
        case HID_USAGE_TRANSPORT_NEXT_TRACK:
            *bit = 4;
            return true;
        case HID_USAGE_TRANSPORT_PREV_TRACK:
            *bit = 5;
            return true;
        case HID_USAGE_BRIGHTNESS_UP:
            *bit = 6;
            return true;
        case HID_USAGE_BRIGHTNESS_DOWN:
            *bit = 7;
            return true;
        default:
            return false;
    }
}

bool annepro2_ble_encode_consumer(annepro2_ble_profile_t profile, const uint16_t *usages, size_t usage_count, uint8_t out[ANNEPRO2_BLE_CONSUMER_MAX_SIZE], uint8_t *out_size) {
    for (uint8_t i = 0; i < ANNEPRO2_BLE_CONSUMER_MAX_SIZE; i++) {
        out[i] = 0;
    }

    if (profile == ANNEPRO2_BLE_PROFILE_AP2D_213) {
        if (usage_count > 4) {
            return false;
        }
        for (size_t i = 0; i < usage_count; i++) {
            out[i * 2]     = (uint8_t)usages[i];
            out[i * 2 + 1] = (uint8_t)(usages[i] >> 8);
        }
        *out_size = ANNEPRO2_BLE_CONSUMER_213_SIZE;
        return true;
    }

    if (profile != ANNEPRO2_BLE_PROFILE_C18_205) {
        return false;
    }

    for (size_t i = 0; i < usage_count; i++) {
        uint8_t bit;
        if (usages[i] == 0) {
            continue;
        }
        if (!annepro2_ble_consumer_205_bit(usages[i], &bit)) {
            return false;
        }
        out[0] |= (uint8_t)(1U << bit);
    }
    *out_size = ANNEPRO2_BLE_CONSUMER_205_SIZE;
    return true;
}

bool annepro2_ble_decode_leds(const uint8_t *payload, uint8_t payload_size, uint8_t *leds) {
    if (payload_size == 1) {
        *leds = payload[0];
        return true;
    }
    if (payload_size == 2 && payload[0] == 0x01) {
        *leds = payload[1];
        return true;
    }
    return false;
}

bool annepro2_ble_encode_slot_state(annepro2_ble_profile_t profile, bool broadcast, annepro2_ble_slot_state_t *state) {
    if (profile == ANNEPRO2_BLE_PROFILE_C18_205) {
        state->command = 0x0B;
        state->action  = broadcast ? 1 : 0;
        return true;
    }

    if (profile == ANNEPRO2_BLE_PROFILE_AP2D_213) {
        state->command = broadcast ? 0x0B : 0x24;
        state->action  = broadcast ? 1 : 2;
        return true;
    }

    return false;
}

bool annepro2_ble_encode_config(annepro2_ble_profile_t profile, int8_t slot, uint32_t *config) {
    if ((profile != ANNEPRO2_BLE_PROFILE_C18_205 && profile != ANNEPRO2_BLE_PROFILE_AP2D_213) || slot < -1 || slot > 3) {
        return false;
    }

    const uint8_t slot_code = slot < 0 ? 0 : (uint8_t)slot + 1;
    const uint8_t payload   = slot_code | (profile == ANNEPRO2_BLE_PROFILE_AP2D_213 ? AP2_BLE_EECONFIG_PROFILE_BIT : 0);
    const uint8_t checksum  = AP2_BLE_EECONFIG_MAGIC ^ AP2_BLE_EECONFIG_VERSION ^ payload ^ AP2_BLE_EECONFIG_CHECK_XOR;
    *config                 = ((uint32_t)AP2_BLE_EECONFIG_MAGIC << 24) | ((uint32_t)AP2_BLE_EECONFIG_VERSION << 16) | ((uint32_t)checksum << 8) | payload;
    return true;
}

bool annepro2_ble_decode_config(uint32_t config, annepro2_ble_profile_t *profile, int8_t *slot) {
    const uint8_t magic    = config >> 24;
    const uint8_t version  = config >> 16;
    const uint8_t checksum = config >> 8;
    const uint8_t payload  = config;

    if (magic != AP2_BLE_EECONFIG_MAGIC || version != AP2_BLE_EECONFIG_VERSION || checksum != (uint8_t)(magic ^ version ^ payload ^ AP2_BLE_EECONFIG_CHECK_XOR)) {
        return false;
    }

    const uint8_t slot_code = payload & AP2_BLE_EECONFIG_SLOT_MASK;
    if (slot_code > 4 || (payload & ~(AP2_BLE_EECONFIG_SLOT_MASK | AP2_BLE_EECONFIG_PROFILE_BIT)) != 0) {
        return false;
    }

    *slot    = slot_code == 0 ? -1 : (int8_t)slot_code - 1;
    *profile = (payload & AP2_BLE_EECONFIG_PROFILE_BIT) ? ANNEPRO2_BLE_PROFILE_AP2D_213 : ANNEPRO2_BLE_PROFILE_C18_205;
    return true;
}
