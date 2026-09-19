// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later

#include "chibi.h"

/* Stable ABI exported by the Rust runtime. */
extern void    ch582_rust_usb_start(void);
extern void    ch582_rust_usb_stop(void);
extern bool    ch582_rust_usb_try_write(uint8_t endpoint, const uint8_t *data, size_t size);
extern uint8_t ch582_rust_usb_keyboard_leds(void);

USBDriver USBD1 = {.state = USB_STOP};

void usbInit(void) {
    USBD1.state = USB_READY;
}

void usbStart(USBDriver *usbp, const void *config) {
    (void)config;
    ch582_rust_usb_start();
    usbp->state = USB_ACTIVE;
}

void usbStop(USBDriver *usbp) {
    ch582_rust_usb_stop();
    usbp->state = USB_STOP;
}

void usbConnectBus(USBDriver *usbp) {
    ch582_rust_usb_start();
    usbp->state = USB_ACTIVE;
}

void usbDisconnectBus(USBDriver *usbp) {
    ch582_rust_usb_stop();
    usbp->state = USB_READY;
}

bool usbStartTransmitI(USBDriver *usbp, usbep_t ep, const uint8_t *data, size_t size) {
    return usbp->state == USB_ACTIVE && ch582_rust_usb_try_write(ep, data, size);
}

usbstate_t usbGetDriverStateI(const USBDriver *usbp) {
    return usbp->state;
}

uint8_t usbGetKeyboardLedsI(const USBDriver *usbp) {
    return usbp->state == USB_ACTIVE ? ch582_rust_usb_keyboard_leds() : 0;
}
