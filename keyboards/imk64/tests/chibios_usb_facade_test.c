// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chibi.h"

static unsigned starts;
static unsigned stops;
static uint8_t  last_endpoint;
static uint8_t  last_report[8];
static uint8_t  leds = 0x02;

void ch582_rust_usb_start(void) {
    ++starts;
}
void ch582_rust_usb_stop(void) {
    ++stops;
}
bool ch582_rust_usb_try_write(uint8_t endpoint, const uint8_t *data, size_t size) {
    if (size != sizeof(last_report)) {
        return false;
    }
    last_endpoint = endpoint;
    memcpy(last_report, data, size);
    return true;
}
uint8_t ch582_rust_usb_keyboard_leds(void) {
    return leds;
}

int main(void) {
    static const uint8_t report[8] = {0x02, 0, 0x04, 0, 0, 0, 0, 0};

    assert(usbGetDriverStateI(&USBD1) == USB_STOP);
    usbInit();
    assert(usbGetDriverStateI(&USBD1) == USB_READY);
    usbStart(&USBD1, NULL);
    assert(starts == 1);
    assert(usbGetDriverStateI(&USBD1) == USB_ACTIVE);
    assert(usbStartTransmitI(&USBD1, 1, report, sizeof(report)));
    assert(last_endpoint == 1);
    assert(memcmp(last_report, report, sizeof(report)) == 0);
    assert(usbGetKeyboardLedsI(&USBD1) == leds);
    usbDisconnectBus(&USBD1);
    assert(stops == 1);
    assert(usbGetDriverStateI(&USBD1) == USB_READY);
    assert(!usbStartTransmitI(&USBD1, 1, report, sizeof(report)));
    return 0;
}
