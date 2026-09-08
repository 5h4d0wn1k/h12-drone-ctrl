#!/usr/bin/env python3
"""H12 - Drone Controller host helper: SIMULATION-ONLY MAVLink framing.
CRC-16/MCRF4XX validator for heartbeat + RC channel frames.
Real arm/control is never triggered from this script.
Educational/authorized own-lab use only (see README "IMPORTANT").
"""
import argparse
import os
import sys

MOD = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, MOD)
from hw_common import DEMO_TAG, read_target


def crc_accumulate(crc, b):
    tmp = b ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    crc = (crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)
    return crc & 0xFFFF


def mavlink_crc(payload):
    crc = 0xFFFF
    for b in payload:
        crc = crc_accumulate(crc, b)
    return crc


def build_heartbeat():
    payload = bytearray(9)
    payload[0] = 0  # type generic
    return bytes(payload)


def build_rc_channels(values):
    payload = bytearray(2 + 2 * len(values))
    payload[0] = 4  # time_boot_ms low byte
    payload[1] = len(values)  # chan_count
    for i, v in enumerate(values):
        payload[2 + i * 2] = v & 0xFF
        payload[3 + i * 2] = (v >> 8) & 0xFF
    return bytes(payload)


def frame(msgid, payload):
    # MAVLink v1 minimal framing
    seq = sys_id = 0
    comp = 1
    header = bytes([0xFE, len(payload), seq, sys_id, comp, msgid])
    crc = mavlink_crc(header[1:] + payload)
    return header + payload + bytes([crc & 0xFF, crc >> 8])


def analyze(text):
    frames = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            raw = bytes.fromhex(line)
        except ValueError:
            continue
        if len(raw) < 9:
            continue
        if raw[0] != 0xFE:
            continue
        length = raw[1]
        payload = raw[6:6 + length]
        rx_crc = (raw[6 + length + 1] << 8) | raw[6 + length]
        want = mavlink_crc(raw[1:6 + length])
        frames.append({"msgid": raw[5], "len": length,
                       "crc_ok": (rx_crc == want)})
    return frames


def run_demo():
    print("=== H12 MAVLink framing (SIMULATION ONLY) ===")
    hb = frame(0, build_heartbeat())
    rc = frame(35, build_rc_channels([1500] * 8))
    print("  HEARTBEAT frame: %d bytes" % len(hb))
    print("  RC_CHANNELS frame: %d bytes" % len(rc))
    print("  shell assert: frames parse + CRCs valid",
          all(f["crc_ok"] for f in analyze(
              hb.hex() + "\n" + rc.hex())))
    print(DEMO_TAG)
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(
        description="H12 Drone controller - simulation-only MAVLink validator")
    p.add_argument("--demo", action="store_true", help="offline demo (exit 0)")
    p.add_argument("--file", help="hex MAVLink frame log")
    args = p.parse_args(argv)
    if args.demo or not args.file:
        return run_demo()
    for f in analyze(open(args.file).read()):
        print(f)
    return 0


if __name__ == "__main__":
    sys.exit(main())
