// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "quantum.h"

// Bluetooth keys (mj61_bt.c): tap = connect to the channel, hold = pair.
enum mj61_bt_keycodes {
    MJ_BT1 = QK_KB_0,
    MJ_BT2,
    MJ_BT3,
    MJ_BT4,
    MJ_BT5,
    MJ_BT6,
    MJ_USB, // back to USB, like FN2 + Esc on stock
};

bool mj61_bt_process_record(uint16_t keycode, keyrecord_t *record);
// VIA debug commands (mj61_dual.c, custom get_value on channel 0):
uint8_t mj61_bt_debug(uint8_t *out, uint8_t size);    // value 0xEF: driver state, last frame received
uint8_t mj61_bt_debug_tx(uint8_t *out, uint8_t size); // value 0xF0: frames sent, last non-report frame
