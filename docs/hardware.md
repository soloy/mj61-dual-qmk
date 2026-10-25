# MJ61 DUAL HOTSWAP REV1: hardware

Everything here comes from static analysis of the stock MelGeek firmware
(`mg_usb_app_MJ61_DUAL_HOTSWAP_REV1.bin`, 20210701) and was checked on a real
board. The MJ63 and MJ64 DUAL boards use the same MCU, RGB driver and
Bluetooth image. Addresses like `0x9568` are offsets in that stock image.

| | |
|---|---|
| MCU | Nuvoton NUC123LD4AN0 (Cortex-M0, 68 KB APROM, 20 KB SRAM, LQFP48) |
| Clock | 12 MHz crystal, PLL 144 MHz, HCLK 72 MHz, USB 48 MHz |
| Stock USB ID | `0416:E324`, vendor HID (usage page `0xFF01`) on interface 2, used by KBTools |
| Bootloader | MelGeek LDROM, USB `0416:A316`, see [bootloader-protocol.md](bootloader-protocol.md) |
| Matrix | 5 x 14, `ROW2COL`. Rows `C0 B10 B9 C13 C12`, columns `C1 C2 C3 C11 C10 C9 C8 A15 A14 A13 D2 D1 D3 D4`. Same key positions as the wired `melgeek/mj61` (`LAYOUT_60_ansi`) |
| RGB | IS31FL3741A on I2C0 (PF2 SDA / PF3 SCL), 7-bit address `0x30`, SDB on PA12 |
| LEDs | 71 RGB LEDs: 61 keys plus two stabilizer LEDs each under Backspace, Enter, both Shifts and Space. Channels R = CSn+2, G = CSn, B = CSn+1. Stock scaling R `0x5A`, G/B `0x69` |
| EEPROM | 24Cxx on I2C1 (PA10 SDA / PA11 SCL), 7-bit `0x50`, 16-bit addresses, at least 16 KB. Stock settings live at `0x0000-0x2DFF`; QMK uses 2 KB at `0x3800` |
| Bluetooth | nRF52832 module on UART1 (PB4 RX / PB5 TX), MelGeek protocol, see [bt-protocol.md](bt-protocol.md) |
| Charger | TP4056. CHRG on PB7, STDBY on PD5, battery voltage on PD0 (ADC) |

## Notes for the QMK port

- **EEPROM in the external 24Cxx, not in flash.** Where ChibiOS places the
  flash EEPROM (wear leveling) depends on the chip's CONFIG0/CONFIG1, and one
  combination hangs its init before the failsafe. `eeprom_i2c1.c` mirrors
  2 KB in RAM, reads the chip in 64-byte chunks at start-up and writes one
  dirty 32-byte page per main-loop pass, so a blank chip does not delay boot.
  Without the chip it keeps working from RAM (no persistence).
- **Two USB endpoints.** The ChibiOS NUC123 USB driver exposes only two
  non-control endpoints, so the keyboard uses the shared endpoint and
  `USB_ENDPOINTS_ARE_REORDERABLE` to fit raw HID (VIA).
- **LED table.** The stock LED table (`0x9568`) is indexed by key number
  0..60, not by matrix position. Entries 61..70 are the stabilizer LEDs, which
  stock copies from the colour of their key.
- **Boot selection.** After a flash the bootloader leaves CONFIG0.CBS pointing
  at LDROM, so every power-up would land in the bootloader. QMK sets it back
  to APROM once USB is configured, so a broken build still powers up into the
  bootloader.
- **USB self-rescue.** If a host starts enumerating the board but it is not
  configured within 5 s, QMK jumps to the bootloader. A charger never starts
  enumeration, so it never triggers this.
