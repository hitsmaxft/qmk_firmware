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

#include "annepro2_ble_state.h"

#define AP2_BLE_PROTOCOL_MAX_FRAME_SIZE 12
#define AP2_BLE_PROTOCOL_MAX_STEP_FRAMES 2
#define AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE 8
#define AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE 18

#define AP2_BLE_PROTOCOL_MOUSE_LOSS_BUTTONS 0x01
#define AP2_BLE_PROTOCOL_MOUSE_LOSS_VERTICAL 0x02
#define AP2_BLE_PROTOCOL_MOUSE_LOSS_HORIZONTAL 0x04

typedef struct {
    uint8_t data[AP2_BLE_PROTOCOL_MAX_FRAME_SIZE];
    uint8_t size;
} ap2_ble_protocol_frame_t;

typedef struct {
    ap2_ble_protocol_frame_t frames[AP2_BLE_PROTOCOL_MAX_STEP_FRAMES];
    uint8_t                  frame_count;
    ap2_ble_actions_t        dispatch_actions;
} ap2_ble_protocol_step_t;

/* Model-specific lifecycle for any command preamble. */
void ap2_ble_protocol_reset(void);
void ap2_ble_protocol_cancel_command(void);
bool ap2_ble_protocol_begin_command(uint8_t slot, uint8_t retries, ap2_ble_actions_t deferred_actions, uint32_t now, ap2_ble_protocol_step_t *step);
bool ap2_ble_protocol_task(uint32_t now, ap2_ble_protocol_step_t *step);
void ap2_ble_protocol_receive_frame(const uint8_t *frame, uint8_t size);

/* Fixed wire encoders supplied by exactly one model protocol object. */
uint8_t ap2_ble_protocol_encode_slot_state(bool broadcast, uint8_t slot, uint8_t out[AP2_BLE_PROTOCOL_MAX_FRAME_SIZE]);
uint8_t ap2_ble_protocol_encode_slot_command(bool broadcast, uint8_t slot, uint8_t out[AP2_BLE_PROTOCOL_MAX_FRAME_SIZE]);
bool    ap2_ble_protocol_encode_consumer(const uint16_t *usages, size_t usage_count, uint8_t out[AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE], uint8_t *out_size);
uint8_t ap2_ble_protocol_encode_mouse(uint8_t buttons, int16_t x, int16_t y, int16_t vertical, int16_t horizontal, uint8_t out[AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE], uint8_t *loss_flags);
