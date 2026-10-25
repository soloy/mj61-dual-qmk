// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdbool.h>

// Writes at most one dirty page to the 24Cxx, never waits. Returns true while
// pages are still waiting to be written.
bool eeprom_i2c1_task(void);
// Writes every dirty page, waiting out each write cycle (before a reset).
void eeprom_i2c1_flush(void);
// Driver state and a live probe of the chip, for debugging over VIA (see mj61_dual.c).
uint8_t eeprom_i2c1_debug(uint8_t *out, uint8_t size);
