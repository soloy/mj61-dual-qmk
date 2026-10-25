#!/usr/bin/env python3
"""Flasher for the MelGeek MJ61 DUAL HOTSWAP (Bluetooth PCB) USB MCU.

Speaks the protocol of the MelGeek LDROM bootloader (USB 0416:A316), as
reverse engineered from KBTools V2.6.9 (see docs/bootloader-protocol.md):
Nuvoton ISP-HID style, 64-byte reports, commands 0xA4 (sync packno),
0xA0 (update APROM), 0x00 (continue) and 0xAB (run APROM).

Nothing is written to the keyboard unless you pass both `flash` and `--yes`.

    python3 mj61_dual_flash.py check FILE.bin
    python3 mj61_dual_flash.py flash FILE.bin --dry-run
    python3 mj61_dual_flash.py enter-bootloader --yes   # stock firmware only
    python3 mj61_dual_flash.py flash FILE.bin --yes

The commands that talk to the keyboard need a Python HID module: `hid`
(`pip install hid`, needs the hidapi library) or cython-hidapi.
"""
import argparse
import struct
import sys
import time

VID = 0x0416
PID_APP = 0xE324  # MelGeek stock firmware
PID_BOOTLOADER = 0xA316  # MelGeek LDROM bootloader
APP_INTERFACE = 2  # vendor HID (usage page 0xFF01) in the stock firmware

CMD_UPDATE_APROM = 0xA0
CMD_SYNC_PACKNO = 0xA4
CMD_RUN_APROM = 0xAB
CMD_CONTINUE = 0x00
CMD_ENTER_DFU = 0x10  # sent to the stock app, see Device::enterUsbMcuDfu

PACKET_SIZE = 64
FIRST_CHUNK = 48
NEXT_CHUNK = 56

APROM_SIZE = 68 * 1024
SRAM_START = 0x20000000
SRAM_END = 0x20005000  # NUC123xD4: 20 KB SRAM


def checksum(packet):
    return sum(packet) & 0xFFFF


def check_image(data):
    """Return a list of problems; empty means the file looks like a NUC123 APROM image."""
    problems = []
    if len(data) < 8:
        return ["file is too small"]
    if len(data) > APROM_SIZE:
        problems.append(f"file is {len(data)} bytes, APROM is only {APROM_SIZE}")
    sp, reset = struct.unpack_from("<II", data, 0)
    if not SRAM_START < sp <= SRAM_END:
        problems.append(f"initial SP {sp:#010x} is not in NUC123 SRAM (STM32 or other image?)")
    if not reset & 1:
        problems.append(f"reset vector {reset:#010x} is not a Thumb address")
    if not (reset & ~1) < len(data):
        problems.append(f"reset vector {reset:#010x} points outside the image (built for 0x08000000?)")
    return problems


def build_packets(data, start_address=0):
    """Yield (description, packet) in the order KBTools sends them, with packno 1, 3, 5, ..."""
    packno = 1
    sync = struct.pack("<III", CMD_SYNC_PACKNO, packno, packno).ljust(PACKET_SIZE, b"\0")
    yield "sync", packno, sync
    packno += 2

    first = data[:FIRST_CHUNK]
    pkt = struct.pack("<IIII", CMD_UPDATE_APROM, packno, start_address, len(data)) + first
    yield "update_aprom", packno, pkt.ljust(PACKET_SIZE, b"\0")
    packno += 2

    offset = len(first)
    while offset < len(data):
        chunk = data[offset:offset + NEXT_CHUNK]
        pkt = struct.pack("<II", CMD_CONTINUE, packno) + chunk
        yield f"data @{offset:#06x}", packno, pkt.ljust(PACKET_SIZE, b"\0")
        packno += 2
        offset += len(chunk)

    run = struct.pack("<II", CMD_RUN_APROM, packno).ljust(PACKET_SIZE, b"\0")
    yield "run_aprom", packno, run


def open_device(hid, pid, interface=None):
    for info in hid.enumerate(VID, pid):
        if interface is None or info.get("interface_number") == interface:
            if hasattr(hid, "Device"):  # `hid` (ctypes, what the flake ships)
                return hid.Device(path=info["path"])
            dev = hid.device()  # cython-hidapi
            dev.open_path(info["path"])
            return dev
    return None


def wait_for_device(hid, pid, timeout_s):
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        dev = open_device(hid, pid)
        if dev:
            return dev
        time.sleep(0.2)
    return None


def send(dev, packet):
    # hidapi: first byte is the report ID (0), followed by the 64-byte report.
    written = dev.write(b"\0" + packet)
    if written < 0:
        raise IOError("hid write failed")


def transact(dev, packet, packno, timeout_ms=2000):
    # The LDROM repeats its last reply on every IN poll, and the host queue can
    # start with an all-zero report, so skip replies until the matching one
    # (like KBTools, which ignores replies that do not match). An old reply can
    # not match: packno and checksum change with every packet.
    send(dev, packet)
    want_sum, want_packno = checksum(packet), packno + 1
    deadline = time.time() + timeout_ms / 1000
    last = b""
    while time.time() < deadline:
        resp = bytes(dev.read(PACKET_SIZE, 100))
        if len(resp) < 8:
            continue
        last = resp
        resp_sum, = struct.unpack_from("<H", resp, 0)
        resp_packno, = struct.unpack_from("<I", resp, 4)
        if resp_sum == want_sum and resp_packno == want_packno:
            return
    raise IOError(f"no matching response for packno {packno} (want checksum {want_sum:#06x}, "
                  f"packno {want_packno}); last reply {last[:8].hex() or 'none'}")


def cmd_check(args):
    data = open(args.file, "rb").read()
    problems = check_image(data)
    sp, reset = struct.unpack_from("<II", data, 0)
    print(f"{args.file}: {len(data)} bytes, SP={sp:#010x}, reset={reset:#010x}")
    for p in problems:
        print("  PROBLEM:", p)
    if not problems:
        n = sum(1 for _ in build_packets(data))
        print(f"  OK: looks like a NUC123 APROM image, {n} packets to send")
    return 1 if problems else 0


def cmd_flash(args):
    data = open(args.file, "rb").read()
    problems = check_image(data)
    if problems:
        for p in problems:
            print("PROBLEM:", p)
        print("Refusing to flash.")
        return 1

    packets = list(build_packets(data))
    if args.dry_run or not args.yes:
        for desc, packno, pkt in packets[:3] + packets[-2:]:
            print(f"{desc:14s} packno={packno:4d} sum={checksum(pkt):#06x} {pkt[:24].hex()}...")
        print(f"... {len(packets)} packets in total. Dry run, nothing sent.")
        if not args.dry_run:
            print("Add --yes to actually flash.")
        return 0

    import hid

    dev = open_device(hid, PID_BOOTLOADER)
    if not dev:
        print(f"Bootloader {VID:04x}:{PID_BOOTLOADER:04x} not found. "
              "Put the keyboard in bootloader mode first.")
        return 1
    try:
        for i, (desc, packno, pkt) in enumerate(packets):
            if desc == "run_aprom":
                # The device resets, no reply expected. It can reset before
                # the host sees the write complete ("device not responding").
                try:
                    send(dev, pkt)
                except Exception as e:  # noqa: BLE001 - hid raises its own types
                    print(f"\n(run_aprom write not confirmed: {e}; the keyboard restarted first)")
            else:
                transact(dev, pkt, packno)
            print(f"\r{i + 1}/{len(packets)} {desc}      ", end="", flush=True)
        print("\nDone. The keyboard should restart into the new firmware.")
    finally:
        dev.close()
    return 0


def cmd_enter_bootloader(args):
    if not args.yes:
        print("This sends 0x10 to the stock MelGeek firmware, which then switches the "
              "board into its bootloader (persistent until new firmware is written). "
              "Add --yes to do it.")
        return 0
    import hid

    dev = open_device(hid, PID_APP, APP_INTERFACE)
    if not dev:
        print(f"Stock firmware {VID:04x}:{PID_APP:04x} (interface {APP_INTERFACE}) not found.")
        return 1
    try:
        # The stock firmware detaches at once, so the write can fail
        # ("device not responding") although the command arrived.
        try:
            send(dev, bytes([CMD_ENTER_DFU]).ljust(PACKET_SIZE, b"\0"))
        except Exception as e:  # noqa: BLE001 - hid raises its own types
            print(f"(write not confirmed: {e}; waiting for the bootloader anyway)")
    finally:
        try:
            dev.close()
        except Exception:  # noqa: BLE001 - the device is already gone
            pass
    boot = wait_for_device(hid, PID_BOOTLOADER, 10)
    if boot:
        boot.close()
        print(f"Bootloader {VID:04x}:{PID_BOOTLOADER:04x} is up.")
        return 0
    print("Bootloader did not show up within 10 s.")
    return 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("check", help="validate a firmware image, talks to nothing")
    c.add_argument("file")
    f = sub.add_parser("flash", help="flash a firmware image through the bootloader")
    f.add_argument("file")
    f.add_argument("--dry-run", action="store_true", help="only print the packets")
    f.add_argument("--yes", action="store_true", help="really write to the keyboard")
    e = sub.add_parser("enter-bootloader", help="ask the stock firmware to start its bootloader")
    e.add_argument("--yes", action="store_true")
    args = ap.parse_args()
    return {"check": cmd_check, "flash": cmd_flash, "enter-bootloader": cmd_enter_bootloader}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
