/*
 * Copyright 2026 BHE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "annepro2_vendor_hid.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define AP2_FRAME_HEADER_SIZE 8

#define AP2_ROUTE_USB_TO_MAIN 0x31
#define AP2_ROUTE_MAIN_TO_USB 0x13

#define AP2_GLOBAL 0x01
#define AP2_GLOBAL_GET_VERSION 0x03

#define AP2_FW 0x02
#define AP2_FW_IAP_MODE 0x01
#define AP2_FW_ENTER_IAP 0x01

static bool ap2_vendor_hid_request_valid(const uint8_t *request, uint8_t length, uint32_t *payload_length) {
    if (request == NULL || length < AP2_FRAME_HEADER_SIZE) {
        return false;
    }

    if (request[0] != 0x7B || request[1] != 0x10 || request[2] != AP2_ROUTE_USB_TO_MAIN || request[3] != 0x10 || request[7] != 0x7D) {
        return false;
    }

    *payload_length = (uint32_t)request[4] | ((uint32_t)request[5] << 8) | ((uint32_t)request[6] << 16);
    return *payload_length <= (uint32_t)length - AP2_FRAME_HEADER_SIZE;
}

static void ap2_vendor_hid_begin_response(const uint8_t *request, uint8_t payload_length, uint8_t response[ANNEPRO2_VENDOR_HID_REPORT_SIZE]) {
    memset(response, 0, ANNEPRO2_VENDOR_HID_REPORT_SIZE);
    memcpy(response, request, AP2_FRAME_HEADER_SIZE);
    response[2] = AP2_ROUTE_MAIN_TO_USB;
    response[4] = payload_length;
    response[5] = 0;
    response[6] = 0;
}

annepro2_vendor_hid_result_t annepro2_vendor_hid_handle(const uint8_t *request, uint8_t length, const annepro2_vendor_hid_versions_t *versions, uint8_t response[ANNEPRO2_VENDOR_HID_REPORT_SIZE]) {
    uint32_t payload_length;

    if (response == NULL || !ap2_vendor_hid_request_valid(request, length, &payload_length)) {
        return ANNEPRO2_VENDOR_HID_UNHANDLED;
    }

    if (payload_length == 2 && request[8] == AP2_GLOBAL && request[9] == AP2_GLOBAL_GET_VERSION) {
        if (versions == NULL) {
            return ANNEPRO2_VENDOR_HID_UNHANDLED;
        }

        ap2_vendor_hid_begin_response(request, 8, response);
        response[8]  = AP2_GLOBAL;
        response[9]  = AP2_GLOBAL_GET_VERSION;
        response[10] = versions->key_minor;
        response[11] = versions->key_major;
        response[12] = versions->led_minor;
        response[13] = versions->led_major;
        response[14] = versions->ble_minor;
        response[15] = versions->ble_major;
        return ANNEPRO2_VENDOR_HID_REPLY;
    }

    if (payload_length == 3 && request[8] == AP2_FW && request[9] == AP2_FW_IAP_MODE && request[10] == AP2_FW_ENTER_IAP) {
        ap2_vendor_hid_begin_response(request, 3, response);
        response[8]  = AP2_FW;
        response[9]  = AP2_FW_IAP_MODE;
        response[10] = 0;
        return ANNEPRO2_VENDOR_HID_REPLY_ENTER_IAP;
    }

    return ANNEPRO2_VENDOR_HID_UNHANDLED;
}
