// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

/*
 * Mirrors the MelGeek "Mj61 DUAL HOTSWAP PCB" manual:
 *   FN1: number row -> F1-F12/Del, WASD arrows, nav cluster on P [ ] L ; ' , . /
 *   FN2: media on 7-=, RGB on Z X C. The Bluetooth keys (Esc, 1-6) do nothing,
 *        since QMK cannot talk to the nRF52.
 *   FN1/FN2 + Y: calculator (in the stock keymap, not in the manual).
 * Added: FN2 + B = QK_BOOT.
 */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_60_ansi(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,
        KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,
        KC_LCTL, KC_LGUI,          KC_LALT,          KC_SPC,                             KC_RALT, MO(1),   MO(2),   KC_RCTL
    ),
    [1] = LAYOUT_60_ansi(
        KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_DEL,
        _______, _______, KC_UP,   _______, _______, _______, KC_CALC, _______, _______, _______, KC_PSCR, KC_SCRL, KC_PAUS, _______,
        _______, KC_LEFT, KC_DOWN, KC_RGHT, _______, _______, _______, _______, _______, KC_INS,  KC_HOME, KC_PGUP,          _______,
        _______,          _______, _______, _______, _______, _______, _______, _______, KC_DEL,  KC_END,  KC_PGDN, _______,
        _______, _______,          _______,          _______,                            _______, _______, _______, _______
    ),
    [2] = LAYOUT_60_ansi(
        _______, _______, _______, _______, _______, _______, _______, KC_VOLU, KC_VOLD, KC_MUTE, KC_MPRV, KC_MNXT, KC_MPLY, _______,
        _______, _______, _______, _______, _______, _______, KC_CALC, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______,          RM_TOGG, RM_NEXT, RM_HUEU, _______, QK_BOOT, _______, _______, _______, _______, _______, _______,
        _______, _______,          _______,          _______,                            _______, _______, _______, _______
    )
};
