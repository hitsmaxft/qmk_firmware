// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#include "chibi.h"
#include "host.h"
#include "host_driver.h"
#include "report.h"

#define KEYBOARD_ENDPOINT 1u

static uint8_t keyboard_leds(void) {
    return usbGetKeyboardLedsI(&USBD1);
}
static void send_keyboard(report_keyboard_t *report) {
    (void)usbStartTransmitI(&USBD1, KEYBOARD_ENDPOINT, (const uint8_t *)report, sizeof(*report));
}
static void send_nkro(report_nkro_t *report) {
    (void)report;
}
static void send_mouse(report_mouse_t *report) {
    (void)report;
}
static void send_extra(report_extra_t *report) {
    (void)report;
}

static host_driver_t chibios_rust_driver = {
    .keyboard_leds = keyboard_leds,
    .send_keyboard = send_keyboard,
    .send_nkro     = send_nkro,
    .send_mouse    = send_mouse,
    .send_extra    = send_extra,
};

void protocol_setup(void) {}
void protocol_pre_init(void) {
    usbInit();
    usbStart(&USBD1, NULL);
    usbConnectBus(&USBD1);
}
void protocol_post_init(void) {
    host_set_driver(&chibios_rust_driver);
}
void protocol_pre_task(void) {}
void protocol_post_task(void) {}
void protocol_keyboard_task(void) {
    extern void keyboard_task(void);
    keyboard_task();
}
