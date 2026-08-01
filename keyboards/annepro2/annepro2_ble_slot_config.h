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

/*
 * Store only a fixed-model slot selection. The target tag prevents a slot
 * from one physical BLE module from being replayed after cross-flashing a
 * different Anne Pro 2 model. The removed dual-profile EEPROM records use a
 * different second byte and are therefore intentionally invalid.
 */
bool annepro2_ble_slot_config_encode(uint8_t target_tag, int8_t slot, uint32_t *config);
bool annepro2_ble_slot_config_decode(uint8_t target_tag, uint32_t config, int8_t *slot);
