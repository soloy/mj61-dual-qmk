# QMK + VIA for the MelGeek MJ61 DUAL HOTSWAP (Bluetooth PCB)

QMK firmware with VIA and Bluetooth for the **MelGeek MJ61 DUAL HOTSWAP REV1**
(the Bluetooth "MJ61 BT PCB", silkscreen `mj61_dual_hotswap_rev1`). The stock
firmware has no VIA support; this one does.

> [!WARNING]
> This is **not** for the wired MJ61 (STM32F303), which upstream QMK already
> supports as `melgeek/mj61`. Never flash that firmware to this board, or
> this firmware to the wired one. The flasher refuses images that are not
> built for the NUC123.

Check your board: it has a Nuvoton **NUC123** MCU (marking `NUC123LD4AN0`)
and an nRF52832 Bluetooth module, and with the stock firmware it shows up
on USB as `0416:E324`.

Flashing custom firmware is at your own risk. So far it has been tested on
one board, from macOS.

## What works

- USB keyboard, all keys and layers (layout like the MelGeek manual, below)
- VIA (remap keys and RGB in [usevia.app](https://usevia.app))
- All 71 RGB LEDs (keys and stabilizer LEDs), animations like QMK's RGB matrix
- Bluetooth over the board's own nRF52 with its stock firmware: keyboard,
  media keys, 6 channels, battery level, light feedback like stock
- Settings saved in the board's EEPROM

Not tested yet: running on a battery (only USB power so far), MJ63/MJ64
DUAL boards (similar hardware, different layouts).

## Keys

| | |
|---|---|
| FN1 + number row | F1-F12, Delete |
| FN1 + W A S D | arrows |
| FN1 + P [ ] / L ; ' / , . / | Print Screen, Scroll Lock, Pause / Insert, Home, Page Up / Delete, End, Page Down |
| FN1/FN2 + Y | calculator |
| FN2 + 7 8 9 0 - = | volume up/down, mute, previous, next, play/pause |
| FN2 + Z X C | RGB on/off, next effect, hue |
| FN2 + 1-6 | Bluetooth channel 1-6: tap to connect, hold 3 s to pair |
| FN2 + Esc | back to USB |
| FN2 + B | bootloader (for flashing) |

FN1 is the right-hand key next to Right Alt, FN2 the one after it.

## Install

1. Download `melgeek_mj61_dual_via.bin` and `mj61_dual_flash.py` from the
   [latest release](../../releases/latest) (or the `tools/` folder).
2. Install Python 3 and the HID module: `pip install hid`. It also needs the
   hidapi library (`brew install hidapi` on macOS, `libhidapi-hidraw0` on
   Debian/Ubuntu; on Windows put `hidapi.dll` next to the script).
3. Check the file (talks to nothing):

       python3 mj61_dual_flash.py check melgeek_mj61_dual_via.bin

4. With the stock firmware running, plug in the keyboard over USB and start
   the bootloader:

       python3 mj61_dual_flash.py enter-bootloader --yes

   From QMK use FN2 + B instead, or hold any key while plugging in.
5. Flash:

       python3 mj61_dual_flash.py flash melgeek_mj61_dual_via.bin --yes

The keyboard restarts into QMK. On macOS, click **Allow** when it asks
whether to let the accessory connect. If USB is not up within 5 s, the
keyboard goes back to the bootloader by itself, so just flash again.

The `default` build is the same without VIA and Bluetooth.

## VIA

Open [usevia.app](https://usevia.app), go to Settings and turn on
"Show Design tab", then load [`mj61_dual.via.json`](mj61_dual.via.json) in
the Design tab. After that the keyboard shows up in Configure.

A new firmware version resets the VIA keymap to the default.

## Back to stock

1. Hold any key while plugging in (or FN2 + B): the board starts its
   bootloader.
2. Get the stock firmware from MelGeek's download page:
   [download.melgeek.cn](https://download.melgeek.cn/p/mj61-bt-pcb/), the
   "MJ61 BT PCB" package. Use `usb/mg_usb_app_MJ61_DUAL_HOTSWAP_REV1.bin`
   (SHA256 `e4cd7bb84e6eebfbbc09466a6354082d6534e390fd818fc05796b306960964bc`).
3. Flash it the same way:

       python3 mj61_dual_flash.py flash mg_usb_app_MJ61_DUAL_HOTSWAP_REV1.bin --yes

MelGeek KBTools should also work (not tested). QMK never touches the Bluetooth chip, so its
firmware and pairings stay as they are.

## Build

Copy (or symlink) `keyboards/melgeek/mj61_dual` into a
[qmk_firmware](https://github.com/qmk/qmk_firmware) checkout, then:

    qmk compile -kb melgeek/mj61_dual -km via

GitHub Actions builds both keymaps on every push and attaches them to
releases (see `.github/workflows/build.yml` for the pinned QMK version).

## How it works

- [docs/hardware.md](docs/hardware.md): MCU, matrix, RGB, EEPROM, charger
- [docs/bootloader-protocol.md](docs/bootloader-protocol.md): the MelGeek bootloader
- [docs/bt-protocol.md](docs/bt-protocol.md): how the USB MCU talks to the nRF52

All of it comes from analysing MelGeek's stock firmware and KBTools for
interoperability. No MelGeek code is included. The board files come from the
Ducky One 2 Mini 1861ST port in QMK.

`tools/mj61_dual_eeprom_debug.py` and `tools/mj61_dual_bt_debug.py` read the
EEPROM and Bluetooth driver state over VIA, handy for bug reports.

## License

GPL-2.0-or-later, see [LICENSE](LICENSE). The files in
`keyboards/melgeek/mj61_dual/boards/` are Apache-2.0 (see their headers).
MelGeek, MJ61 and KBTools are MelGeek's names; this project is not affiliated
with MelGeek.
