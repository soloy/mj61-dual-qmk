# MelGeek LDROM bootloader

Reverse engineered from MelGeek KBTools V2.6.9 (macOS and Windows) and tested
against the real bootloader. `tools/mj61_dual_flash.py` implements it.

## Entering the bootloader

- **From the stock firmware:** send one 64-byte HID report to interface 2 of
  `0416:E324` with byte 0 = `0x10` and the rest 0. The firmware sets
  CONFIG0.CBS to boot from LDROM and resets the chip
  (`mj61_dual_flash.py enter-bootloader`).
- **From QMK:** FN2 + B (`QK_BOOT`), or hold any key while plugging in. QMK
  sets CBS the same way.

The board then shows up as `0416:A316` ("BootUSB") with one HID interface.

## Protocol

It is Nuvoton's ISP-HID protocol with MelGeek's own PID and without
`CMD_CONNECT` (`0xAE`).

- Every packet is 64 bytes (written as 65 with report ID 0), little-endian
  `u32` fields.
- The checksum is the 16-bit sum of all 64 bytes sent. The reply has the
  checksum as `u16` at offset 0 and the packet number as `u32` at offset 4.
- The packet number starts at 1 and goes up by one for every packet sent and
  every reply received (so sent packets are 1, 3, 5, ...).

| Step | Packet |
|---|---|
| Sync | `[0xA4][packno][packno][0...]` |
| Start writing | `[0xA0][packno][start address = 0][total length][48 bytes of data]` |
| More data | `[0x00][packno][56 bytes of data]`, repeated until done |
| Run the app | `[0xAB][packno][0...]` |

Send the next packet only after a reply whose checksum and packet number
match. The bootloader repeats its last reply on every IN poll, and the first
report can be all zeros, so read until the reply matches.

After `0xAB` the bootloader starts the app once but leaves CBS pointing at
LDROM; the app has to set it back (stock and QMK both do).

## KBTools

KBTools picks what to flash only from the file name (`mg_usb_app_*`,
`mg_bt_app_*`, `mg_bt_sd_*`) and does not check the contents of a `.bin`. It
also finds a board that is already in the bootloader and flashes it, which is
a way back to stock if needed.
