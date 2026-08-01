/*
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

/* Fixed C18 BLE 2.05 wire protocol. No BLE 2.13 code is linked here. */
#include "annepro2_ble_protocol.h"

#include <string.h>

#define HID_USAGE_BRIGHTNESS_UP 0x006F
#define HID_USAGE_BRIGHTNESS_DOWN 0x0070
#define HID_USAGE_TRANSPORT_NEXT_TRACK 0x00B5
#define HID_USAGE_TRANSPORT_PREV_TRACK 0x00B6
#define HID_USAGE_TRANSPORT_PLAY_PAUSE 0x00CD
#define HID_USAGE_AUDIO_MUTE 0x00E2
#define HID_USAGE_AUDIO_VOL_UP 0x00E9
#define HID_USAGE_AUDIO_VOL_DOWN 0x00EA

static int8_t clamp_mouse_axis8(int16_t value, uint8_t loss_flag, uint8_t *loss_flags) {
    if (value < -127) {
        *loss_flags |= loss_flag;
        return -127;
    }
    if (value > 127) {
        *loss_flags |= loss_flag;
        return 127;
    }
    return (int8_t)value;
}

void ap2_ble_protocol_reset(void) {}

void ap2_ble_protocol_cancel_command(void) {}

bool ap2_ble_protocol_begin_command(uint8_t slot, uint8_t retries, ap2_ble_actions_t deferred_actions, uint32_t now, ap2_ble_protocol_step_t *step) {
    (void)slot;
    (void)retries;
    (void)deferred_actions;
    (void)now;
    (void)step;
    return false;
}

bool ap2_ble_protocol_task(uint32_t now, ap2_ble_protocol_step_t *step) {
    (void)now;
    (void)step;
    return false;
}

void ap2_ble_protocol_receive_frame(const uint8_t *frame, uint8_t size) {
    (void)frame;
    (void)size;
}

uint8_t ap2_ble_protocol_encode_slot_state(bool broadcast, uint8_t slot, uint8_t out[AP2_BLE_PROTOCOL_MAX_FRAME_SIZE]) {
    static const uint8_t prefix[] = {
        0x7B, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7D, 0x20, 0x0B,
    };

    if (out == NULL || slot > 3) {
        return 0;
    }

    memcpy(out, prefix, sizeof(prefix));
    out[10] = slot;
    out[11] = broadcast ? 1 : 0;
    return 12;
}

uint8_t ap2_ble_protocol_encode_slot_command(bool broadcast, uint8_t slot, uint8_t out[AP2_BLE_PROTOCOL_MAX_FRAME_SIZE]) {
    static const uint8_t prefix[] = {
        0x7B, 0x12, 0x53, 0x00, 0x03, 0x00, 0x00, 0x7D, 0x40, 0x00,
    };

    if (out == NULL || slot > 3) {
        return 0;
    }

    memcpy(out, prefix, sizeof(prefix));
    out[9]  = broadcast ? 0x01 : 0x04;
    out[10] = slot;
    out[11] = 0x00;
    return 12;
}

static bool consumer_bit(uint16_t usage, uint8_t *bit) {
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

bool ap2_ble_protocol_encode_consumer(const uint16_t *usages, size_t usage_count, uint8_t out[AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE], uint8_t *out_size) {
    if (out == NULL || out_size == NULL || (usage_count > 0 && usages == NULL)) {
        return false;
    }

    memset(out, 0, AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE);
    for (size_t i = 0; i < usage_count; i++) {
        uint8_t bit;
        if (usages[i] == 0) {
            continue;
        }
        if (!consumer_bit(usages[i], &bit)) {
            return false;
        }
        out[0] |= (uint8_t)(1U << bit);
    }
    *out_size = 4;
    return true;
}

uint8_t ap2_ble_protocol_encode_mouse(uint8_t buttons, int16_t x, int16_t y, int16_t vertical, int16_t horizontal, uint8_t out[AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE], uint8_t *loss_flags) {
    static const uint8_t prefix[] = {
        0x7B, 0x12, 0x53, 0x00, 0x0A, 0x00, 0x00, 0x7D, 0x60, 0x04,
    };

    if (out == NULL || loss_flags == NULL) {
        return 0;
    }

    *loss_flags = 0;
    memcpy(out, prefix, sizeof(prefix));

    /* C18 KEY 2.36.3 report ID 3: 3 buttons, X/Y, wheel, pan, pad. */
    if (buttons & 0xF8) {
        *loss_flags |= AP2_BLE_PROTOCOL_MOUSE_LOSS_BUTTONS;
    }
    out[10] = buttons & 0x07;
    out[11] = (uint8_t)x;
    out[12] = (uint8_t)(x >> 8);
    out[13] = (uint8_t)y;
    out[14] = (uint8_t)(y >> 8);
    out[15] = (uint8_t)clamp_mouse_axis8(vertical, AP2_BLE_PROTOCOL_MOUSE_LOSS_VERTICAL, loss_flags);
    out[16] = (uint8_t)clamp_mouse_axis8(horizontal, AP2_BLE_PROTOCOL_MOUSE_LOSS_HORIZONTAL, loss_flags);
    out[17] = 0;
    return AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE;
}
