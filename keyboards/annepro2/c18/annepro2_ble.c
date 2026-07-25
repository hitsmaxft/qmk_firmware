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

/* C18-only BLE 2.05/2.13 compatibility implementation. */
#include "annepro2_ble.h"
#include "annepro2_ble_parser.h"
#include "annepro2_ble_profile.h"
#include "annepro2_ble_state.h"
#include "ch.h"
#include "eeconfig.h"
#include "hal.h"
#include "host.h"
#include "host_driver.h"
#include "print.h"
#include "report.h"
#include "timer.h"
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
#    include "version.h"
#endif

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

static void   ap2_ble_switch_ble_driver(void);
static void   ap2_ble_execute_actions(ap2_ble_actions_t actions);
static void   ap2_ble_handle_rx_frame(const uint8_t *frame, uint8_t size);
static void   ap2_ble_handle_command_ack(uint8_t command, uint8_t value);
static void   ap2_ble_reset_rx_parser(void);
static void   ap2_ble_send_broadcast(void);
static void   ap2_ble_send_connect(void);
static void   ap2_ble_send_slot_state(void);
static int8_t ap2_ble_read_saved_slot(void);
static void   ap2_ble_save_slot(int8_t slot);
static void   ap2_ble_write_config(int8_t slot, annepro2_ble_profile_t profile);
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

/* One-shot slot-key state sent before the official 0x40/0x01 or 0x04 command. */
static uint8_t ble_mcu_slot_state[10] = {
    0x7b, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7d, 0x20, 0x0b,
};

/* Reply sent by the official keyboard MCU after a BLE 0x20/0x0c request. */
static uint8_t ble_mcu_hid_handshake_response[12] = {
    0x7b, 0x12, 0x43, 0x00, 0x04, 0x00, 0x00, 0x7d, 0x20, 0x0c, 0x00, 0x00,
};

/* Reply emitted by the official keyboard MCU for an incoming 0x20/0x07. */
static uint8_t ble_mcu_state_sync_response[10] = {
    0x7b, 0x12, 0x43, 0x00, 0x03, 0x00, 0x00, 0x7d, 0x20, 0x07,
};

static uint8_t ble_mcu_bootload[11] = {0x7b, 0x10, 0x51, 0x10, 0x03, 0x00, 0x00, 0x7d, 0x02, 0x01, 0x01};

static host_driver_t         *last_host_driver = NULL;
static ap2_ble_state_t        ble_state;
static annepro2_ble_parser_t  ble_rx_parser;
static annepro2_ble_profile_t ble_profile = ANNEPRO2_BLE_DEFAULT_PROFILE;
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
}

/* -------------------- Public Function Implementation ---------------------- */

void annepro2_ble_bootload(void) {
    sdWrite(&SD1, ble_mcu_bootload, sizeof(ble_mcu_bootload));
}

void annepro2_ble_startup(void) {
    annepro2_ble_profile_t saved_profile;
    int8_t                 saved_slot;
    if (annepro2_ble_decode_config(eeconfig_read_kb(), &saved_profile, &saved_slot)) {
        ble_profile = saved_profile;
    } else {
        ble_profile = ANNEPRO2_BLE_DEFAULT_PROFILE;
    }
    saved_slot = ap2_ble_read_saved_slot();
#if defined(QMK_USERSPACE_VERSION)
    AP2_BLE_LOG("build qmk=%s userspace=%s", QMK_GIT_HASH, QMK_USERSPACE_VERSION);
#else
    AP2_BLE_LOG("build qmk=%s", QMK_GIT_HASH);
#endif
    AP2_BLE_LOG("wake %d profile=%u", saved_slot, (unsigned)ble_profile);
    ap2_ble_execute_actions(ap2_ble_state_startup(&ble_state, saved_slot, timer_read32()));
}

void annepro2_ble_broadcast(uint8_t port) {
    AP2_BLE_LOG("broadcast request slot=%u", port > 3 ? 3 : port);
    ap2_ble_execute_actions(ap2_ble_state_broadcast(&ble_state, port, timer_read32()));
}

void annepro2_ble_connect(uint8_t port) {
    AP2_BLE_LOG("connect request slot=%u", port > 3 ? 3 : port);
    ap2_ble_execute_actions(ap2_ble_state_connect(&ble_state, port, timer_read32()));
}

void annepro2_ble_slot_press(uint8_t port) {
    /*
     * Match the official keyboard MCU: a tap connects on key release, while a
     * 500 ms hold starts pairing/advertising and does nothing on release.
     */
    AP2_BLE_LOG("slot %u press", port > 3 ? 3 : port);
    ap2_ble_execute_actions(ap2_ble_state_slot_press(&ble_state, port, timer_read32()));
}

void annepro2_ble_slot_release(uint8_t port) {
    AP2_BLE_LOG("slot %u release", port > 3 ? 3 : port);
    ap2_ble_execute_actions(ap2_ble_state_slot_release(&ble_state, port, timer_read32()));
}

void annepro2_ble_disconnect(void) {
    AP2_BLE_LOG("disconnect");
    ap2_ble_execute_actions(ap2_ble_state_disconnect(&ble_state));
    ap2_ble_reset_rx_parser();
}

void annepro2_ble_unpair(void) {
    AP2_BLE_LOG("tx unpair");
    ap2_ble_execute_actions(ap2_ble_state_unpair(&ble_state));
    ap2_ble_reset_rx_parser();
}

annepro2_ble_profile_t annepro2_ble_get_profile(void) {
    return ble_profile;
}

void annepro2_ble_set_profile(annepro2_ble_profile_t profile) {
    if (profile != ANNEPRO2_BLE_PROFILE_C18_205 && profile != ANNEPRO2_BLE_PROFILE_AP2D_213) {
        return;
    }
    if (profile == ble_profile) {
        return;
    }

    /*
     * A slot number belongs to the BLE module's bond database. Clear the
     * autoconnect selection when changing module profiles so a stale C18 slot
     * is never replayed to an AP2D module (or vice versa).
     */
    annepro2_ble_disconnect();
    ble_profile = profile;
    ap2_ble_write_config(-1, ble_profile);
    AP2_BLE_LOG("profile=%u", (unsigned)ble_profile);
}

void annepro2_ble_task(void) {
    const uint32_t now = timer_read32();
    if (annepro2_ble_parser_expire(&ble_rx_parser, now)) {
        AP2_BLE_LOG("rx partial timeout");
    }
    ap2_ble_execute_actions(ap2_ble_state_task(&ble_state, now));
}

void annepro2_ble_rx_byte(uint8_t byte) {
    const uint8_t                   *frame;
    uint8_t                          size;
    const annepro2_ble_parse_event_t event = annepro2_ble_parser_feed(&ble_rx_parser, byte, timer_read32(), &frame, &size);

    switch (event) {
        case ANNEPRO2_BLE_PARSE_FRAME:
            ap2_ble_handle_rx_frame(frame, size);
            break;
        case ANNEPRO2_BLE_PARSE_TIMEOUT:
            AP2_BLE_LOG("rx partial timeout");
            break;
        case ANNEPRO2_BLE_PARSE_INVALID_LENGTH:
            AP2_BLE_LOG("rx invalid length");
            break;
        case ANNEPRO2_BLE_PARSE_INVALID_DELIMITER:
            AP2_BLE_LOG("rx invalid delimiter");
            break;
        case ANNEPRO2_BLE_PARSE_NONE:
            break;
    }
}

/* ------------------- Static Function Implementation ----------------------- */
static void ap2_ble_execute_actions(ap2_ble_actions_t actions) {
    if (actions != AP2_BLE_ACTION_NONE) {
        AP2_BLE_LOG("actions=%03X state=%u slot=%u retries=%u", actions, (unsigned)ble_state.state, ble_state.selected_slot, ble_state.command_retries);
    }

    if (actions & AP2_BLE_ACTION_SEND_UNPAIR) {
        sdWrite(&SD1, ble_mcu_unpair, sizeof(ble_mcu_unpair));
    }
    if (actions & AP2_BLE_ACTION_ROUTE_USB) {
        ap2_ble_begin_route_request();
    }
    if (actions & AP2_BLE_ACTION_SEND_WAKEUP) {
        sdWrite(&SD1, ble_mcu_wakeup, sizeof(ble_mcu_wakeup));
    }
    if (actions & AP2_BLE_ACTION_SEND_SLOT_STATE) {
        ap2_ble_send_slot_state();
    }
    if (actions & AP2_BLE_ACTION_SEND_BROADCAST) {
        ap2_ble_send_broadcast();
    }
    if (actions & AP2_BLE_ACTION_SEND_CONNECT) {
        ap2_ble_send_connect();
    }
    if (actions & AP2_BLE_ACTION_CLEAR_SLOT) {
        ap2_ble_save_slot(-1);
    }
    if (actions & AP2_BLE_ACTION_ROUTE_BLE) {
        ap2_ble_switch_ble_driver();
    }
    if (actions & AP2_BLE_ACTION_SAVE_SLOT) {
        ap2_ble_save_slot((int8_t)ble_state.selected_slot);
    }
}

static void ap2_ble_send_broadcast(void) {
    AP2_BLE_LOG("tx broadcast slot=%u attempt=%u", ble_state.selected_slot, ble_state.command_retries + 1);
    sdWrite(&SD1, ble_mcu_start_broadcast, sizeof(ble_mcu_start_broadcast));
    sdPut(&SD1, ble_state.selected_slot);
}

static void ap2_ble_send_connect(void) {
    AP2_BLE_LOG("tx connect slot=%u attempt=%u", ble_state.selected_slot, ble_state.command_retries + 1);
    sdWrite(&SD1, ble_mcu_connect, sizeof(ble_mcu_connect));
    sdPut(&SD1, ble_state.selected_slot);
}

static void ap2_ble_send_slot_state(void) {
    /*
     * The stock firmware emits this state once before a slot action. It is not
     * part of the retried 0x40/0x01 or 0x40/0x04 transaction.
     */
    annepro2_ble_slot_state_t slot_state;
    if (!annepro2_ble_encode_slot_state(ble_profile, ble_state.command_slot_broadcast, &slot_state)) {
        return;
    }

    AP2_BLE_LOG("tx slot state command=%02X action=%u", slot_state.command, slot_state.action);
    ble_mcu_slot_state[9] = slot_state.command;
    sdWrite(&SD1, ble_mcu_slot_state, sizeof(ble_mcu_slot_state));
    sdPut(&SD1, ble_state.selected_slot);
    sdPut(&SD1, slot_state.action);
}

static void ap2_ble_handle_command_ack(uint8_t command, uint8_t value) {
    const ap2_ble_state_id_t before = ble_state.state;
    AP2_BLE_LOG("rx command ack=%02X value=%02X state=%u", command, value, (unsigned)before);
    ap2_ble_execute_actions(ap2_ble_state_command_ack(&ble_state, command, timer_read32()));
    if (before == ble_state.state) {
        AP2_BLE_LOG("ignore stale command ack=%02X", command);
    }
}

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

    if (size >= 11 && frame[1] == 0x12 && frame[2] == 0x35) {
        AP2_BLE_LOG("rx decoded group=%02X command=%02X value=%02X state=%u", frame[8], frame[9], frame[10], (unsigned)ble_state.state);

        if (frame[4] == 0x03 && frame[5] == 0x00 && frame[8] == 0x40 && (frame[9] == 0x01 || frame[9] == 0x04)) {
            ap2_ble_handle_command_ack(frame[9], frame[10]);
        }

        /*
         * The official keyboard MCU preserves the value and reverses the
         * routing field when replying to this state-sync request.
         */
        if (frame[4] == 0x03 && frame[5] == 0x00 && frame[8] == 0x20 && frame[9] == 0x07) {
            AP2_BLE_LOG("tx state sync response value=%02X", frame[10]);
            sdWrite(&SD1, ble_mcu_state_sync_response, sizeof(ble_mcu_state_sync_response));
            sdPut(&SD1, frame[10]);
        }

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
            const ap2_ble_actions_t actions = ap2_ble_state_handshake(&ble_state);
            if (actions & AP2_BLE_ACTION_ROUTE_BLE) {
                AP2_BLE_LOG("rx hid handshake ready");
            }
            ap2_ble_execute_actions(actions);
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

static void ap2_ble_reset_rx_parser(void) {
    annepro2_ble_parser_reset(&ble_rx_parser);
}

static int8_t ap2_ble_read_saved_slot(void) {
    const uint32_t         config = eeconfig_read_kb();
    int8_t                 slot;
    annepro2_ble_profile_t profile;

    if (annepro2_ble_decode_config(config, &profile, &slot)) {
        return slot;
    }

    /* Migrate the legacy QMK encoding: zero disables, 1..4 select slots 0..3. */
    return config >= 1 && config <= 4 ? (int8_t)config - 1 : -1;
}

static void ap2_ble_save_slot(int8_t slot) {
    ap2_ble_write_config(slot, ble_profile);
}

static void ap2_ble_write_config(int8_t slot, annepro2_ble_profile_t profile) {
    uint32_t config;
    if (!annepro2_ble_encode_config(profile, slot, &config)) {
        return;
    }

    if (eeconfig_read_kb() != config) {
        eeconfig_update_kb(config);
    }
}

static uint8_t ap2_ble_leds(void) {
    return 0; // TODO: Figure out how to obtain LED status
}

static void ap2_ble_mouse(report_mouse_t *report) {}

static void ap2_ble_extra(report_extra_t *report) {
    if (report->report_id == REPORT_ID_CONSUMER) {
        uint8_t        payload[ANNEPRO2_BLE_CONSUMER_MAX_SIZE];
        uint8_t        payload_size;
        const uint16_t usage       = report->usage;
        const size_t   usage_count = usage == 0 ? 0 : 1;

        if (!annepro2_ble_encode_consumer(ble_profile, &usage, usage_count, payload, &payload_size)) {
            AP2_BLE_LOG("consumer unsupported usage=%04X profile=%u", usage, (unsigned)ble_profile);
            annepro2_ble_encode_consumer(ble_profile, &usage, 0, payload, &payload_size);
        }

        /* UART payload includes the 0x10/0x08 group and command bytes. */
        ble_mcu_send_consumer_report[4] = payload_size + 2;
        sdPut(&SD1, 0x0);
        sdWrite(&SD1, ble_mcu_send_consumer_report, sizeof(ble_mcu_send_consumer_report));
        sdWrite(&SD1, payload, payload_size);
        AP2_BLE_LOG("tx consumer usage=%04X bytes=%u profile=%u", usage, payload_size, (unsigned)ble_profile);
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
