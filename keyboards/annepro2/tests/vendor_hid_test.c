#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../annepro2_vendor_hid.h"

static const annepro2_vendor_hid_versions_t versions = {
    .key_major = 2,
    .key_minor = 37,
    .led_major = 2,
    .led_minor = 33,
    .ble_major = 2,
    .ble_minor = 13,
};

static void test_get_version(void) {
    uint8_t request[ANNEPRO2_VENDOR_HID_REPORT_SIZE] = {
        0x7B, 0x10, 0x31, 0x10, 0x02, 0x00, 0x00, 0x7D, 0x01, 0x03,
    };
    uint8_t response[ANNEPRO2_VENDOR_HID_REPORT_SIZE];
    uint8_t expected[] = {
        0x7B, 0x10, 0x13, 0x10, 0x08, 0x00, 0x00, 0x7D, 0x01, 0x03, 0x25, 0x02, 0x21, 0x02, 0x0D, 0x02,
    };

    memset(response, 0xA5, sizeof(response));
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_REPLY);
    assert(memcmp(response, expected, sizeof(expected)) == 0);
    for (size_t i = sizeof(expected); i < sizeof(response); i++) {
        assert(response[i] == 0);
    }
}

static void test_enter_iap(void) {
    uint8_t request[ANNEPRO2_VENDOR_HID_REPORT_SIZE] = {
        0x7B, 0x10, 0x31, 0x10, 0x03, 0x00, 0x00, 0x7D, 0x02, 0x01, 0x01,
    };
    uint8_t response[ANNEPRO2_VENDOR_HID_REPORT_SIZE];
    uint8_t expected[] = {
        0x7B, 0x10, 0x13, 0x10, 0x03, 0x00, 0x00, 0x7D, 0x02, 0x01, 0x00,
    };

    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_REPLY_ENTER_IAP);
    assert(memcmp(response, expected, sizeof(expected)) == 0);
    for (size_t i = sizeof(expected); i < sizeof(response); i++) {
        assert(response[i] == 0);
    }
}

static void test_rejects_malformed_and_unknown_reports(void) {
    uint8_t request[ANNEPRO2_VENDOR_HID_REPORT_SIZE] = {
        0x7B, 0x10, 0x31, 0x10, 0x02, 0x00, 0x00, 0x7D, 0x01, 0x03,
    };
    uint8_t response[ANNEPRO2_VENDOR_HID_REPORT_SIZE];

    assert(annepro2_vendor_hid_handle(NULL, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, NULL) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    assert(annepro2_vendor_hid_handle(request, 9, &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);

    request[0] = 0;
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    request[0] = 0x7B;

    request[2] = 0x13;
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    request[2] = 0x31;

    request[3] = 0;
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    request[3] = 0x10;

    request[7] = 0;
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    request[7] = 0x7D;

    request[4] = 65;
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    request[4] = 2;

    request[6] = 1;
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    request[6] = 0;

    request[9] = 4;
    assert(annepro2_vendor_hid_handle(request, sizeof(request), &versions, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
    request[9] = 3;

    assert(annepro2_vendor_hid_handle(request, sizeof(request), NULL, response) == ANNEPRO2_VENDOR_HID_UNHANDLED);
}

int main(void) {
    test_get_version();
    test_enter_iap();
    test_rejects_malformed_and_unknown_reports();
    return 0;
}
