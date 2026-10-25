// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * EEPROM on the 24Cxx serial EEPROM that the stock firmware uses for its
 * settings: I2C1 on PA10 SDA / PA11 SCL (muxed by the ChibiOS NUC123 I2C
 * driver), 7-bit address 0x50, 16-bit memory addresses. The stock firmware
 * writes up to 0x2DFF, so the chip holds at least 16 KB; QMK lives above
 * that, so going back to the stock firmware keeps its settings.
 *
 * QMK's own eeprom_i2c driver goes through i2c_master, which only knows one
 * bus (I2C0, the IS31FL3741), hence this small driver on I2CD1.
 *
 * The whole QMK area is mirrored in RAM: it is read once at start-up (in 64-byte
 * chunks: one 2 KB read outlasts the 20 ms timeout at 400 kHz), reads
 * come from RAM (the VIA dynamic keymap reads on every key press) and writes
 * only mark pages dirty. eeprom_i2c1_task() (from housekeeping) writes one
 * dirty page per 24Cxx write cycle without waiting, so the first boot on a
 * blank chip, which rewrites the whole area word by word, is not held up by
 * ~1000 write cycles. Pages go highest first: QMK keeps its magic numbers
 * (eeconfig, VIA) at the bottom, so a power cut halfway through leaves them
 * unwritten and the next boot redoes the defaults. shutdown_kb() flushes
 * everything before a reset or bootloader jump. If the chip does not answer at
 * start-up, the RAM copy works on its own like a transient EEPROM: the
 * keyboard types and VIA works until power-off. This runs before the
 * hold-any-key failsafe, so it must never block.
 */
#include <string.h>
#include <hal.h>
#include "eeprom_driver.h"
#include "wait.h"
#include "timer.h"
#include "eeprom_i2c1.h"
#include "quantum.h"

#define EE_I2C_ADDR 0x50
#define EE_BASE 0x3800 // above the stock firmware's data (0x0000-0x2DFF)
#define EE_PAGE_SIZE 32 // 24C32..24C512 pages are 32-128 bytes; stock writes 64
#define EE_WRITE_MS 6 // 24Cxx write cycle is 5 ms max; +1 for sleep rounding
#define EE_TIMEOUT TIME_MS2I(20)
#define EE_READ_CHUNK 64 // ~1.5 ms at 400 kHz, well inside EE_TIMEOUT
#define EE_PAGES (EEPROM_SIZE / EE_PAGE_SIZE)

_Static_assert(EE_BASE + EEPROM_SIZE <= 0x4000, "QMK area must fit a 16 KB 24C128");
_Static_assert(EE_BASE % EE_PAGE_SIZE == 0 && EEPROM_SIZE % EE_PAGE_SIZE == 0, "pages must line up");
_Static_assert(EEPROM_SIZE % EE_READ_CHUNK == 0, "start-up reads must line up");

static const I2CConfig ee_i2c_config = {.clock = 400000};
static uint8_t         ee_cache[EEPROM_SIZE];
static bool            ee_present;
static uint8_t         ee_dirty[(EE_PAGES + 7) / 8]; // pages changed in RAM, not yet on the chip
static uint16_t        ee_dirty_count;
static bool            ee_writing; // a write cycle may still be running
static uint32_t        ee_write_time;
// For eeprom_i2c1_debug(): what happened at start-up and since.
static msg_t      ee_init_status;
static i2cflags_t ee_init_errors, ee_last_errors;
static msg_t      ee_last_status;
static uint16_t   ee_writes_ok, ee_writes_failed;

static bool ee_is_dirty(uint16_t page) {
    return ee_dirty[page / 8] & (1 << (page % 8));
}

static void ee_set_dirty(uint16_t page, bool dirty) {
    if (ee_is_dirty(page) == dirty) {
        return;
    }
    ee_dirty[page / 8] ^= 1 << (page % 8);
    if (dirty) {
        ee_dirty_count++;
    } else {
        ee_dirty_count--;
    }
}

static msg_t ee_transfer(const uint8_t *tx, size_t txn, uint8_t *rx, size_t rxn) {
    i2cAcquireBus(&I2CD1);
    msg_t status = i2cMasterTransmitTimeout(&I2CD1, EE_I2C_ADDR, tx, txn, rx, rxn, EE_TIMEOUT);
    ee_last_status = status;
    ee_last_errors = status == MSG_OK ? 0 : i2cGetErrors(&I2CD1);
    i2cReleaseBus(&I2CD1);
    if (status == MSG_TIMEOUT) { // the driver is left locked after a timeout
        i2cStop(&I2CD1);
        i2cStart(&I2CD1, &ee_i2c_config);
    }
    return status;
}

static msg_t ee_write_page(uint16_t page) {
    uint8_t  packet[2 + EE_PAGE_SIZE];
    uint16_t a = EE_BASE + page * EE_PAGE_SIZE;
    packet[0]  = a >> 8;
    packet[1]  = a & 0xFF;
    memcpy(packet + 2, ee_cache + page * EE_PAGE_SIZE, EE_PAGE_SIZE);
    return ee_transfer(packet, sizeof(packet), NULL, 0);
}

bool eeprom_i2c1_task(void) {
    if (!ee_present || ee_dirty_count == 0) {
        return false;
    }
    if (ee_writing && timer_elapsed32(ee_write_time) < EE_WRITE_MS) {
        return true;
    }
    uint16_t page = EE_PAGES;
    while (!ee_is_dirty(--page)) {
    }
    msg_t status  = ee_write_page(page);
    ee_writing    = true; // after a NACK too: the chip may still be busy
    ee_write_time = timer_read32();
    if (status == MSG_OK) {
        ee_set_dirty(page, false);
        ee_writes_ok++;
    } else {
        ee_writes_failed++;
    }
    if (status == MSG_TIMEOUT) {
        ee_present = false; // bus stuck: keep working from RAM, never stall the main loop again
    }
    return ee_present && ee_dirty_count > 0;
}

void eeprom_i2c1_flush(void) {
    for (uint16_t tries = 0; tries < 3 * EE_PAGES && eeprom_i2c1_task(); tries++) {
        wait_ms(EE_WRITE_MS);
    }
}

void eeprom_driver_init(void) {
    i2cStart(&I2CD1, &ee_i2c_config);
    // In chunks: one 2 KB read takes ~46 ms at 400 kHz, past EE_TIMEOUT.
    ee_init_status = MSG_OK;
    for (uint16_t off = 0; off < EEPROM_SIZE && ee_init_status == MSG_OK; off += EE_READ_CHUNK) {
        const uint8_t a[2] = {(EE_BASE + off) >> 8, (EE_BASE + off) & 0xFF};
        ee_init_status     = ee_transfer(a, sizeof(a), ee_cache + off, EE_READ_CHUNK);
    }
    ee_init_errors = ee_last_errors;
    ee_present     = ee_init_status == MSG_OK;
    if (!ee_present) {
        memset(ee_cache, 0xFF, sizeof(ee_cache));
    }
}

void eeprom_driver_format(bool erase) {
    if (erase) {
        eeprom_driver_erase();
    }
}

void eeprom_driver_erase(void) {
    uint8_t zero[EE_PAGE_SIZE] = {0};
    for (uintptr_t a = 0; a < EEPROM_SIZE; a += EE_PAGE_SIZE) {
        eeprom_write_block(zero, (void *)a, EE_PAGE_SIZE);
    }
}

void eeprom_read_block(void *buf, const void *addr, size_t len) {
    uintptr_t offset = (uintptr_t)addr;
    if (offset >= EEPROM_SIZE) {
        memset(buf, 0xFF, len);
        return;
    }
    if (len > EEPROM_SIZE - offset) {
        memset((uint8_t *)buf + (EEPROM_SIZE - offset), 0xFF, len - (EEPROM_SIZE - offset));
        len = EEPROM_SIZE - offset;
    }
    memcpy(buf, ee_cache + offset, len);
}

void eeprom_write_block(const void *buf, void *addr, size_t len) {
    const uint8_t *src    = buf;
    uintptr_t      offset = (uintptr_t)addr;

    while (len > 0 && offset < EEPROM_SIZE) {
        size_t n = EE_PAGE_SIZE - (offset % EE_PAGE_SIZE); // never cross a page
        if (n > len) {
            n = len;
        }
        if (memcmp(ee_cache + offset, src, n) != 0) {
            memcpy(ee_cache + offset, src, n);
            if (ee_present) {
                ee_set_dirty(offset / EE_PAGE_SIZE, true);
            }
        }
        src += n;
        offset += n;
        len -= n;
    }
}

uint8_t eeprom_i2c1_debug(uint8_t *out, uint8_t size) {
    uint8_t d[32] = {0};
    d[0]          = ee_present;
    d[1]          = (uint8_t)ee_init_status;
    d[2]          = ee_init_errors;
    d[3]          = (uint8_t)ee_last_status;
    d[4]          = ee_last_errors;
    d[5]          = ee_writes_ok >> 8;
    d[6]          = ee_writes_ok & 0xFF;
    d[7]          = ee_writes_failed >> 8;
    d[8]          = ee_writes_failed & 0xFF;
    d[9]          = ee_dirty_count >> 8;
    d[10]         = ee_dirty_count & 0xFF;
    // Live: read 8 bytes at the QMK base now, after any write cycle still running
    // (the chip NACKs everything until it is done).
    if (ee_writing) {
        wait_ms(EE_WRITE_MS);
    }
    const uint8_t a[2] = {EE_BASE >> 8, EE_BASE & 0xFF};
    d[11]              = (uint8_t)ee_transfer(a, sizeof(a), d + 13, 8);
    d[12]              = ee_last_errors;
    // Live: which 7-bit addresses 0x50..0x57 ACK a one-byte read (changes nothing on the chip).
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t byte;
        i2cAcquireBus(&I2CD1);
        msg_t st = i2cMasterReceiveTimeout(&I2CD1, 0x50 + i, &byte, 1, EE_TIMEOUT);
        i2cReleaseBus(&I2CD1);
        if (st == MSG_OK) {
            d[21] |= 1 << i;
        } else if (st == MSG_TIMEOUT) {
            i2cStop(&I2CD1);
            i2cStart(&I2CD1, &ee_i2c_config);
        }
    }
    if (size > sizeof(d)) {
        size = sizeof(d);
    }
    memcpy(out, d, size);
    return size;
}
