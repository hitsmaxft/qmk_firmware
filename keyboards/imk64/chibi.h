// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t usbep_t;

typedef enum {
    USB_STOP = 0,
    USB_READY,
    USB_ACTIVE,
    USB_SUSPENDED,
} usbstate_t;

typedef struct {
    volatile usbstate_t state;
} USBDriver;

extern USBDriver USBD1;

void       usbInit(void);
void       usbStart(USBDriver *usbp, const void *config);
void       usbStop(USBDriver *usbp);
void       usbConnectBus(USBDriver *usbp);
void       usbDisconnectBus(USBDriver *usbp);
bool       usbStartTransmitI(USBDriver *usbp, usbep_t ep, const uint8_t *data, size_t size);
usbstate_t usbGetDriverStateI(const USBDriver *usbp);
uint8_t    usbGetKeyboardLedsI(const USBDriver *usbp);
