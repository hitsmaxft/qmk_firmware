/*
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

/* Fixed C18D/AP2D BLE 2.13 wire protocol. No BLE 2.05 code is linked here. */
#include "annepro2_ble_protocol.h"

#include "annepro2_ble_213_slot.h"

#include <string.h>

static ap2_ble_213_slot_state_t slot_state;

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

static void reset_step(ap2_ble_protocol_step_t *step) {
    memset(step, 0, sizeof(*step));
}

static ap2_ble_protocol_frame_t *next_step_frame(ap2_ble_protocol_step_t *step) {
    if (step->frame_count >= AP2_BLE_PROTOCOL_MAX_STEP_FRAMES) {
        return NULL;
    }
    return &step->frames[step->frame_count++];
}

static void encode_slot_actions(ap2_ble_213_slot_actions_t actions, ap2_ble_protocol_step_t *step) {
    if (actions & AP2_BLE_213_SLOT_ACTION_QUERY) {
        ap2_ble_protocol_frame_t *frame = next_step_frame(step);
        if (frame != NULL) {
            frame->size = ap2_ble_213_slot_encode_query(frame->data);
        }
    }
    if (actions & AP2_BLE_213_SLOT_ACTION_SELECT) {
        ap2_ble_protocol_frame_t *frame = next_step_frame(step);
        if (frame != NULL) {
            frame->size = ap2_ble_213_slot_encode_select(slot_state.target_slot, frame->data);
        }
    }
    if (actions & AP2_BLE_213_SLOT_ACTION_PREPARE_1) {
        ap2_ble_protocol_frame_t *frame = next_step_frame(step);
        if (frame != NULL) {
            frame->size = ap2_ble_213_slot_encode_prepare(1, frame->data);
        }
    }
    if (actions & AP2_BLE_213_SLOT_ACTION_PREPARE_2) {
        ap2_ble_protocol_frame_t *frame = next_step_frame(step);
        if (frame != NULL) {
            frame->size = ap2_ble_213_slot_encode_prepare(2, frame->data);
        }
    }
}

void ap2_ble_protocol_reset(void) {
    ap2_ble_213_slot_reset(&slot_state);
}

void ap2_ble_protocol_cancel_command(void) {
    ap2_ble_213_slot_reset(&slot_state);
}

bool ap2_ble_protocol_begin_command(uint8_t slot, uint8_t retries, ap2_ble_actions_t deferred_actions, uint32_t now, ap2_ble_protocol_step_t *step) {
    if (retries != 0 || step == NULL) {
        return false;
    }

    reset_step(step);
    encode_slot_actions(ap2_ble_213_slot_begin(&slot_state, slot, deferred_actions, now), step);
    return true;
}

bool ap2_ble_protocol_task(uint32_t now, ap2_ble_protocol_step_t *step) {
    if (step == NULL) {
        return false;
    }

    uint16_t deferred_actions;
    reset_step(step);
    const ap2_ble_213_slot_actions_t actions = ap2_ble_213_slot_task(&slot_state, now, &deferred_actions);
    encode_slot_actions(actions, step);
    if (actions & AP2_BLE_213_SLOT_ACTION_DISPATCH) {
        step->dispatch_actions = (ap2_ble_actions_t)deferred_actions;
    }
    return actions != AP2_BLE_213_SLOT_ACTION_NONE;
}

void ap2_ble_protocol_receive_frame(const uint8_t *frame, uint8_t size) {
    uint8_t slot;
    if (ap2_ble_213_slot_decode_response(frame, size, &slot)) {
        ap2_ble_213_slot_response(&slot_state, slot);
    }
}

uint8_t ap2_ble_protocol_encode_slot_state(bool broadcast, uint8_t slot, uint8_t out[AP2_BLE_PROTOCOL_MAX_FRAME_SIZE]) {
    static const uint8_t prefix[] = {
        0x7B, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7D, 0x20, 0x00,
    };

    if (out == NULL || slot > 3) {
        return 0;
    }

    memcpy(out, prefix, sizeof(prefix));
    out[9]  = broadcast ? 0x0B : 0x24;
    out[10] = slot;
    out[11] = broadcast ? 1 : 2;
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
    return 11;
}

bool ap2_ble_protocol_encode_consumer(const uint16_t *usages, size_t usage_count, uint8_t out[AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE], uint8_t *out_size) {
    if (out == NULL || out_size == NULL || usage_count > 4 || (usage_count > 0 && usages == NULL)) {
        return false;
    }

    memset(out, 0, AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE);
    for (size_t i = 0; i < usage_count; i++) {
        out[i * 2]     = (uint8_t)usages[i];
        out[i * 2 + 1] = (uint8_t)(usages[i] >> 8);
    }
    *out_size = 8;
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

    /* AP2D KEY 3.08 report ID 2: 16 buttons, X/Y, wheel, pad. */
    out[10] = buttons;
    out[11] = 0;
    out[12] = (uint8_t)x;
    out[13] = (uint8_t)(x >> 8);
    out[14] = (uint8_t)y;
    out[15] = (uint8_t)(y >> 8);
    out[16] = (uint8_t)clamp_mouse_axis8(vertical, AP2_BLE_PROTOCOL_MOUSE_LOSS_VERTICAL, loss_flags);
    out[17] = 0;
    if (horizontal != 0) {
        *loss_flags |= AP2_BLE_PROTOCOL_MOUSE_LOSS_HORIZONTAL;
    }
    return AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE;
}
