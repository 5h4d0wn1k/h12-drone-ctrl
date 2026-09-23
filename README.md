> **⚠️ EDUCATIONAL USE ONLY — AUTHORIZED TESTING ONLY.**
> This project exists for education, research, and **defense of systems you own
> or hold explicit written authorization to assess**. Unauthorized use is
> prohibited and may be illegal. Read [ETHICS.md](ETHICS.md) and
> [SCOPE.md](SCOPE.md) before use. Use at your own risk; **AS IS**, no warranty.

# H12 — Drone Controller

**Drone control protocol security toolkit** by **5h4d0wn1k** for **RF and
telemetry research on hardware you own**: MAVLink v1/v2 heartbeat and RC
channel framing, RC receiver emulation with PWM channel control, and ESP-NOW /
Bayang drone detection on an ESP32 — with a simulation-only host helper and
MAVLink fixtures for offline study. Never targets any aircraft, pilot or
radio you don't own and directly control.

## Why study drone telemetry

Consumer drone control links commonly ride 2.4 GHz ISM (ESP-NOW, FHSS) with
MAVLink telemetry on serial or 433/868/915 MHz — and the same packets that
carry arming commands carry replay and spoofing risk. This project teaches
the protocol layer: how heartbeats (MAVLink msg 0) and RC channel frames
(msg 35) are framed, CRC-16/MCRF4XX-validated, and parsed, and how
ESP-NOW-inspired detection identifies drone radio signatures. Everything is
bench-scoped: the ESP32 firmware drives MAVLink/RC over your own wiring, the
host helper is simulation-only, and live RF proof stays inside a shielded,
authorized lab. See [ETHICS.md](ETHICS.md) and [SCOPE.md](SCOPE.md).

## Features

- **MAVLink framing** — heartbeat (msg 0) and RC Channels (msg 35) build/parse
  with CRC-16/MCRF4XX checksum validation (`host/h12_cli.py`,
  `firmware/h12_drone_ctrl/h12_drone_ctrl.ino`).
- **RC receiver emulation** — 8 PWM channels (1000–2000 µs) at a 20 ms update
  rate with stick-jitter simulation.
- **ESP-NOW / Bayang detection** — drone protocol identification with MAC-based
  deduplication and a 5-second timeout (`detectDrones`).
- **Serial command interface** — throttle/arm handling with low-throttle arm
  gating.
- **Simulation-only host helper** — `python3 host/h12_cli.py --demo` validates
  MAVLink frames offline and exits `0`; real arm/control is never triggered.
- **Test fixtures** — offline MAVLink frame corpus in `fixtures/mavlink_frames.txt`.

## Quickstart

### Firmware (ESP32)

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/h12_drone_ctrl
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 firmware/h12_drone_ctrl
```

Bench wiring: ESP32 DevKit with the flight controller on GPIO17 (TX)/GPIO16
(RX) for MAVLink serial, on-board PCB antenna for 2.4 GHz ESP-NOW.

### Host helper (offline)

```bash
# Validate MAVLink framing, exit 0
python3 host/h12_cli.py --demo

# Validate a hex MAVLink frame log
python3 host/h12_cli.py --file fixtures/mavlink_frames.txt

# Run the offline test suite
python3 -m unittest discover -s tests
```

## Project structure

```
firmware/h12_drone_ctrl/h12_drone_ctrl.ino   # ESP32 firmware (MAVLink + ESP-NOW + RC)
host/h12_cli.py                              # simulation-only MAVLink host helper
host/hw_common.py                            # shared hardware helpers
fixtures/mavlink_frames.txt                  # offline MAVLink frame corpus
tests/                                       # unittest coverage
```

## Documentation

- [firmware README](firmware/README.md) — ESP32 build and bench details.
- [ETHICS.md](ETHICS.md) — acceptable and prohibited use.
- [SCOPE.md](SCOPE.md) — authorized target scope and shielded-lab rules.
- [SECURITY.md](SECURITY.md) — responsible disclosure.

## Contributing

New MAVLink message types, detection heuristics and fixture frames are
welcome. Open an issue or PR against the default branch; keep contributions
scoped to bench and simulation tooling.

## License

MIT — full legal shield in [LICENSE](LICENSE). Educational, authorization-
required software for studying drone telemetry on hardware you own in an
isolated lab.