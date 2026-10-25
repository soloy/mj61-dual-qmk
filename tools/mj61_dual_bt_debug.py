#!/usr/bin/env python3
"""Read the Bluetooth driver state of the MJ61 DUAL QMK `via` build over raw HID.

Sends VIA custom get_value on channel 0, value 0xEF (via_command_kb in
mj61_dual.c) and decodes the answer. Read-only. --watch repeats once a second.

    python3 mj61_dual_bt_debug.py [--watch]
"""
import sys
import time

VID, PID = 0xEDED, 0xED62
HOSTS = {0: "AUTO", 1: "NONE", 2: "USB", 3: "BLUETOOTH", 4: "2P4GHZ"}


def query(hid, value=0xEF):
    info = [d for d in hid.enumerate(VID, PID) if d["usage_page"] == 0xFF60]
    if not info:
        return None
    dev = hid.Device(path=info[0]["path"])
    try:
        dev.write(bytes([0, 0x08, 0x00, value]).ljust(33, b"\0"))
        return dev.read(32, 1000)
    finally:
        dev.close()


def show_tx(r):
    if not r or r[:3] != bytes([0x08, 0x00, 0xF0]):
        return
    d = r[3:]
    print(f"  sent {d[0] << 8 | d[1]} frames ({d[2] << 8 | d[3]} HID reports), dropped {d[20] << 8 | d[21]} (UART queue full), "
          f"last other frame {d[4:20].hex(' ')}")


def show(r):
    if not r or r[:3] != bytes([0x08, 0x00, 0xEF]):
        print("No answer or not a Bluetooth build:", r.hex() if r else None)
        return
    d = r[3:]
    print(f"nRF52 ready {bool(d[0])}, connected {bool(d[1])}, host LEDs {d[2]:#04x}, channel {d[3] + 1}, "
          f"output {HOSTS.get(d[4], d[4])} (now {HOSTS.get(d[5], d[5])}), frames {d[6] << 8 | d[7]}, "
          f"dropped bytes {d[8] << 8 | d[9]}, last frame {d[10:26].hex(' ')}, "
          f"battery ADC {d[26] << 8 | d[27]} = {d[28] if d[28] != 0xFF else '?'} %")


def main():
    import hid

    while True:
        show(query(hid))
        show_tx(query(hid, 0xF0))
        if "--watch" not in sys.argv:
            return 0
        time.sleep(1)


if __name__ == "__main__":
    sys.exit(main())
