#!/usr/bin/env python3
"""Read the EEPROM driver state of the MJ61 DUAL QMK `via` build over raw HID.

Sends VIA custom get_value on channel 0, value 0xEE (handled by
via_command_kb in mj61_dual.c) and decodes the answer. Read-only: it reads
8 bytes from the 24Cxx and probes 0x50..0x57 with one-byte reads.

    python3 mj61_dual_eeprom_debug.py
"""
import sys

VID, PID = 0xEDED, 0xED62
MSG = {0: "MSG_OK", 0xFF: "MSG_TIMEOUT", 0xFE: "MSG_RESET (NACK or bus error)"}
ERRS = {0x01: "BUS_ERROR", 0x02: "ARBITRATION_LOST", 0x04: "ACK_FAILURE", 0x08: "OVERRUN",
        0x10: "PEC_ERROR", 0x20: "TIMEOUT", 0x40: "SMB_ALERT"}


def errs(v):
    return ",".join(n for b, n in ERRS.items() if v & b) or "none"


def main():
    import hid

    info = [d for d in hid.enumerate(VID, PID) if d["usage_page"] == 0xFF60]
    if not info:
        print(f"No {VID:04x}:{PID:04x} raw HID interface (is the via build running?)")
        return 1
    dev = hid.Device(path=info[0]["path"])
    try:
        dev.write(bytes([0, 0x08, 0x00, 0xEE]).ljust(33, b"\0"))
        r = dev.read(32, 1000)
    finally:
        dev.close()
    if len(r) < 25 or r[:3] != bytes([0x08, 0x00, 0xEE]):
        print("Unexpected answer:", r.hex())
        return 1
    d = r[3:]
    print(f"chip present at start-up : {bool(d[0])}")
    print(f"start-up read            : {MSG.get(d[1], d[1])}, errors {errs(d[2])}")
    print(f"last transfer            : {MSG.get(d[3], d[3])}, errors {errs(d[4])}")
    print(f"page writes ok / failed  : {d[5] << 8 | d[6]} / {d[7] << 8 | d[8]}")
    print(f"pages waiting            : {d[9] << 8 | d[10]}")
    print(f"live read @0x3800        : {MSG.get(d[11], d[11])}, errors {errs(d[12])}, data {d[13:21].hex()}")
    print(f"addresses answering      : {[hex(0x50 + i) for i in range(8) if d[21] >> i & 1] or 'none'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
