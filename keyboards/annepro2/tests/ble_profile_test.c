#include <assert.h>
#include <stdint.h>

#include "../annepro2_ble_profile.h"

static void test_consumer_profiles(void) {
    uint8_t  out[ANNEPRO2_BLE_CONSUMER_MAX_SIZE];
    uint8_t  size;
    uint16_t usages[]      = {0x00E2, 0x00E9, 0x00B5};
    uint16_t four_usages[] = {0x00E2, 0x00E9, 0x00B5, 0x00CD};

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_C18_205, usages, 3, out, &size));
    assert(size == 4);
    assert(out[0] == 0x13 && out[1] == 0 && out[2] == 0 && out[3] == 0);

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_AP2D_213, usages, 3, out, &size));
    assert(size == 8);
    assert(out[0] == 0xE2 && out[1] == 0x00);
    assert(out[2] == 0xE9 && out[3] == 0x00);
    assert(out[4] == 0xB5 && out[5] == 0x00);
    assert(out[6] == 0x00 && out[7] == 0x00);

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_C18_205, NULL, 0, out, &size));
    assert(size == 4 && out[0] == 0 && out[1] == 0 && out[2] == 0 && out[3] == 0);

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_AP2D_213, four_usages, 4, out, &size));
    assert(size == 8);
    assert(out[0] == 0xE2 && out[1] == 0x00);
    assert(out[2] == 0xE9 && out[3] == 0x00);
    assert(out[4] == 0xB5 && out[5] == 0x00);
    assert(out[6] == 0xCD && out[7] == 0x00);

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_AP2D_213, NULL, 0, out, &size));
    assert(size == 8);
    for (uint8_t i = 0; i < size; i++) {
        assert(out[i] == 0);
    }

    uint16_t unsupported = 0x0183;
    assert(!annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_C18_205, &unsupported, 1, out, &size));
    assert(!annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_AP2D_213, four_usages, 5, out, &size));
    assert(!annepro2_ble_encode_consumer((annepro2_ble_profile_t)2, usages, 1, out, &size));
}

static void test_consumer_205_all_bits(void) {
    static const uint16_t usages[] = {
        0x00E2, // Mute
        0x00E9, // Volume up
        0x00EA, // Volume down
        0x00CD, // Play/pause
        0x00B5, // Next track
        0x00B6, // Previous track
        0x006F, // Brightness up
        0x0070, // Brightness down
    };
    uint8_t out[ANNEPRO2_BLE_CONSUMER_MAX_SIZE];
    uint8_t size;

    for (uint8_t bit = 0; bit < 8; bit++) {
        assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_C18_205, &usages[bit], 1, out, &size));
        assert(size == ANNEPRO2_BLE_CONSUMER_205_SIZE);
        assert(out[0] == (uint8_t)(1U << bit));
        assert(out[1] == 0 && out[2] == 0 && out[3] == 0);
    }

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_C18_205, usages, 8, out, &size));
    assert(size == ANNEPRO2_BLE_CONSUMER_205_SIZE);
    assert(out[0] == 0xFF);
}

static void test_consumer_213_preserves_usage_order(void) {
    const uint16_t usages[]   = {0x0001, 0x0183, 0x1234, 0xFFFF};
    const uint8_t  expected[] = {0x01, 0x00, 0x83, 0x01, 0x34, 0x12, 0xFF, 0xFF};
    uint8_t        out[ANNEPRO2_BLE_CONSUMER_MAX_SIZE];
    uint8_t        size;

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_AP2D_213, usages, 4, out, &size));
    assert(size == ANNEPRO2_BLE_CONSUMER_213_SIZE);
    for (uint8_t i = 0; i < size; i++) {
        assert(out[i] == expected[i]);
    }
}

static void test_slot_state_profiles(void) {
    annepro2_ble_slot_state_t state;

    assert(annepro2_ble_encode_slot_state(ANNEPRO2_BLE_PROFILE_C18_205, true, &state));
    assert(state.command == 0x0B && state.action == 1);
    assert(annepro2_ble_encode_slot_state(ANNEPRO2_BLE_PROFILE_C18_205, false, &state));
    assert(state.command == 0x0B && state.action == 0);

    assert(annepro2_ble_encode_slot_state(ANNEPRO2_BLE_PROFILE_AP2D_213, true, &state));
    assert(state.command == 0x0B && state.action == 1);
    assert(annepro2_ble_encode_slot_state(ANNEPRO2_BLE_PROFILE_AP2D_213, false, &state));
    assert(state.command == 0x24 && state.action == 2);

    assert(!annepro2_ble_encode_slot_state((annepro2_ble_profile_t)2, true, &state));
}

static void test_config_roundtrip_and_corruption(void) {
    for (int profile = ANNEPRO2_BLE_PROFILE_C18_205; profile <= ANNEPRO2_BLE_PROFILE_AP2D_213; profile++) {
        for (int slot = -1; slot <= 3; slot++) {
            uint32_t               config;
            annepro2_ble_profile_t decoded_profile;
            int8_t                 decoded_slot;
            assert(annepro2_ble_encode_config((annepro2_ble_profile_t)profile, slot, &config));
            assert(annepro2_ble_decode_config(config, &decoded_profile, &decoded_slot));
            assert(decoded_profile == (annepro2_ble_profile_t)profile);
            assert(decoded_slot == slot);
            assert(!annepro2_ble_decode_config(config ^ 0x00000100, &decoded_profile, &decoded_slot));
            assert(!annepro2_ble_decode_config(config ^ 0x01000000, &decoded_profile, &decoded_slot));
        }
    }

    uint32_t ignored;
    assert(!annepro2_ble_encode_config(ANNEPRO2_BLE_PROFILE_C18_205, -2, &ignored));
    assert(!annepro2_ble_encode_config(ANNEPRO2_BLE_PROFILE_AP2D_213, 4, &ignored));
    assert(!annepro2_ble_decode_config(0, &(annepro2_ble_profile_t){0}, &(int8_t){0}));
}

int main(void) {
    test_consumer_profiles();
    test_consumer_205_all_bits();
    test_consumer_213_preserves_usage_order();
    test_slot_state_profiles();
    test_config_roundtrip_and_corruption();
    return 0;
}
