#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../annepro2_ble_protocol.h"

#ifndef EXPECTED_BLE_213
#    define EXPECTED_BLE_213 0
#endif

static void assert_bytes(const uint8_t *actual, const uint8_t *expected, uint8_t size) {
    assert(memcmp(actual, expected, size) == 0);
}

static void test_consumer_report(void) {
    uint8_t        out[AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE];
    uint8_t        size;
    const uint16_t usages[] = {0x00E2, 0x00E9, 0x00B5};

    assert(ap2_ble_protocol_encode_consumer(usages, 3, out, &size));
#if EXPECTED_BLE_213
    static const uint8_t expected[] = {0xE2, 0x00, 0xE9, 0x00, 0xB5, 0x00, 0x00, 0x00};
    assert(size == 8);
#else
    static const uint8_t expected[] = {0x13, 0x00, 0x00, 0x00};
    assert(size == 4);
#endif
    assert_bytes(out, expected, size);
    assert(ap2_ble_protocol_encode_consumer(NULL, 0, out, &size));
    for (uint8_t i = 0; i < size; i++) {
        assert(out[i] == 0);
    }
}

static void test_protocol_specific_consumer_limits(void) {
    uint8_t  out[AP2_BLE_PROTOCOL_MAX_CONSUMER_SIZE];
    uint8_t  size;
    uint16_t usages[] = {0x0001, 0x0183, 0x1234, 0xFFFF, 0x0002};

#if EXPECTED_BLE_213
    static const uint8_t expected[] = {0x01, 0x00, 0x83, 0x01, 0x34, 0x12, 0xFF, 0xFF};
    assert(ap2_ble_protocol_encode_consumer(usages, 4, out, &size));
    assert(size == sizeof(expected));
    assert_bytes(out, expected, size);
    assert(!ap2_ble_protocol_encode_consumer(usages, 5, out, &size));
#else
    static const uint16_t all_usages[] = {0x00E2, 0x00E9, 0x00EA, 0x00CD, 0x00B5, 0x00B6, 0x006F, 0x0070};
    assert(ap2_ble_protocol_encode_consumer(all_usages, 8, out, &size));
    assert(size == 4 && out[0] == 0xFF);
    assert(!ap2_ble_protocol_encode_consumer(usages, 1, out, &size));
#endif
}

static void test_mouse_report(void) {
    uint8_t out[AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE];
    uint8_t loss_flags;
    uint8_t size = ap2_ble_protocol_encode_mouse(0x05, -300, 400, -7, 8, out, &loss_flags);

#if EXPECTED_BLE_213
    static const uint8_t expected[] = {0x7B, 0x12, 0x53, 0x00, 0x0A, 0x00, 0x00, 0x7D, 0x60, 0x04, 0x05, 0x00, 0xD4, 0xFE, 0x90, 0x01, 0xF9, 0x00};
    assert(loss_flags == AP2_BLE_PROTOCOL_MOUSE_LOSS_HORIZONTAL);
#else
    static const uint8_t expected[] = {0x7B, 0x12, 0x53, 0x00, 0x0A, 0x00, 0x00, 0x7D, 0x60, 0x04, 0x05, 0xD4, 0xFE, 0x90, 0x01, 0xF9, 0x08, 0x00};
    assert(loss_flags == 0);
#endif
    assert(size == sizeof(expected));
    assert_bytes(out, expected, size);

    size = ap2_ble_protocol_encode_mouse(0, 0, 0, 0, 0, out, &loss_flags);
    assert(size == AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE);
    assert(loss_flags == 0);
    for (uint8_t i = 10; i < size; i++) {
        assert(out[i] == 0);
    }
}

static void test_mouse_loss_is_explicit(void) {
    uint8_t out[AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE];
    uint8_t loss_flags;

    assert(ap2_ble_protocol_encode_mouse(0xF8, 0, 0, 200, -200, out, &loss_flags) == AP2_BLE_PROTOCOL_MOUSE_FRAME_SIZE);
#if EXPECTED_BLE_213
    assert(loss_flags == (AP2_BLE_PROTOCOL_MOUSE_LOSS_VERTICAL | AP2_BLE_PROTOCOL_MOUSE_LOSS_HORIZONTAL));
    assert(out[10] == 0xF8 && out[11] == 0x00);
    assert(out[16] == 0x7F && out[17] == 0x00);
#else
    assert(loss_flags == (AP2_BLE_PROTOCOL_MOUSE_LOSS_BUTTONS | AP2_BLE_PROTOCOL_MOUSE_LOSS_VERTICAL | AP2_BLE_PROTOCOL_MOUSE_LOSS_HORIZONTAL));
    assert(out[10] == 0x00);
    assert(out[15] == 0x7F && out[16] == 0x81 && out[17] == 0x00);
#endif
    assert(ap2_ble_protocol_encode_mouse(0, 0, 0, 0, 0, NULL, &loss_flags) == 0);
    assert(ap2_ble_protocol_encode_mouse(0, 0, 0, 0, 0, out, NULL) == 0);
}

static void test_complete_slot_frames(void) {
    uint8_t out[AP2_BLE_PROTOCOL_MAX_FRAME_SIZE];
    uint8_t size;

#if EXPECTED_BLE_213
    static const uint8_t broadcast_state[] = {0x7B, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7D, 0x20, 0x0B, 0x02, 0x01};
    static const uint8_t connect_state[]   = {0x7B, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7D, 0x20, 0x24, 0x03, 0x02};
    static const uint8_t broadcast[]       = {0x7B, 0x12, 0x53, 0x00, 0x03, 0x00, 0x00, 0x7D, 0x40, 0x01, 0x02};
    static const uint8_t connect[]         = {0x7B, 0x12, 0x53, 0x00, 0x03, 0x00, 0x00, 0x7D, 0x40, 0x04, 0x03};
#else
    static const uint8_t broadcast_state[] = {0x7B, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7D, 0x20, 0x0B, 0x02, 0x01};
    static const uint8_t connect_state[]   = {0x7B, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7D, 0x20, 0x0B, 0x03, 0x00};
    static const uint8_t broadcast[]       = {0x7B, 0x12, 0x53, 0x00, 0x03, 0x00, 0x00, 0x7D, 0x40, 0x01, 0x02, 0x00};
    static const uint8_t connect[]         = {0x7B, 0x12, 0x53, 0x00, 0x03, 0x00, 0x00, 0x7D, 0x40, 0x04, 0x03, 0x00};
#endif

    size = ap2_ble_protocol_encode_slot_state(true, 2, out);
    assert(size == sizeof(broadcast_state));
    assert_bytes(out, broadcast_state, size);
    size = ap2_ble_protocol_encode_slot_state(false, 3, out);
    assert(size == sizeof(connect_state));
    assert_bytes(out, connect_state, size);
    size = ap2_ble_protocol_encode_slot_command(true, 2, out);
    assert(size == sizeof(broadcast));
    assert_bytes(out, broadcast, size);
    size = ap2_ble_protocol_encode_slot_command(false, 3, out);
    assert(size == sizeof(connect));
    assert_bytes(out, connect, size);
    assert(ap2_ble_protocol_encode_slot_command(false, 4, out) == 0);
    assert(ap2_ble_protocol_encode_slot_state(false, 0, NULL) == 0);
}

static void test_preamble_is_model_local(void) {
    ap2_ble_protocol_step_t step;
    ap2_ble_protocol_reset();
#if EXPECTED_BLE_213
    static const uint8_t query[] = {0x7B, 0x12, 0x53, 0x00, 0x02, 0x00, 0x00, 0x7D, 0xC0, 0x17};
    assert(ap2_ble_protocol_begin_command(2, 0, AP2_BLE_ACTION_SEND_CONNECT, 100, &step));
    assert(step.frame_count == 1 && step.frames[0].size == sizeof(query));
    assert_bytes(step.frames[0].data, query, sizeof(query));
    assert(!ap2_ble_protocol_begin_command(2, 1, AP2_BLE_ACTION_SEND_CONNECT, 100, &step));
#else
    assert(!ap2_ble_protocol_begin_command(2, 0, AP2_BLE_ACTION_SEND_CONNECT, 100, &step));
    assert(!ap2_ble_protocol_task(1000, &step));
#endif
}

int main(void) {
    test_consumer_report();
    test_protocol_specific_consumer_limits();
    test_mouse_report();
    test_mouse_loss_is_explicit();
    test_complete_slot_frames();
    test_preamble_is_model_local();
    return 0;
}
