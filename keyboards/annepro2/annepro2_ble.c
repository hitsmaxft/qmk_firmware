/*
    Copyright (C) 2020 Yaotian Feng, Codetector<codetector@codetector.cn>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "annepro2_ble.h"
#include "ch.h"
#include "hal.h"
#include "host.h"
#include "host_driver.h"
#include "print.h"
#include "report.h"
#include "timer.h"

#define AP2_BLE_RX_HEADER_SIZE 8
#define AP2_BLE_RX_MAX_PAYLOAD 32
#define AP2_BLE_RX_MAX_FRAME_SIZE (AP2_BLE_RX_HEADER_SIZE + AP2_BLE_RX_MAX_PAYLOAD)

#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
#    define AP2_BLE_LOG(fmt, ...) uprintf("AP2 BLE %08lX " fmt "\n", (unsigned long)timer_read32(), ##__VA_ARGS__)
#else
#    define AP2_BLE_LOG(fmt, ...)
#endif

/* -------------------- Static Function Prototypes -------------------------- */
static uint8_t ap2_ble_leds(void);
static void    ap2_ble_mouse(report_mouse_t *report);
static void    ap2_ble_extra(report_extra_t *report);
static void    ap2_ble_keyboard(report_keyboard_t *report);

static void ap2_ble_switch_ble_driver(void);
static void ap2_ble_handle_rx_frame(const uint8_t *frame, uint8_t size);
static void ap2_ble_reset_rx_parser(uint8_t byte);
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
static void ap2_ble_log_rx_frame(const uint8_t *frame, uint8_t size);
#endif

/* -------------------- Static Local Variables ------------------------------ */
static host_driver_t ap2_ble_driver = {ap2_ble_leds, ap2_ble_keyboard, NULL, ap2_ble_mouse, ap2_ble_extra};

static uint8_t ble_mcu_wakeup[11] = {0x7b, 0x12, 0x53, 0x00, 0x03, 0x00, 0x01, 0x7d, 0x02, 0x01, 0x02};

static uint8_t ble_mcu_start_broadcast[10] = {
    0x7b, 0x12, 0x53, 0x00, 0x03, 0x00, 0x00, 0x7d, 0x40, 0x01, // Broadcast ID[0-3]
};

static uint8_t ble_mcu_connect[10] = {
    0x7b, 0x12, 0x53, 0x00, 0x03, 0x00, 0x00, 0x7d, 0x40, 0x04, // Connect ID [0-3]
};

static uint8_t ble_mcu_send_report[10] = {
    0x7b, 0x12, 0x53, 0x00, 0x0A, 0x00, 0x00, 0x7d, 0x10, 0x04,
};

static uint8_t ble_mcu_send_consumer_report[10] = {
    0x7b, 0x12, 0x53, 0x00, 0x06, 0x00, 0x00, 0x7d, 0x10, 0x08,
};

static uint8_t ble_mcu_unpair[10] = {
    0x7b, 0x12, 0x53, 0x00, 0x02, 0x00, 0x00, 0x7d, 0x40, 0x05,
};

/* Reply sent by the official keyboard MCU after a BLE 0x20/0x0c request. */
static uint8_t ble_mcu_hid_handshake_response[12] = {
    0x7b, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7d, 0x20, 0x0c, 0x00, 0x00,
};

static uint8_t ble_mcu_bootload[11] = {0x7b, 0x10, 0x51, 0x10, 0x03, 0x00, 0x00, 0x7d, 0x02, 0x01, 0x01};

static host_driver_t *last_host_driver = NULL;
static int8_t         last_broadcast   = -1;
static bool           ble_route_requested;
static uint8_t        ble_rx_frame[AP2_BLE_RX_MAX_FRAME_SIZE];
static uint8_t        ble_rx_frame_size;
static uint8_t        ble_rx_expected_size;
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
static uint8_t ble_debug_keyboard_reports;
#endif
#ifdef NKRO_ENABLE
static bool lastNkroStatus = false;
#endif // NKRO_ENABLE

static void ap2_ble_begin_route_request(void) {
    /* Do not leave reports routed to an old BLE link while selecting a slot. */
    if (host_get_driver() == &ap2_ble_driver) {
        clear_keyboard();
#ifdef NKRO_ENABLE
        keymap_config.nkro = lastNkroStatus;
#endif
        host_set_driver(last_host_driver);
        AP2_BLE_LOG("route pending");
    }
    ble_route_requested = true;
}

/* -------------------- Public Function Implementation ---------------------- */

void annepro2_ble_bootload(void) {
    sdWrite(&SD1, ble_mcu_bootload, sizeof(ble_mcu_bootload));
}

void annepro2_ble_startup(void) {
    AP2_BLE_LOG("tx wakeup");
    sdWrite(&SD1, ble_mcu_wakeup, sizeof(ble_mcu_wakeup));
}

void annepro2_ble_broadcast(uint8_t port) {
    if (port > 3) {
        port = 3;
    }
    const bool reconnect = last_broadcast == (int8_t)port;

    AP2_BLE_LOG("tx broadcast slot=%u reconnect=%u previous_slot=%d", port, reconnect, last_broadcast);
    ap2_ble_begin_route_request();
    sdWrite(&SD1, ble_mcu_start_broadcast, sizeof(ble_mcu_start_broadcast));
    sdPut(&SD1, port);
    sdPut(&SD1, 0x00);
    if (reconnect) {
        annepro2_ble_connect(port);
    }
    last_broadcast = port;
}

void annepro2_ble_connect(uint8_t port) {
    if (port > 3) {
        port = 3;
    }
    ap2_ble_begin_route_request();
    AP2_BLE_LOG("tx connect slot=%u", port);
    sdWrite(&SD1, ble_mcu_connect, sizeof(ble_mcu_connect));
    sdPut(&SD1, port);
    sdPut(&SD1, 0x00);
}

void annepro2_ble_disconnect(void) {
    ble_route_requested = false;
    AP2_BLE_LOG("route usb ble_driver=%u", host_get_driver() == &ap2_ble_driver);

    /* This only changes QMK's output route; it does not disconnect the module. */
    /* Skip if the driver is already enabled */
    if (host_get_driver() != &ap2_ble_driver) {
        return;
    }

    clear_keyboard();
#ifdef NKRO_ENABLE
    keymap_config.nkro = lastNkroStatus;
#endif
    host_set_driver(last_host_driver);
}

void annepro2_ble_unpair(void) {
    AP2_BLE_LOG("tx unpair");
    sdWrite(&SD1, ble_mcu_unpair, sizeof(ble_mcu_unpair));
    last_broadcast = -1;
    annepro2_ble_disconnect();
}

void annepro2_ble_rx_byte(uint8_t byte) {
    if (ble_rx_frame_size == 0) {
        if (byte == 0x7b) {
            ble_rx_frame[ble_rx_frame_size++] = byte;
        }
        return;
    }

    ble_rx_frame[ble_rx_frame_size++] = byte;

    if (ble_rx_frame_size == 5) {
        const uint8_t payload_size = ble_rx_frame[4];
        if (payload_size > AP2_BLE_RX_MAX_PAYLOAD) {
            AP2_BLE_LOG("rx invalid payload_len=%u", payload_size);
            ap2_ble_reset_rx_parser(byte);
            return;
        }
        ble_rx_expected_size = AP2_BLE_RX_HEADER_SIZE + payload_size;
    }

    if (ble_rx_frame_size == AP2_BLE_RX_HEADER_SIZE && ble_rx_frame[7] != 0x7d) {
        AP2_BLE_LOG("rx invalid delimiter=%02X", ble_rx_frame[7]);
        ap2_ble_reset_rx_parser(byte);
        return;
    }

    if (ble_rx_expected_size != 0 && ble_rx_frame_size == ble_rx_expected_size) {
        ap2_ble_handle_rx_frame(ble_rx_frame, ble_rx_frame_size);
        ble_rx_frame_size    = 0;
        ble_rx_expected_size = 0;
    }
}

/* ------------------- Static Function Implementation ----------------------- */
static void ap2_ble_switch_ble_driver(void) {
    if (host_get_driver() == &ap2_ble_driver) {
        return;
    }
    clear_keyboard();
    last_host_driver = host_get_driver();
#ifdef NKRO_ENABLE
    lastNkroStatus = keymap_config.nkro;
#endif
    keymap_config.nkro = false;
    host_set_driver(&ap2_ble_driver);
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
    ble_debug_keyboard_reports = 0;
#endif
    AP2_BLE_LOG("route ble");
}

static void ap2_ble_handle_rx_frame(const uint8_t *frame, uint8_t size) {
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
    ap2_ble_log_rx_frame(frame, size);
#endif

    /* Preserve the existing Caps Lock ABI until its event opcode is known. */
    if (size == sizeof(ble_capslock)) {
        for (uint8_t i = 0; i < sizeof(ble_capslock); i++) {
            ((uint8_t *)&ble_capslock)[i] = frame[i];
        }
    }

    if (size >= 11 && frame[1] == 0x12 && frame[2] == 0x35) {
        AP2_BLE_LOG("rx decoded group=%02X command=%02X value=%02X requested=%u", frame[8], frame[9], frame[10], ble_route_requested);

        /*
         * The official keyboard firmware answers an incoming 0x20/0x0c with
         * a 0x20/0x0c response before continuing. Hardware traces show this
         * handshake only after macOS has completed the BLE/HID setup.
         * 0x40/0x01 and 0x40/0x04 are command acknowledgements and must not
         * change QMK's host driver.
         */
        if (frame[4] == 0x03 && frame[5] == 0x00 && frame[8] == 0x20 && frame[9] == 0x0c) {
            AP2_BLE_LOG("tx hid handshake response");
            sdWrite(&SD1, ble_mcu_hid_handshake_response, sizeof(ble_mcu_hid_handshake_response));
            if (ble_route_requested) {
                AP2_BLE_LOG("rx hid handshake ready");
                ap2_ble_switch_ble_driver();
            }
        }
    } else {
        AP2_BLE_LOG("rx frame len=%u type=%02X/%02X", size, frame[1], frame[2]);
    }
}

#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
static void ap2_ble_log_rx_frame(const uint8_t *frame, uint8_t size) {
    uprintf("AP2 BLE %08lX rx%u", (unsigned long)timer_read32(), size);
    for (uint8_t i = 0; i < size; i++) {
        uprintf(" %02X", frame[i]);
    }
    uprintf("\n");
}
#endif

static void ap2_ble_reset_rx_parser(uint8_t byte) {
    ble_rx_frame_size    = 0;
    ble_rx_expected_size = 0;
    if (byte == 0x7b) {
        ble_rx_frame[ble_rx_frame_size++] = byte;
    }
}

static uint8_t ap2_ble_leds(void) {
    return 0; // TODO: Figure out how to obtain LED status
}

static void ap2_ble_mouse(report_mouse_t *report) {}

static inline uint16_t CONSUMER2AP2(uint16_t usage) {
    switch (usage) {
        case AUDIO_VOL_DOWN:
            return 0x04;
        case AUDIO_VOL_UP:
            return 0x02;
        case AUDIO_MUTE:
            return 0x01;
        case TRANSPORT_PLAY_PAUSE:
            return 0x08;
        case TRANSPORT_NEXT_TRACK:
            return 0x10;
        case TRANSPORT_PREV_TRACK:
            return 0x20;
        default:
            return 0x00;
    }
}

static void ap2_ble_extra(report_extra_t *report) {
    if (report->report_id == REPORT_ID_CONSUMER) {
        sdPut(&SD1, 0x0);
        sdWrite(&SD1, ble_mcu_send_consumer_report, sizeof(ble_mcu_send_consumer_report));
        sdPut(&SD1, CONSUMER2AP2(report->usage));
        static const uint8_t dummy[3] = {0};
        sdWrite(&SD1, dummy, sizeof(dummy));
    }
}

/*!
 * @brief  Send keyboard HID report for Bluetooth driver
 */
static void ap2_ble_keyboard(report_keyboard_t *report) {
    sdPut(&SD1, 0x0);
    sdWrite(&SD1, ble_mcu_send_report, sizeof(ble_mcu_send_report));
    sdWrite(&SD1, (uint8_t *)report, KEYBOARD_REPORT_SIZE);
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
    if (ble_debug_keyboard_reports < 3) {
        ble_debug_keyboard_reports++;
        AP2_BLE_LOG("tx keyboard report=%u", ble_debug_keyboard_reports);
    }
#endif
}
