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
#include "annepro2_ble_profile.h"
#include "ch.h"
#include "eeconfig.h"
#include "hal.h"
#include "host.h"
#include "host_driver.h"
#include "print.h"
#include "report.h"
#include "timer.h"

#define AP2_BLE_RX_HEADER_SIZE 8
#define AP2_BLE_RX_MAX_PAYLOAD 32
#define AP2_BLE_RX_MAX_FRAME_SIZE (AP2_BLE_RX_HEADER_SIZE + AP2_BLE_RX_MAX_PAYLOAD)

#ifndef ANNEPRO2_BLE_COMMAND_TIMEOUT
#    define ANNEPRO2_BLE_COMMAND_TIMEOUT 500
#endif

#ifndef ANNEPRO2_BLE_COMMAND_RETRIES
#    define ANNEPRO2_BLE_COMMAND_RETRIES 2
#endif

#ifndef ANNEPRO2_BLE_STARTUP_DELAY
#    define ANNEPRO2_BLE_STARTUP_DELAY 500
#endif

#ifndef ANNEPRO2_BLE_SLOT_HOLD_TIME
#    define ANNEPRO2_BLE_SLOT_HOLD_TIME 500
#endif

#ifndef ANNEPRO2_BLE_HANDSHAKE_TIMEOUT
#    define ANNEPRO2_BLE_HANDSHAKE_TIMEOUT 10000
#endif

#ifndef ANNEPRO2_BLE_SLOT_SWITCH_DELAY
#    define ANNEPRO2_BLE_SLOT_SWITCH_DELAY 1000
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
static void   ap2_ble_handle_rx_frame(const uint8_t *frame, uint8_t size);
static void   ap2_ble_handle_command_ack(uint8_t command, uint8_t value);
static void   ap2_ble_reset_rx_parser(uint8_t byte);
static void   ap2_ble_start_broadcast(uint8_t port, int8_t slot_state, bool handshake_timeout);
static void   ap2_ble_send_broadcast(void);
static void   ap2_ble_send_connect(void);
static void   ap2_ble_send_slot_state(void);
static void   ap2_ble_start_connect_slot(uint8_t port);
static void   ap2_ble_start_connect(void);
static void   ap2_ble_start_handshake_wait(void);
static bool   ap2_ble_operation_pending(void);
static void   ap2_ble_queue_intent(uint8_t port, bool broadcast);
static void   ap2_ble_dispatch_intent(void);
static int8_t ap2_ble_read_saved_slot(void);
static void   ap2_ble_save_slot(int8_t slot);
static void   ap2_ble_write_config(int8_t slot, annepro2_ble_profile_t profile);
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
static void ap2_ble_log_rx_frame(const uint8_t *frame, uint8_t size);
#endif

typedef enum {
    AP2_BLE_STATE_USB,
    AP2_BLE_STATE_WAIT_BROADCAST_ACK,
    AP2_BLE_STATE_WAIT_CONNECT_ACK,
    AP2_BLE_STATE_WAIT_HANDSHAKE,
    AP2_BLE_STATE_ACTIVE,
    AP2_BLE_STATE_STARTUP_PASSIVE,
} ap2_ble_state_t;

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

/* Slot-key state sent by the official keyboard MCU after 0x40/0x01 or 0x04. */
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
static ap2_ble_state_t        ble_state        = AP2_BLE_STATE_USB;
static int8_t                 startup_slot     = -1;
static int8_t                 held_slot        = -1;
static bool                   command_slot_state_pending;
static bool                   command_slot_broadcast;
static int8_t                 pending_slot = -1;
static uint8_t                selected_slot;
static bool                   held_slot_broadcast;
static bool                   pending_broadcast;
static bool                   handshake_timeout_enabled;
static uint8_t                command_retries;
static uint8_t                handshake_recoveries;
static uint32_t               command_timer;
static uint32_t               startup_timer;
static uint32_t               slot_hold_timer;
static uint32_t               handshake_timer;
static uint32_t               pending_timer;
static uint8_t                ble_rx_frame[AP2_BLE_RX_MAX_FRAME_SIZE];
static uint8_t                ble_rx_frame_size;
static uint8_t                ble_rx_expected_size;
static annepro2_ble_profile_t ble_profile = ANNEPRO2_BLE_DEFAULT_PROFILE;
#if defined(CONSOLE_ENABLE) && defined(ANNEPRO2_BLE_DEBUG)
static uint8_t ble_debug_keyboard_reports;
#endif
#ifdef NKRO_ENABLE
static bool lastNkroStatus = false;
#endif // NKRO_ENABLE

static void ap2_ble_set_state(ap2_ble_state_t state) {
    if (ble_state != state) {
        AP2_BLE_LOG("state %u -> %u", (unsigned)ble_state, (unsigned)state);
        ble_state = state;
    }
}

static bool ap2_ble_route_requested(void) {
    return ble_state != AP2_BLE_STATE_USB;
}

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
    startup_slot         = ap2_ble_read_saved_slot();
    startup_timer        = timer_read32();
    pending_slot         = -1;
    handshake_recoveries = 0;
    AP2_BLE_LOG("wake %d profile=%u", startup_slot, (unsigned)ble_profile);
    sdWrite(&SD1, ble_mcu_wakeup, sizeof(ble_mcu_wakeup));
    if (startup_slot >= 0) {
        /*
         * Keep the already required wakeup settling delay useful: if the BLE
         * module restores its bonded link on its own, accept its handshake.
         * Otherwise the task sends the proven one-shot broadcast at 500 ms.
         */
        selected_slot = (uint8_t)startup_slot;
        ap2_ble_set_state(AP2_BLE_STATE_STARTUP_PASSIVE);
    }
}

void annepro2_ble_broadcast(uint8_t port) {
    if (port > 3) {
        port = 3;
    }

    startup_slot = -1;
    if (ble_state == AP2_BLE_STATE_STARTUP_PASSIVE) {
        ap2_ble_set_state(AP2_BLE_STATE_USB);
    }
    if (ap2_ble_operation_pending()) {
        ap2_ble_queue_intent(port, true);
        return;
    }

    pending_slot         = -1;
    handshake_recoveries = 0;
    ap2_ble_start_broadcast(port, 1, false);
}

void annepro2_ble_connect(uint8_t port) {
    if (port > 3) {
        port = 3;
    }

    startup_slot = -1;
    if (ble_state == AP2_BLE_STATE_STARTUP_PASSIVE) {
        ap2_ble_set_state(AP2_BLE_STATE_USB);
    }
    ap2_ble_begin_route_request();
    if (ap2_ble_operation_pending()) {
        if (selected_slot == port) {
            pending_slot = -1;
            AP2_BLE_LOG("connect slot=%u already pending", port);
            return;
        }

        ap2_ble_queue_intent(port, false);
        return;
    }

    pending_slot         = -1;
    handshake_recoveries = 0;
    ap2_ble_start_connect_slot(port);
}

void annepro2_ble_slot_press(uint8_t port) {
    /*
     * Match the official keyboard MCU: a tap connects on key release, while a
     * 500 ms hold starts pairing/advertising and does nothing on release.
     */
    if (port > 3) {
        port = 3;
    }
    if (ble_state == AP2_BLE_STATE_STARTUP_PASSIVE) {
        ap2_ble_set_state(AP2_BLE_STATE_USB);
    }
    startup_slot        = -1;
    held_slot           = port;
    held_slot_broadcast = false;
    slot_hold_timer     = timer_read32();
    AP2_BLE_LOG("slot %u press", port);
}

void annepro2_ble_slot_release(uint8_t port) {
    if (held_slot != (int8_t)port) {
        return;
    }

    held_slot = -1;
    if (!held_slot_broadcast) {
        AP2_BLE_LOG("slot %u tap", port);
        annepro2_ble_connect(port);
    }
    held_slot_broadcast = false;
}

void annepro2_ble_disconnect(void) {
    ap2_ble_set_state(AP2_BLE_STATE_USB);
    command_retries            = 0;
    handshake_recoveries       = 0;
    handshake_timeout_enabled  = false;
    command_slot_state_pending = false;
    startup_slot               = -1;
    held_slot                  = -1;
    held_slot_broadcast        = false;
    pending_slot               = -1;
    ap2_ble_save_slot(-1);
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
    annepro2_ble_disconnect();
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
    if (held_slot >= 0 && !held_slot_broadcast && timer_elapsed32(slot_hold_timer) >= ANNEPRO2_BLE_SLOT_HOLD_TIME) {
        held_slot_broadcast = true;
        AP2_BLE_LOG("slot %u hold", (uint8_t)held_slot);
        annepro2_ble_broadcast((uint8_t)held_slot);
    }

    if (startup_slot >= 0 && timer_elapsed32(startup_timer) >= ANNEPRO2_BLE_STARTUP_DELAY) {
        const uint8_t slot = (uint8_t)startup_slot;
        startup_slot       = -1;
        AP2_BLE_LOG("auto %u recovery=%u after passive window", slot, handshake_recoveries);
        ap2_ble_start_broadcast(slot, -1, true);
    }

    if (pending_slot >= 0 && timer_elapsed32(pending_timer) >= ANNEPRO2_BLE_SLOT_SWITCH_DELAY) {
        ap2_ble_dispatch_intent();
    }

    if (ble_state == AP2_BLE_STATE_WAIT_HANDSHAKE) {
        if (!handshake_timeout_enabled || timer_elapsed32(handshake_timer) < ANNEPRO2_BLE_HANDSHAKE_TIMEOUT) {
            return;
        }

        AP2_BLE_LOG("handshake timeout slot=%u recovery=%u", selected_slot, handshake_recoveries);
        ap2_ble_set_state(AP2_BLE_STATE_USB);
        command_slot_state_pending = false;
        handshake_timeout_enabled  = false;
        if (handshake_recoveries == 0) {
            handshake_recoveries = 1;
            startup_slot         = selected_slot;
            startup_timer        = timer_read32();
            sdWrite(&SD1, ble_mcu_wakeup, sizeof(ble_mcu_wakeup));
        }
        return;
    }

    if (ble_state != AP2_BLE_STATE_WAIT_BROADCAST_ACK && ble_state != AP2_BLE_STATE_WAIT_CONNECT_ACK) {
        return;
    }
    if (timer_elapsed32(command_timer) < ANNEPRO2_BLE_COMMAND_TIMEOUT) {
        return;
    }

    if (command_retries >= ANNEPRO2_BLE_COMMAND_RETRIES) {
        AP2_BLE_LOG("command timeout state=%u slot=%u retries=%u", (unsigned)ble_state, selected_slot, command_retries);
        /* A late handshake is still valid, so keep the BLE route request alive. */
        ap2_ble_start_handshake_wait();
        return;
    }

    command_retries++;
    if (ble_state == AP2_BLE_STATE_WAIT_BROADCAST_ACK) {
        ap2_ble_send_broadcast();
    } else {
        ap2_ble_send_connect();
    }
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
static void ap2_ble_start_broadcast(uint8_t port, int8_t slot_state, bool handshake_timeout) {
    if (port > 3) {
        port = 3;
    }

    startup_slot = -1;
    ap2_ble_begin_route_request();
    selected_slot              = port;
    command_slot_state_pending = slot_state >= 0;
    command_slot_broadcast     = slot_state > 0;
    handshake_timeout_enabled  = handshake_timeout;
    command_retries            = 0;
    ap2_ble_set_state(AP2_BLE_STATE_WAIT_BROADCAST_ACK);
    ap2_ble_send_broadcast();
}

static void ap2_ble_send_broadcast(void) {
    AP2_BLE_LOG("tx broadcast slot=%u attempt=%u", selected_slot, command_retries + 1);
    sdWrite(&SD1, ble_mcu_start_broadcast, sizeof(ble_mcu_start_broadcast));
    sdPut(&SD1, selected_slot);
    ap2_ble_send_slot_state();
    command_timer = timer_read32();
}

static void ap2_ble_send_connect(void) {
    AP2_BLE_LOG("tx connect slot=%u attempt=%u", selected_slot, command_retries + 1);
    sdWrite(&SD1, ble_mcu_connect, sizeof(ble_mcu_connect));
    sdPut(&SD1, selected_slot);
    ap2_ble_send_slot_state();
    command_timer = timer_read32();
}

static void ap2_ble_send_slot_state(void) {
    if (!command_slot_state_pending) {
        return;
    }

    /*
     * The stock firmware emits this state once at the edge of a slot action.
     * It is not part of the retried 0x40/0x01 or 0x40/0x04 transaction.
     */
    annepro2_ble_slot_state_t slot_state;
    command_slot_state_pending = false;
    if (!annepro2_ble_encode_slot_state(ble_profile, command_slot_broadcast, &slot_state)) {
        return;
    }

    AP2_BLE_LOG("tx slot state command=%02X action=%u", slot_state.command, slot_state.action);
    ble_mcu_slot_state[9] = slot_state.command;
    sdWrite(&SD1, ble_mcu_slot_state, sizeof(ble_mcu_slot_state));
    sdPut(&SD1, selected_slot);
    sdPut(&SD1, slot_state.action);
}

static void ap2_ble_start_connect_slot(uint8_t port) {
    startup_slot = -1;
    ap2_ble_begin_route_request();
    selected_slot              = port;
    command_slot_state_pending = true;
    command_slot_broadcast     = false;
    handshake_timeout_enabled  = true;
    ap2_ble_start_connect();
}

static void ap2_ble_start_connect(void) {
    command_retries = 0;
    ap2_ble_set_state(AP2_BLE_STATE_WAIT_CONNECT_ACK);
    ap2_ble_send_connect();
}

static void ap2_ble_start_handshake_wait(void) {
    handshake_timer = timer_read32();
    ap2_ble_set_state(AP2_BLE_STATE_WAIT_HANDSHAKE);
}

static bool ap2_ble_operation_pending(void) {
    return ble_state == AP2_BLE_STATE_WAIT_BROADCAST_ACK || ble_state == AP2_BLE_STATE_WAIT_CONNECT_ACK || ble_state == AP2_BLE_STATE_WAIT_HANDSHAKE;
}

static void ap2_ble_queue_intent(uint8_t port, bool broadcast) {
    pending_slot      = port;
    pending_broadcast = broadcast;
    pending_timer     = timer_read32();
    AP2_BLE_LOG("queue %s slot=%u current=%u state=%u", broadcast ? "broadcast" : "connect", port, selected_slot, (unsigned)ble_state);
}

static void ap2_ble_dispatch_intent(void) {
    const uint8_t slot      = (uint8_t)pending_slot;
    const bool    broadcast = pending_broadcast;

    pending_slot               = -1;
    command_slot_state_pending = false;
    handshake_timeout_enabled  = false;
    handshake_recoveries       = 0;
    ap2_ble_set_state(AP2_BLE_STATE_USB);
    AP2_BLE_LOG("dispatch %s slot=%u", broadcast ? "broadcast" : "connect", slot);

    if (broadcast) {
        ap2_ble_start_broadcast(slot, 1, false);
    } else {
        ap2_ble_start_connect_slot(slot);
    }
}

static void ap2_ble_handle_command_ack(uint8_t command, uint8_t value) {
    AP2_BLE_LOG("rx command ack=%02X value=%02X state=%u", command, value, (unsigned)ble_state);

    if (command == 0x01) {
        if (ble_state != AP2_BLE_STATE_WAIT_BROADCAST_ACK) {
            AP2_BLE_LOG("ignore stale broadcast ack");
            return;
        }
        command_retries = 0;
        ap2_ble_start_handshake_wait();
        return;
    }

    if (command == 0x04) {
        if (ble_state != AP2_BLE_STATE_WAIT_CONNECT_ACK) {
            AP2_BLE_LOG("ignore stale connect ack");
            return;
        }
        command_retries = 0;
        ap2_ble_start_handshake_wait();
    }
}

static void ap2_ble_switch_ble_driver(void) {
    command_slot_state_pending = false;
    startup_slot               = -1;
    command_retries            = 0;
    handshake_recoveries       = 0;
    handshake_timeout_enabled  = false;
    ap2_ble_save_slot(selected_slot);
    ap2_ble_set_state(AP2_BLE_STATE_ACTIVE);
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
        AP2_BLE_LOG("rx decoded group=%02X command=%02X value=%02X state=%u", frame[8], frame[9], frame[10], (unsigned)ble_state);

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
            const bool route_requested = ap2_ble_route_requested();
            AP2_BLE_LOG("tx hid handshake response");
            sdWrite(&SD1, ble_mcu_hid_handshake_response, sizeof(ble_mcu_hid_handshake_response));
            if (route_requested) {
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
