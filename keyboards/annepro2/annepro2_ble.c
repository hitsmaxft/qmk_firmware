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
#include "report.h"

/* -------------------- Static Function Prototypes -------------------------- */
static uint8_t ap2_ble_leds(void);
static void    ap2_ble_mouse(report_mouse_t *report);
static void    ap2_ble_extra(report_extra_t *report);
static void    ap2_ble_keyboard(report_keyboard_t *report);

static void ap2_ble_switch_ble_driver(void);

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

static uint8_t ble_mcu_bootload[11] = {0x7b, 0x10, 0x51, 0x10, 0x03, 0x00, 0x00, 0x7d, 0x02, 0x01, 0x01};

static host_driver_t *last_host_driver = NULL;
#ifdef NKRO_ENABLE
static bool lastNkroStatus = false;
#endif // NKRO_ENABLE

/* -------------------- Public Function Implementation ---------------------- */

void annepro2_ble_bootload(void) {
    sdWrite(&SD1, ble_mcu_bootload, sizeof(ble_mcu_bootload));
}

void annepro2_ble_startup(void) {
    sdWrite(&SD1, ble_mcu_wakeup, sizeof(ble_mcu_wakeup));
}

void annepro2_ble_broadcast(uint8_t port) {
    if (port > 3) {
        port = 3;
    }
    sdWrite(&SD1, ble_mcu_start_broadcast, sizeof(ble_mcu_start_broadcast));
    sdPut(&SD1, port);
    sdPut(&SD1, 0x00);
    static int last_broadcast = -1;
    if (last_broadcast == port) {
        annepro2_ble_connect(port);
    }
    last_broadcast = port;
}

void annepro2_ble_connect(uint8_t port) {
    if (port > 3) {
        port = 3;
    }
    sdWrite(&SD1, ble_mcu_connect, sizeof(ble_mcu_connect));
    sdPut(&SD1, port);
    sdPut(&SD1, 0x00);
    ap2_ble_switch_ble_driver();
}

void annepro2_ble_slot_press(uint8_t port) {
    /* Preserve the original C15 behavior: repeated presses connect. */
    annepro2_ble_broadcast(port);
}

void annepro2_ble_slot_release(uint8_t port) {
    (void)port;
}

void annepro2_ble_disconnect(void) {
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
    sdWrite(&SD1, ble_mcu_unpair, sizeof(ble_mcu_unpair));
}

void annepro2_ble_task(void) {}

annepro2_ble_profile_t annepro2_ble_get_profile(void) {
    return ANNEPRO2_BLE_PROFILE_C18_205;
}

void annepro2_ble_set_profile(annepro2_ble_profile_t profile) {
    (void)profile;
}

void annepro2_ble_rx_byte(uint8_t byte) {
    /* C15 has no decoded asynchronous BLE event consumer. */
    (void)byte;
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
}

static uint8_t ap2_ble_leds(void) {
    return 0;
}

static void ap2_ble_mouse(report_mouse_t *report) {}

static inline uint16_t consumer_to_ap2(uint16_t usage) {
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
        sdPut(&SD1, consumer_to_ap2(report->usage));
        static const uint8_t dummy[3] = {0};
        sdWrite(&SD1, dummy, sizeof(dummy));
    }
}

static void ap2_ble_keyboard(report_keyboard_t *report) {
    sdPut(&SD1, 0x0);
    sdWrite(&SD1, ble_mcu_send_report, sizeof(ble_mcu_send_report));
    sdWrite(&SD1, (uint8_t *)report, KEYBOARD_REPORT_SIZE);
}
