// Copyright 2026 soloy
// SPDX-License-Identifier: GPL-2.0-or-later
#include "quantum.h"
#include "eeprom_i2c1.h"
#ifdef VIA_ENABLE
#    include "via.h"
#    include "raw_hid.h"
#endif
#ifdef BLUETOOTH_ENABLE
#    include "mj61_bt.h"

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    return mj61_bt_process_record(keycode, record) && process_record_user(keycode, record);
}
#endif

#ifdef RGB_MATRIX_ENABLE
/*
 * Generated from the stock firmware (LED table at 0x9568).
 * Channel order R = CSn+2, G = CSn, B = CSn+1 follows how the stock firmware
 * and KBTools handle key colours (see docs/hardware.md).
 */
const is31fl3741_led_t PROGMEM g_is31fl3741_leds[IS31FL3741_LED_COUNT] = {
/* From the stock firmware LED table @0x9568
 *   driver   R          G          B */
    {0, SW1_CS9,   SW1_CS7,   SW1_CS8  }, /* key  0 [0, 0] */
    {0, SW2_CS9,   SW2_CS7,   SW2_CS8  }, /* key  1 [0, 1] */
    {0, SW3_CS9,   SW3_CS7,   SW3_CS8  }, /* key  2 [0, 2] */
    {0, SW4_CS9,   SW4_CS7,   SW4_CS8  }, /* key  3 [0, 3] */
    {0, SW5_CS9,   SW5_CS7,   SW5_CS8  }, /* key  4 [0, 4] */
    {0, SW6_CS9,   SW6_CS7,   SW6_CS8  }, /* key  5 [0, 5] */
    {0, SW7_CS9,   SW7_CS7,   SW7_CS8  }, /* key  6 [0, 6] */
    {0, SW8_CS9,   SW8_CS7,   SW8_CS8  }, /* key  7 [0, 7] */
    {0, SW9_CS9,   SW9_CS7,   SW9_CS8  }, /* key  8 [0, 8] */
    {0, SW1_CS24,  SW1_CS22,  SW1_CS23 }, /* key  9 [0, 9] */
    {0, SW2_CS24,  SW2_CS22,  SW2_CS23 }, /* key 10 [0,10] */
    {0, SW3_CS24,  SW3_CS22,  SW3_CS23 }, /* key 11 [0,11] */
    {0, SW4_CS24,  SW4_CS22,  SW4_CS23 }, /* key 12 [0,12] */
    {0, SW6_CS24,  SW6_CS22,  SW6_CS23 }, /* key 13 [0,13] */
    {0, SW1_CS12,  SW1_CS10,  SW1_CS11 }, /* key 14 [1, 0] */
    {0, SW2_CS12,  SW2_CS10,  SW2_CS11 }, /* key 15 [1, 1] */
    {0, SW3_CS12,  SW3_CS10,  SW3_CS11 }, /* key 16 [1, 2] */
    {0, SW4_CS12,  SW4_CS10,  SW4_CS11 }, /* key 17 [1, 3] */
    {0, SW5_CS12,  SW5_CS10,  SW5_CS11 }, /* key 18 [1, 4] */
    {0, SW6_CS12,  SW6_CS10,  SW6_CS11 }, /* key 19 [1, 5] */
    {0, SW7_CS12,  SW7_CS10,  SW7_CS11 }, /* key 20 [1, 6] */
    {0, SW8_CS12,  SW8_CS10,  SW8_CS11 }, /* key 21 [1, 7] */
    {0, SW9_CS12,  SW9_CS10,  SW9_CS11 }, /* key 22 [1, 8] */
    {0, SW8_CS24,  SW8_CS22,  SW8_CS23 }, /* key 23 [1, 9] */
    {0, SW9_CS24,  SW9_CS22,  SW9_CS23 }, /* key 24 [1,10] */
    {0, SW1_CS27,  SW1_CS25,  SW1_CS26 }, /* key 25 [1,11] */
    {0, SW2_CS27,  SW2_CS25,  SW2_CS26 }, /* key 26 [1,12] */
    {0, SW3_CS27,  SW3_CS25,  SW3_CS26 }, /* key 27 [1,13] */
    {0, SW1_CS15,  SW1_CS13,  SW1_CS14 }, /* key 28 [2, 0] */
    {0, SW2_CS15,  SW2_CS13,  SW2_CS14 }, /* key 29 [2, 1] */
    {0, SW3_CS15,  SW3_CS13,  SW3_CS14 }, /* key 30 [2, 2] */
    {0, SW4_CS15,  SW4_CS13,  SW4_CS14 }, /* key 31 [2, 3] */
    {0, SW5_CS15,  SW5_CS13,  SW5_CS14 }, /* key 32 [2, 4] */
    {0, SW6_CS15,  SW6_CS13,  SW6_CS14 }, /* key 33 [2, 5] */
    {0, SW7_CS15,  SW7_CS13,  SW7_CS14 }, /* key 34 [2, 6] */
    {0, SW8_CS15,  SW8_CS13,  SW8_CS14 }, /* key 35 [2, 7] */
    {0, SW9_CS15,  SW9_CS13,  SW9_CS14 }, /* key 36 [2, 8] */
    {0, SW4_CS27,  SW4_CS25,  SW4_CS26 }, /* key 37 [2, 9] */
    {0, SW5_CS27,  SW5_CS25,  SW5_CS26 }, /* key 38 [2,10] */
    {0, SW6_CS27,  SW6_CS25,  SW6_CS26 }, /* key 39 [2,11] */
    {0, SW8_CS27,  SW8_CS25,  SW8_CS26 }, /* key 40 [2,12] */
    {0, SW2_CS18,  SW2_CS16,  SW2_CS17 }, /* key 41 [3, 0] */
    {0, SW4_CS18,  SW4_CS16,  SW4_CS17 }, /* key 42 [3, 2] */
    {0, SW5_CS18,  SW5_CS16,  SW5_CS17 }, /* key 43 [3, 3] */
    {0, SW6_CS18,  SW6_CS16,  SW6_CS17 }, /* key 44 [3, 4] */
    {0, SW7_CS18,  SW7_CS16,  SW7_CS17 }, /* key 45 [3, 5] */
    {0, SW8_CS18,  SW8_CS16,  SW8_CS17 }, /* key 46 [3, 6] */
    {0, SW9_CS18,  SW9_CS16,  SW9_CS17 }, /* key 47 [3, 7] */
    {0, SW1_CS30,  SW1_CS28,  SW1_CS29 }, /* key 48 [3, 8] */
    {0, SW2_CS30,  SW2_CS28,  SW2_CS29 }, /* key 49 [3, 9] */
    {0, SW3_CS30,  SW3_CS28,  SW3_CS29 }, /* key 50 [3,10] */
    {0, SW5_CS30,  SW5_CS28,  SW5_CS29 }, /* key 51 [3,11] */
    {0, SW7_CS30,  SW7_CS28,  SW7_CS29 }, /* key 52 [3,12] */
    {0, SW1_CS21,  SW1_CS19,  SW1_CS20 }, /* key 53 [4, 0] */
    {0, SW2_CS21,  SW2_CS19,  SW2_CS20 }, /* key 54 [4, 1] */
    {0, SW3_CS21,  SW3_CS19,  SW3_CS20 }, /* key 55 [4, 3] */
    {0, SW5_CS21,  SW5_CS19,  SW5_CS20 }, /* key 56 [4, 5] */
    {0, SW7_CS21,  SW7_CS19,  SW7_CS20 }, /* key 57 [4, 9] */
    {0, SW8_CS21,  SW8_CS19,  SW8_CS20 }, /* key 58 [4,10] */
    {0, SW9_CS21,  SW9_CS19,  SW9_CS20 }, /* key 59 [4,11] */
    {0, SW9_CS30,  SW9_CS28,  SW9_CS29 }, /* key 60 [4,13] */
    {0, SW5_CS24,  SW5_CS22,  SW5_CS23 }, /* key 13 stabilizer */
    {0, SW7_CS24,  SW7_CS22,  SW7_CS23 }, /* key 13 stabilizer */
    {0, SW7_CS27,  SW7_CS25,  SW7_CS26 }, /* key 40 stabilizer */
    {0, SW9_CS27,  SW9_CS25,  SW9_CS26 }, /* key 40 stabilizer */
    {0, SW1_CS18,  SW1_CS16,  SW1_CS17 }, /* key 41 stabilizer */
    {0, SW3_CS18,  SW3_CS16,  SW3_CS17 }, /* key 41 stabilizer */
    {0, SW6_CS30,  SW6_CS28,  SW6_CS29 }, /* key 52 stabilizer */
    {0, SW8_CS30,  SW8_CS28,  SW8_CS29 }, /* key 52 stabilizer */
    {0, SW4_CS21,  SW4_CS19,  SW4_CS20 }, /* key 56 stabilizer */
    {0, SW6_CS21,  SW6_CS19,  SW6_CS20 }, /* key 56 stabilizer */
};

// Per-channel current scaling used by the stock firmware (R lower than G/B),
// instead of QMK's default 0xFF on every channel.
void keyboard_post_init_kb(void) {
    for (int i = 0; i < IS31FL3741_LED_COUNT; i++) {
        is31fl3741_set_scaling_registers(&g_is31fl3741_leds[i], 0x5A, 0x69, 0x69);
    }
    is31fl3741_update_led_control_registers(0);
    keyboard_post_init_user();
}

// The ChibiOS NUC123 I2C driver muxes PF2/PF3 to I2C0 itself in i2c_lld_init(),
// and QMK's default i2c_init() uses STM32-style pin modes. Nothing to do here.
void i2c_init(void) {}
#endif

/*
 * Entering the MelGeek bootloader (LDROM, USB 0416:A316, used by KBTools).
 *
 * Mirrors what the stock firmware does (mg_usb_app_MJ61_DUAL_HOTSWAP_REV1.bin,
 * routine at 0x1B20): set CONFIG0.CBS to 01 (boot from LDROM), set
 * ISPCON.BS and do a chip reset. CONFIG0 is persistent, so the board keeps
 * starting in the bootloader until KBTools has written new firmware.
 */
#define NUC123_CONFIG0 0x00300000UL
#define NUC123_CONFIG1 0x00300004UL
#define ISP_CMD_READ 0x00
#define ISP_CMD_PROGRAM 0x21
#define ISP_CMD_PAGE_ERASE 0x22
#define CONFIG0_CBS_Msk 0xC0UL
#define CONFIG0_CBS_LDROM 0x40UL

static uint32_t fmc_command(uint32_t cmd, uint32_t addr, uint32_t data) {
    FMC->ISPCMD = cmd;
    FMC->ISPADR = addr;
    FMC->ISPDAT = data;
    FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
    __ISB();
    while (FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk) {
    }
    return FMC->ISPDAT;
}

void bootloader_jump(void) {
    __disable_irq();

    // Unlock protected registers.
    do {
        SYS->REGWRPROT = 0x59;
        SYS->REGWRPROT = 0x16;
        SYS->REGWRPROT = 0x88;
    } while (SYS->REGWRPROT != 1);

    FMC->ISPCON |= FMC_ISPCON_ISPEN_Msk | FMC_ISPCON_CFGUEN_Msk;

    uint32_t config0 = fmc_command(ISP_CMD_READ, NUC123_CONFIG0, 0);
    uint32_t config1 = fmc_command(ISP_CMD_READ, NUC123_CONFIG1, 0);

    if ((config0 & CONFIG0_CBS_Msk) != CONFIG0_CBS_LDROM) {
        config0 = (config0 & ~CONFIG0_CBS_Msk) | CONFIG0_CBS_LDROM;
        fmc_command(ISP_CMD_PAGE_ERASE, NUC123_CONFIG0, 0);
        fmc_command(ISP_CMD_PROGRAM, NUC123_CONFIG0, config0);
        fmc_command(ISP_CMD_PROGRAM, NUC123_CONFIG1, config1);
    }

    FMC->ISPCON |= FMC_ISPCON_BS_Msk;
    SYS->IPRSTC1 |= SYS_IPRSTC1_CHIP_RST_Msk;
    while (1) {
    }
}

/*
 * Leaving the bootloader for good. After a flash the MelGeek LDROM starts the
 * app once (ISP command 0xAB) but leaves CONFIG0.CBS at 01, so every power-up
 * lands in the bootloader again. The stock firmware puts CBS back to 10 (boot
 * from APROM, routine at 0x1BAE). QMK does the same, but only once USB has
 * been configured: until then a QMK that crashes or has broken USB still
 * powers up into the bootloader and can be reflashed.
 */
#define CONFIG0_CBS_APROM 0x80UL

static void boot_from_aprom_once_usb_works(void) {
    static bool done;
    if (done || USBD1.state != USB_ACTIVE) {
        return;
    }
    done = true;

    __disable_irq();
    do {
        SYS->REGWRPROT = 0x59;
        SYS->REGWRPROT = 0x16;
        SYS->REGWRPROT = 0x88;
    } while (SYS->REGWRPROT != 1);
    uint32_t ispcon = FMC->ISPCON;
    FMC->ISPCON = ispcon | FMC_ISPCON_ISPEN_Msk | FMC_ISPCON_CFGUEN_Msk;

    uint32_t config0 = fmc_command(ISP_CMD_READ, NUC123_CONFIG0, 0);
    uint32_t config1 = fmc_command(ISP_CMD_READ, NUC123_CONFIG1, 0);
    if ((config0 & CONFIG0_CBS_Msk) == CONFIG0_CBS_LDROM) {
        config0 = (config0 & ~CONFIG0_CBS_Msk) | CONFIG0_CBS_APROM;
        fmc_command(ISP_CMD_PAGE_ERASE, NUC123_CONFIG0, 0);
        fmc_command(ISP_CMD_PROGRAM, NUC123_CONFIG0, config0);
        fmc_command(ISP_CMD_PROGRAM, NUC123_CONFIG1, config1);
    }

    FMC->ISPCON = ispcon;
    SYS->REGWRPROT = 0; // lock again
    __enable_irq();
}

// eeprom_i2c1.c defers EEPROM writes: one page per main loop pass, the rest
// before QMK resets or jumps to the bootloader.
void housekeeping_task_kb(void) {
    boot_from_aprom_once_usb_works();
    eeprom_i2c1_task();
    housekeeping_task_user();
}

bool shutdown_kb(bool jump_to_bootloader) {
    eeprom_i2c1_flush();
    return shutdown_user(jump_to_bootloader);
}

#ifdef VIA_ENABLE
/*
 * Debug: VIA custom get_value on channel 0 (unused by the VIA definition),
 * value 0xEE, returns the EEPROM driver state from data[3] on, see
 * eeprom_i2c1_debug() and tools/mj61_dual_eeprom_debug.py.
 */
bool via_command_kb(uint8_t *data, uint8_t length) {
    if (length >= 4 && data[0] == id_custom_get_value && data[1] == id_custom_channel && data[2] == 0xEE) {
        eeprom_i2c1_debug(data + 3, length - 3);
        raw_hid_send(data, length);
        return true;
    }
#    ifdef BLUETOOTH_ENABLE
    if (length >= 4 && data[0] == id_custom_get_value && data[1] == id_custom_channel && (data[2] == 0xEF || data[2] == 0xF0)) {
        if (data[2] == 0xEF) {
            mj61_bt_debug(data + 3, length - 3);
        } else {
            mj61_bt_debug_tx(data + 3, length - 3);
        }
        raw_hid_send(data, length);
        return true;
    }
#    endif
    return false;
}
#endif

/*
 * Failsafe: holding any key while plugging in jumps to the bootloader.
 * The pin table is known from the stock firmware, but the key positions are
 * not, so this does not rely on bootmagic finding the right key.
 */
void keyboard_pre_init_kb(void) {
    static const pin_t rows[] = MATRIX_ROW_PINS;
    static const pin_t cols[] = MATRIX_COL_PINS;

    for (uint8_t r = 0; r < ARRAY_SIZE(rows); r++) {
        gpio_set_pin_input_high(rows[r]);
    }
    for (uint8_t c = 0; c < ARRAY_SIZE(cols); c++) {
        gpio_set_pin_output(cols[c]);
        gpio_write_pin_low(cols[c]);
    }
    wait_ms(5);

    bool pressed = false;
    for (uint8_t r = 0; r < ARRAY_SIZE(rows); r++) {
        if (!gpio_read_pin(rows[r])) {
            pressed = true;
        }
    }

    for (uint8_t c = 0; c < ARRAY_SIZE(cols); c++) {
        gpio_set_pin_input_high(cols[c]);
    }

    if (pressed) {
        bootloader_jump();
    }

    keyboard_pre_init_user();
}

/*
 * USB self-rescue: if a host has started talking to us (first GET_DESCRIPTOR)
 * but MJ61_USB_RESCUE_MS later the device has neither an address nor a
 * configuration, USB is broken in this build, so jump to the MelGeek
 * bootloader where KBTools can restore the stock firmware. A charger never sends a SETUP packet, so it
 * never arms this. Runs from a ChibiOS virtual timer, independent of the main
 * loop (which starts late on a blank EEPROM). Armed once per power-up.
 *
 * get_usb_descriptor() is called from the USB interrupt (EP0 setup); the
 * linker routes it through __wrap_get_usb_descriptor (rules.mk).
 */
#ifndef MJ61_USB_RESCUE_MS
#    define MJ61_USB_RESCUE_MS 5000
#endif

#if MJ61_USB_RESCUE_MS > 0
uint16_t __real_get_usb_descriptor(const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, const void **const DescriptorAddress);

static virtual_timer_t usb_rescue_vt;
static bool            usb_rescue_armed;
static volatile uint32_t usb_rescue_ms = MJ61_USB_RESCUE_MS; // volatile: kept in RAM so the emulator checks can shorten it

static void usb_rescue_check(virtual_timer_t *vtp, void *arg) {
    (void)vtp;
    (void)arg;
    // Only when the host never managed to give us an address. An addressed
    // but unconfigured device is a host decision, not broken USB: macOS on
    // Apple Silicon holds a new device there until the user clicks Allow.
    if (USBD1.configuration == 0 && USBD1.address == 0) {
        bootloader_jump();
    }
}

// Called from the USB interrupt for every GET_DESCRIPTOR with the size
// returned (0 = not found, the request gets a STALL). For the usbdiag keymap.
__attribute__((weak)) void mj61_usb_descriptor_hook(uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint16_t size) {}

uint16_t __wrap_get_usb_descriptor(const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, const void **const DescriptorAddress) {
    if (!usb_rescue_armed) {
        usb_rescue_armed = true;
        chSysLockFromISR();
        chVTSetI(&usb_rescue_vt, TIME_MS2I(usb_rescue_ms), usb_rescue_check, NULL);
        chSysUnlockFromISR();
    }
    uint16_t size = __real_get_usb_descriptor(wValue, wIndex, wLength, DescriptorAddress);
    mj61_usb_descriptor_hook(wValue, wIndex, wLength, size);
    return size;
}
#endif
