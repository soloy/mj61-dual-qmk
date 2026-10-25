# MelGeek MJ61 DUAL HOTSWAP (Bluetooth PCB)

A 60% hotswap RGB PCB with USB and Bluetooth. Silkscreen: `mj61_dual_hotswap_rev1`. Stock firmware: `MelGeek_MJ61_DUAL_HOTSWAP_REV1_20210701` (USB ID `0416:E324`). Not the wired MJ61 (STM32F303, `melgeek/mj61`).

* Keyboard Maintainer: soloy
* Hardware Supported: MJ61 DUAL HOTSWAP REV1 (Nuvoton NUC123 USB MCU + nRF52832 Bluetooth module)
* Hardware Availability: discontinued (MelGeek "MJ61 BT PCB")

Make example for this keyboard (after setting up your build environment):

    qmk compile -kb melgeek/mj61_dual -km via

The `via` keymap has VIA and Bluetooth (through the nRF52 with its stock MelGeek firmware), `default` has neither.

## Bootloader

The board has MelGeek's LDROM bootloader (USB `0416:A316`, Nuvoton ISP-HID style). Flash with `mj61_dual_flash.py`. Enter the bootloader in one of these ways:

* **From the stock firmware**: `mj61_dual_flash.py enter-bootloader --yes`
* **Keycode in layout**: FN2 + B (`QK_BOOT`)
* **Failsafe**: hold any key while plugging in

The bootloader stays active until new firmware is written, so the stock firmware can always be flashed back.
