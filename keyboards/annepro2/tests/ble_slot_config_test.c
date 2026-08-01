#include <assert.h>
#include <stdint.h>

#include "../annepro2_ble_slot_config.h"

static void test_roundtrip(void) {
    const uint8_t tags[] = {0x18, 0x1D, 0x2D};
    for (uint8_t tag_index = 0; tag_index < sizeof(tags); tag_index++) {
        for (int8_t slot = -1; slot <= 3; slot++) {
            uint32_t config;
            int8_t   decoded_slot;
            assert(annepro2_ble_slot_config_encode(tags[tag_index], slot, &config));
            assert(annepro2_ble_slot_config_decode(tags[tag_index], config, &decoded_slot));
            assert(decoded_slot == slot);
            assert(!annepro2_ble_slot_config_decode(tags[tag_index], config ^ 0x00000100, &decoded_slot));
        }
    }
}

static void test_model_isolation_and_legacy_rejection(void) {
    uint32_t config;
    int8_t   slot;
    assert(annepro2_ble_slot_config_encode(0x18, 2, &config));
    assert(!annepro2_ble_slot_config_decode(0x1D, config, &slot));
    assert(!annepro2_ble_slot_config_decode(0x2D, config, &slot));

    /* Removed dual-profile format: A2 01 checksum payload. */
    assert(!annepro2_ble_slot_config_decode(0x18, 0xA201FB02, &slot));
    assert(!annepro2_ble_slot_config_decode(0x1D, 0xA201F30A, &slot));
    assert(!annepro2_ble_slot_config_decode(0x18, 3, &slot));
}

static void test_invalid_inputs(void) {
    uint32_t config;
    int8_t   slot;
    assert(!annepro2_ble_slot_config_encode(0, 0, &config));
    assert(!annepro2_ble_slot_config_encode(0x18, -2, &config));
    assert(!annepro2_ble_slot_config_encode(0x18, 4, &config));
    assert(!annepro2_ble_slot_config_encode(0x18, 0, NULL));
    assert(!annepro2_ble_slot_config_decode(0, 0, &slot));
    assert(!annepro2_ble_slot_config_decode(0x18, 0, NULL));
}

int main(void) {
    test_roundtrip();
    test_model_isolation_and_legacy_rejection();
    test_invalid_inputs();
    return 0;
}
