# NUC123 <-> nRF52 protocol (UART1)

The nRF52 keeps the stock MelGeek Bluetooth firmware. The NUC123 talks to it
over UART1 (PB4 RX / PB5 TX), 115200 8N1. From static analysis of the stock
USB firmware; the parts QMK uses are confirmed on hardware.

## Frames

Both directions use 16-byte frames:

```
byte 0     cmd
byte 1     len   (data bytes, 0..12)
byte 2     sum   = low byte of the sum of all 16 bytes with byte 2 = 0
byte 3     0
byte 4..   data, rest 0
```

## NUC123 -> nRF52

| cmd | data | Meaning |
|---|---|---|
| `0x00` | - | Hello after start-up |
| `0x06` | 8 / 4 / 3 bytes | HID report, the same bytes stock sends to USB: keyboard (8), mouse (4), consumer (3, a 24-bit usage bitmap) |
| `0x0b` | `[mode, channel]` | Mode 0 = connect to channel, 1 = pair on channel. Channel is 0-based (channel 0 advertises as `MelGeek-C1`) |
| `0x0c` | `[percent]` | Battery level, sent on connect and when it changes |
| `0x09` | - | Before deep sleep (stock only) |
| `0x01`, `0x02`, `0x0e`-`0x12` | | Used by KBTools (BT name, DFU and more), not used by QMK |

## nRF52 -> NUC123

| cmd | data | Meaning |
|---|---|---|
| `0x00` | - | Ready. Stock then sends `0x0b [0, channel]` for the saved channel |
| `0x03` | `[channel, state]` | BT state for the LEDs; state 3 = advertising for pairing |
| `0x04` | `[1/0, channel]` | Connected / disconnected |
| `0x0d` | `[leds]` | Host LED state (Caps Lock) |
| `0x08`, `0x81`-`0x85` | | BT firmware update over KBTools, not used by QMK |

## What QMK does

1. Opens UART1 and sends `0x00`.
2. On `0x00` from the nRF52, sends `0x0b [0, channel]` if the output is
   Bluetooth.
3. On `0x04 [1, ...]`, sends HID reports as `0x06` and the battery as `0x0c`.
4. FN2 + 1..6: tap = `0x0b [0, ch]`, hold 3 s = `0x0b [1, ch]` (pair).
   FN2 + Esc: releases all keys over BT and switches the output to USB.
