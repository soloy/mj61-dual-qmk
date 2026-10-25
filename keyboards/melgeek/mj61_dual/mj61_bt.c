// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Bluetooth through the board's nRF52, which keeps the stock MelGeek BT
 * firmware. The NUC123 talks to it over UART1 (PB4 RX / PB5 TX, 115200) in
 * 16-byte frames: [cmd][len][sum][0][data, up to 12 bytes], sum = low byte of
 * the sum of all 16 bytes with byte 2 = 0. See docs/bt-protocol.md; only
 * commands the stock firmware sends itself are used here:
 *   0x00 hello after start-up, 0x0b [mode, channel] connect (0) or pair (1),
 *   0x06 HID report, the bytes stock sends to USB: keyboard 8, mouse 4,
 *        consumer 3 (24-bit usage bitmap, see consumer_bits).
 *   0x0c [percent] battery level (shown by macOS), on connect and on change.
 * From the nRF52: 0x00 ready, 0x04 [1/0, channel] connected/disconnected,
 * 0x0d [leds].
 *
 * Light feedback over the whole keyboard, like stock: blue breathing while
 * the nRF52 advertises for pairing (0x03 [channel, 3]), two white blinks when
 * it connects, a white fade-out on the way back to USB.
 *
 * The channel is kept in eeconfig_kb. The battery is measured like stock:
 * 10 ADC samples of PD0 (AIN0), the middle four of the sorted samples
 * averaged and looked up in stock's voltage tables (0x91b8/0x9218), with PB7
 * low = charging and PD5 low = full (TP4056 CHRG/STDBY).
 *
 * Keys (mj61_bt.h): MJ_BT1..6 connect to channel 1..6 on a tap and pair on a
 * hold (MJ_BT_PAIR_MS), and switch the output to Bluetooth; MJ_USB switches
 * back to USB, like FN2 + Esc on the stock firmware.
 */
#include <string.h>
#include <hal.h>
#include "quantum.h"
#include "bluetooth.h"
#include "connection.h"
#include "host.h"
#include "mj61_bt.h"

#define BT_FRAME 16
#define BT_DATA_MAX 12
#define BT_RX_GAP_MS 5 // a frame arrives in ~1.4 ms; a longer gap means we lost sync

#ifndef MJ61_BT_PAIR_MS
#    define MJ61_BT_PAIR_MS 3000
#endif

enum {
    BT_CMD_HELLO     = 0x00,
    BT_CMD_REPORT    = 0x06,
    BT_CMD_CHANNEL   = 0x0b,
    BT_CMD_BATTERY   = 0x0c,
    BT_RX_READY      = 0x00,
    BT_RX_STATE      = 0x03,
    BT_RX_CONNECTION = 0x04,
    BT_RX_LEDS       = 0x0d,
};

static const SerialConfig bt_serial_config = {
    .speed  = 115200,
    .mode   = NUC123_SERIAL_MODE_DEFAULT,
    .data   = NUC123_SERIAL_DATA_8BITS,
    .parity = NUC123_SERIAL_PARITY_N,
    .stop   = NUC123_SERIAL_STOP_1,
};

static bool     nrf_ready, bt_connected;
static uint8_t  bt_leds;
static uint8_t  bt_channel; // 0-based, as on the wire
static uint8_t  rx[BT_FRAME];
static uint8_t  rx_len;
static uint32_t rx_time;
static uint16_t bt_key_time;
static uint16_t rx_frames, rx_dropped; // for mj61_bt_debug()
enum { FX_NONE, FX_PAIRING, FX_CONNECTED, FX_USB };
static uint8_t  fx;
static uint32_t fx_time;

static void fx_start(uint8_t effect) {
    fx      = effect;
    fx_time = timer_read32();
}

static uint16_t tx_frames, tx_reports, tx_dropped;
static uint8_t  tx_last[BT_FRAME]; // last frame other than a HID report
static uint16_t battery_raw;
static uint8_t  battery_percent = 0xFF, battery_sent = 0xFF;
static uint32_t battery_time;

#define BATTERY_INTERVAL_MS 30000

// Stock tables (percent, ADC reading), not charging / charging; 2.8 V .. 4.2 V.
static const uint16_t battery_table[2][15][2] = {
    {{0, 417}, {0, 434}, {0, 450}, {0, 465}, {0, 480}, {1, 497}, {2, 512}, {5, 528},
     {28, 545}, {48, 560}, {62, 576}, {75, 592}, {86, 607}, {96, 623}, {99, 638}},
    {{0, 422}, {0, 448}, {0, 463}, {0, 479}, {0, 496}, {1, 510}, {2, 527}, {5, 543},
     {28, 558}, {48, 575}, {62, 590}, {75, 605}, {86, 620}, {96, 635}, {99, 640}},
};
static uint8_t  rx_last[BT_FRAME];

static uint8_t frame_sum(const uint8_t *f) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < BT_FRAME; i++) {
        sum += i == 2 ? 0 : f[i];
    }
    return sum;
}

static void bt_send(uint8_t cmd, uint8_t len, const void *data) {
    uint8_t f[BT_FRAME] = {cmd, len};
    if (len > BT_DATA_MAX) {
        return;
    }
    if (len) {
        memcpy(f + 4, data, len);
    }
    f[2] = frame_sum(f);
    // Never block the main loop on a stalled UART: send whole frames or none.
    osalSysLock();
    bool room = oqGetEmptyI(&SD1.oqueue) >= sizeof(f);
    osalSysUnlock();
    if (!room) {
        tx_dropped++;
        return;
    }
    sdWrite(&SD1, f, sizeof(f));
    tx_frames++;
    if (cmd == BT_CMD_REPORT) {
        tx_reports++;
    } else {
        memcpy(tx_last, f, BT_FRAME);
    }
}

static void bt_select_channel(uint8_t mode) {
    const uint8_t d[2] = {mode, bt_channel};
    bt_send(BT_CMD_CHANNEL, sizeof(d), d);
}

static void battery_adc_init(void) {
    SYS->REGWRPROT = 0x59;
    SYS->REGWRPROT = 0x16;
    SYS->REGWRPROT = 0x88;
    CLK->APBCLK |= CLK_APBCLK_ADC_EN_Msk;
    CLK->CLKSEL1 &= ~CLK_CLKSEL1_ADC_S_Msk;                                         // 12 MHz crystal
    CLK->CLKDIV = (CLK->CLKDIV & ~CLK_CLKDIV_ADC_N_Msk) | (11 << CLK_CLKDIV_ADC_N_Pos); // / 12 = 1 MHz
    SYS->REGWRPROT = 0;
    PD->OFFD |= 1UL << 16; // PD0: no digital input
    SYS->GPD_MFP |= 1UL;   // PD0 = AIN0, as stock
    SYS->ALT_MFP1 |= 1UL << 16;
    ADC->ADCHER = 1;
    ADC->ADCR   = ADC_ADCR_ADEN_Msk; // single conversion
}

static uint16_t battery_adc_read(void) {
    ADC->ADSR = ADC_ADSR_ADF_Msk;
    ADC->ADCR |= ADC_ADCR_ADST_Msk;
    for (uint16_t i = 0; i < 2000 && !(ADC->ADSR & ADC_ADSR_ADF_Msk); i++) {
    }
    return ADC->ADDR[0] & ADC_ADDR_RSLT_Msk;
}

static void battery_measure(void) {
    uint16_t v[10];
    for (uint8_t i = 0; i < 10; i++) {
        v[i] = battery_adc_read();
        for (uint8_t j = i; j > 0 && v[j - 1] > v[j]; j--) { // insertion sort
            uint16_t t = v[j];
            v[j]       = v[j - 1];
            v[j - 1]   = t;
        }
    }
    battery_raw = (v[3] + v[4] + v[5] + v[6]) / 4;

    if (!gpio_read_pin(D5)) { // TP4056 STDBY: charged
        battery_percent = 100;
        return;
    }
    const uint16_t(*t)[2] = battery_table[!gpio_read_pin(B7)]; // TP4056 CHRG: charging
    if (battery_raw <= t[0][1]) {
        battery_percent = t[0][0];
    } else if (battery_raw >= t[14][1]) {
        battery_percent = t[14][0];
    } else {
        uint8_t i = 1;
        while (battery_raw > t[i][1]) {
            i++;
        }
        battery_percent = t[i - 1][0] + (uint32_t)(t[i][0] - t[i - 1][0]) * (battery_raw - t[i - 1][1]) / (t[i][1] - t[i - 1][1]);
    }
}

static void battery_send(void) {
    if (bt_connected && battery_percent != 0xFF) {
        bt_send(BT_CMD_BATTERY, 1, &battery_percent);
        battery_sent = battery_percent;
    }
}

static bool host_is_bluetooth(void) {
    return connection_get_host_raw() == CONNECTION_HOST_BLUETOOTH;
}

static void bt_handle(const uint8_t *f) {
    const uint8_t *d = f + 4;
    switch (f[0]) {
        case BT_RX_READY:
            if (f[1] == 0) {
                nrf_ready = true;
                if (host_is_bluetooth()) {
                    bt_select_channel(0); // reconnect, as stock does
                }
            }
            break;
        case BT_RX_STATE:
            if (f[1] >= 2) {
                if (d[1] == 3) {
                    fx_start(FX_PAIRING); // advertising: no bonded host connected
                } else if (fx == FX_PAIRING) {
                    fx = FX_NONE;
                }
            }
            break;
        case BT_RX_CONNECTION:
            if (f[1] >= 1) {
                bt_connected = d[0] == 1;
                if (bt_connected) {
                    fx_start(FX_CONNECTED);
                    battery_send(); // stock sends the level on every connect
                } else if (host_is_bluetooth()) {
                    bt_select_channel(0);
                }
            }
            break;
        case BT_RX_LEDS:
            if (f[1] >= 1) {
                bt_leds = d[0];
            }
            break;
    }
}

void bluetooth_init(void) {
    sdStart(&SD1, &bt_serial_config);
    // The ChibiOS driver also muxes PB6/PB7 as nRTS/nCTS; stock uses only
    // PB4/PB5, and PB7 is a charger status input.
    SYS->GPB_MFP &= ~((1UL << 6) | (1UL << 7));
    gpio_set_pin_output_push_pull(B8); // low while the NUC123 is awake (stock: high before deep sleep)
    gpio_write_pin_low(B8);
    gpio_set_pin_input(B7); // charger status inputs
    gpio_set_pin_input(D5);
    bt_channel = eeconfig_read_kb() & 0x07;
    if (bt_channel > 5) {
        bt_channel = 0;
    }
    battery_adc_init();
    battery_measure();
    battery_time = timer_read32();
    wait_ms(1);
    bt_send(BT_CMD_HELLO, 0, NULL);
}

void bluetooth_task(void) {
    if (timer_elapsed32(battery_time) > BATTERY_INTERVAL_MS) {
        battery_time = timer_read32();
        battery_measure();
        if (battery_percent != battery_sent) {
            battery_send();
        }
    }
    msg_t c;
    while ((c = sdGetTimeout(&SD1, TIME_IMMEDIATE)) >= 0) {
        if (rx_len && timer_elapsed32(rx_time) > BT_RX_GAP_MS) {
            rx_len = 0;
        }
        rx_time      = timer_read32();
        rx[rx_len++] = (uint8_t)c;
        if (rx_len < BT_FRAME) {
            continue;
        }
        if (rx[1] <= BT_DATA_MAX && rx[3] == 0 && frame_sum(rx) == rx[2]) {
            memcpy(rx_last, rx, BT_FRAME);
            rx_frames++;
            bt_handle(rx);
            rx_len = 0;
        } else {
            memmove(rx, rx + 1, BT_FRAME - 1); // resync: drop one byte
            rx_dropped++;
            rx_len = BT_FRAME - 1;
        }
    }
}

bool bluetooth_is_connected(void) {
    return bt_connected;
}

bool bluetooth_can_send_nkro(void) {
    return false;
}

uint8_t bluetooth_keyboard_leds(void) {
    return bt_leds;
}

void bluetooth_send_keyboard(report_keyboard_t *report) {
    if (!bt_connected) {
        return;
    }
    uint8_t r[8] = {report->mods, 0};
    memcpy(r + 2, report->keys, 6);
    bt_send(BT_CMD_REPORT, sizeof(r), r);
}

void bluetooth_send_nkro(report_nkro_t *report) {} // never: bluetooth_can_send_nkro() is false

static int8_t clamp8(int16_t v) {
    return v > INT8_MAX ? INT8_MAX : v < INT8_MIN ? INT8_MIN : v;
}

// Stock sends the 4-byte boot mouse report of its USB mouse interface:
// buttons (5 bits), X, Y, wheel.
void bluetooth_send_mouse(report_mouse_t *report) {
    if (!bt_connected) {
        return;
    }
    const uint8_t r[4] = {report->buttons & 0x1F, clamp8(report->x), clamp8(report->y), clamp8(report->v)};
    bt_send(BT_CMD_REPORT, sizeof(r), r);
}

// Stock's consumer report (USB report ID 2, sent over BT without the ID) is a
// 24-bit bitmap, one bit per usage in this order (descriptor at 0x8fb9).
static const uint16_t consumer_bits[24] = {
    0x0B5, 0x0B6, 0x0B7, 0x0CD, 0x0E2, 0x0E5, 0x0E7, 0x0E9, 0x0EA, 0x152, 0x153, 0x154,
    0x155, 0x183, 0x18A, 0x192, 0x194, 0x221, 0x223, 0x224, 0x225, 0x226, 0x227, 0x22A,
};

void bluetooth_send_consumer(uint16_t usage) {
    if (!bt_connected) {
        return;
    }
    uint8_t r[3] = {0};
    for (uint8_t i = 0; usage && i < 24; i++) {
        if (consumer_bits[i] == usage) {
            r[i / 8] = 1 << (i % 8);
            break;
        }
    }
    bt_send(BT_CMD_REPORT, sizeof(r), r); // usage 0 or one not in the bitmap: all released
}

void bluetooth_send_system(uint16_t usage) {} // stock sends no system control over BT

// QMK sends raw HID to the active host driver, so with the output on
// Bluetooth VIA's answers would be dropped. VIA only exists on USB: answer there.
void bluetooth_send_raw_hid(uint8_t *data, uint8_t length) {
    host_driver_t *usb = host_get_driver();
    if (usb && usb->send_raw_hid) {
        usb->send_raw_hid(data, length);
    }
}

bool mj61_bt_process_record(uint16_t keycode, keyrecord_t *record) {
    if (keycode >= MJ_BT1 && keycode <= MJ_BT6) {
        if (record->event.pressed) {
            bt_key_time = timer_read();
        } else {
            bt_channel = keycode - MJ_BT1;
            eeconfig_update_kb((eeconfig_read_kb() & ~0x07UL) | bt_channel);
            connection_set_host(CONNECTION_HOST_BLUETOOTH);
            bt_select_channel(timer_elapsed(bt_key_time) >= MJ61_BT_PAIR_MS ? 1 : 0);
        }
        return false;
    }
    if (keycode == MJ_USB) {
        if (record->event.pressed && host_is_bluetooth()) {
            const uint8_t none[8] = {0};
            if (bt_connected) {
                bt_send(BT_CMD_REPORT, sizeof(none), none); // release everything on the BT host
            }
            connection_set_host(CONNECTION_HOST_USB);
            fx_start(FX_USB);
        }
        return false;
    }
    return true;
}

uint8_t mj61_bt_debug(uint8_t *out, uint8_t size) {
    uint8_t d[29] = {nrf_ready, bt_connected, bt_leds, bt_channel, connection_get_host_raw(), connection_get_host(),
                     rx_frames >> 8, rx_frames & 0xFF, rx_dropped >> 8, rx_dropped & 0xFF};
    memcpy(d + 10, rx_last, BT_FRAME);
    d[26] = battery_raw >> 8;
    d[27] = battery_raw & 0xFF;
    d[28] = battery_percent;
    if (size > sizeof(d)) {
        size = sizeof(d);
    }
    memcpy(out, d, size);
    return size;
}

uint8_t mj61_bt_debug_tx(uint8_t *out, uint8_t size) {
    uint8_t d[22] = {tx_frames >> 8, tx_frames & 0xFF, tx_reports >> 8, tx_reports & 0xFF};
    memcpy(d + 4, tx_last, BT_FRAME);
    d[20] = tx_dropped >> 8;
    d[21] = tx_dropped & 0xFF;
    if (size > sizeof(d)) {
        size = sizeof(d);
    }
    memcpy(out, d, size);
    return size;
}

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_kb(void) {
    if (!rgb_matrix_indicators_user()) {
        return false;
    }
    uint32_t t   = timer_elapsed32(fx_time);
    uint8_t  max = rgb_matrix_get_val() < 128 ? 128 : rgb_matrix_get_val(); // visible even when dimmed
    switch (fx) {
        case FX_PAIRING: { // blue breathing, 1.6 s
            uint16_t p = t % 1600;
            uint8_t  v = (p < 800 ? p : 1600 - p) * max / 800;
            rgb_matrix_set_color_all(0, 0, v);
            break;
        }
        case FX_CONNECTED: // two white blinks
            if (t >= 600) {
                fx = FX_NONE;
            } else {
                uint8_t v = (t / 150) % 2 ? 0 : max;
                rgb_matrix_set_color_all(v, v, v);
            }
            break;
        case FX_USB: // white, fading out
            if (t >= 800) {
                fx = FX_NONE;
            } else {
                uint8_t v = (800 - t) * max / 800;
                rgb_matrix_set_color_all(v, v, v);
            }
            break;
    }
    return true;
}
#endif
