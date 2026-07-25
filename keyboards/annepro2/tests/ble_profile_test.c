#include <assert.h>
#include <stdint.h>

#include "../annepro2_ble_profile.h"

static void test_consumer_profiles(void) {
    uint8_t  out[ANNEPRO2_BLE_CONSUMER_MAX_SIZE];
    uint8_t  size;
    uint16_t usages[] = {0x00E2, 0x00E9, 0x00B5};

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_C18_205, usages, 3, out, &size));
    assert(size == 4);
    assert(out[0] == 0x13 && out[1] == 0 && out[2] == 0 && out[3] == 0);

    assert(annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_AP2D_213, usages, 3, out, &size));
    assert(size == 8);
    assert(out[0] == 0xE2 && out[1] == 0x00);
    assert(out[2] == 0xE9 && out[3] == 0x00);
    assert(out[4] == 0xB5 && out[5] == 0x00);
    assert(out[6] == 0x00 && out[7] == 0x00);

    uint16_t unsupported = 0x0183;
    assert(!annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_C18_205, &unsupported, 1, out, &size));
    assert(!annepro2_ble_encode_consumer(ANNEPRO2_BLE_PROFILE_AP2D_213, usages, 5, out, &size));
}

static void test_led_forms(void) {
    uint8_t leds;
    uint8_t one[] = {0x02};
    uint8_t two[] = {0x01, 0x05};
    uint8_t bad[] = {0x02, 0x05};

    assert(annepro2_ble_decode_leds(one, sizeof(one), &leds) && leds == 0x02);
    assert(annepro2_ble_decode_leds(two, sizeof(two), &leds) && leds == 0x05);
    assert(!annepro2_ble_decode_leds(bad, sizeof(bad), &leds));
    assert(!annepro2_ble_decode_leds(one, 0, &leds));
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
    test_led_forms();
    test_config_roundtrip_and_corruption();
    return 0;
}
