// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define GPIO_INPUT_PIN_DELAY (NUC123_HCLK / 6 / 1000000L)

// EEPROM: the stock firmware's 24Cxx on I2C1, see eeprom_i2c1.c. Not the
// flash: where wear_leveling would put it depends on CONFIG0/CONFIG1, and one
// combination hangs its init before the failsafe (docs/hardware.md).
#define EEPROM_SIZE 2048
// NUC123(AN) only has 2 non-control endpoints in the ChibiOS driver. Each
// logical endpoint has both an IN and an OUT hardware buffer, so raw HID (VIA)
// can use one endpoint number for both directions.
#define USB_ENDPOINTS_ARE_REORDERABLE

/*
 * RGB, from the stock firmware (see docs/hardware.md):
 *   IS31FL3741 on I2C0 (PF2 SDA / PF3 SCL, muxed by the ChibiOS NUC123 I2C
 *   driver), 7-bit address 0x30 (ADDR -> GND), SDB on PA12.
 *   Stock runs the bus at 1 MHz; 400 kHz is used here.
 */
#define IS31FL3741_I2C_ADDRESS_1 IS31FL3741_I2C_ADDRESS_GND
#define IS31FL3741_SDB_PIN A12

#define I2C_DRIVER I2CD0
// The NUC123 I2CConfig only has a clock field, like the Kinetis ports.
#define USE_I2CV1
#define USE_I2CV1_CONTRIB
#define I2C1_CLOCK_SPEED 400000
// Only used so QMK's generic (weak) i2c_init() compiles; mj61_dual.c replaces it.
#define USE_GPIOV1
#define I2C1_SCL_PIN F3
#define I2C1_SDA_PIN F2
#define I2C1_SCL_PAL_MODE PAL_MODE_OUTPUT_OPENDRAIN
#define I2C1_SDA_PAL_MODE PAL_MODE_OUTPUT_OPENDRAIN
