# H12 — Drone Controller

MAVLink protocol handling, RC receiver emulation, and drone detection for ESP32.

## Overview

Multi-protocol drone communication tool:
- MAVLink v1/v2 heartbeat and RC channel transmission
- ESP-NOW drone detection and protocol identification
- RC receiver emulation with PWM channel control
- Serial command interface for arming/disarm/throttle
- Automatic drone discovery via ESP-NOW sniffing

## Hardware

| Component | Connection | Role |
|-----------|------------|------|
| ESP32 DevKit | Main board | MAVLink + ESP-NOW |
| Flight controller | GPIO17(TX)/GPIO16(RX) | MAVLink serial |
| Antenna | On-board PCB | 2.4 GHz reception |

## Features

- **MAVLink protocol**: Heartbeat (Msg 0) and RC Channels (Msg 35) transmission
- **ESP-NOW detection**: Identifies Bayang and ESP-NOW drone protocols
- **RC emulation**: 8-channel PWM values with stick jitter simulation
- **Drone tracking**: MAC-based deduplication with 5s timeout
- **CRC-16/MCRF4XX**: MAVLink-compliant checksum calculation

## Serial Output

```
=== H12 — Drone Controller ===
MAVLink + ESP-NOW + RC emulation active

[DRONE] New: AA:BB:CC:DD:EE:FF (proto=2)
[ARMED] Throttle low — ready to arm
[THROTTLE] 1150 µs
```

## Build & Flash

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 firmware/
```

## Legal Disclaimer

## IMPORTANT: Read before use.

This project is provided for **educational and authorized security testing purposes only**.

### Authorization Requirements
- You MUST have explicit written permission from the network owner before using this tool
- Unauthorized interception of network communications is illegal under federal and state laws
- This tool should ONLY be used on networks you own or have written authorization to test

### Spectrum Regulatory Notes (ISM / RC bands)
- RC and telemetry commonly use 2.4 GHz ISM (802.11 / ESP-NOW / FHSS)
  and 5.8 GHz; MAVLink gaps may use 433/868/915 MHz ISM/SRD.
- License-free operation must not cause harmful interference and must
  respect local power/duty limits (FCC Part 15, ETSI EN 300 328,
  ECC DEC(09)03); drones are additionally governed by aviation rules.
- MAVLink/RC output must stay on your own bench wiring/RF harness.

### No Third-Party Disruption
Arm/disarm, heartbeat, or RC traffic aimed at any aircraft, pilot, or
radio you don't own and directly control in an isolated lab is out of
scope. Proofs here are MAVLink frames/fixtures and offline simulation
only.

### Legal Framework
- **Computer Fraud and Abuse Act (CFAA)**: Unauthorized access to computer systems is a federal crime
- **Wiretap Act (18 U.S.C. § 2511)**: Interception of electronic communications without consent is illegal
- **State Laws**: Many states have additional computer crime and wiretapping statutes
- **GDPR/CCPA**: Data collection may be subject to privacy regulations

### Acceptable Use
- Testing security of your own networks
- Authorized penetration testing with written scope
- Academic research in controlled lab environments
- Security education and training

### Prohibited Use
- Intercepting communications on networks you don't own
- Attacking infrastructure without authorization
- Any activity that violates applicable laws or regulations
- Commercial use without proper licensing

### No Warranty
This software is provided "AS IS" without warranty of any kind. The author is not responsible for any misuse or damage caused by this software.

### Responsible Disclosure
If you discover vulnerabilities using this tool, follow responsible disclosure practices:
1. Report to the vendor/owner privately
2. Allow reasonable time for remediation
3. Do not exploit beyond proof of concept

## Live Lab Test Plan

Run ONLY on an isolated, authorized own-lab bench against devices, networks,
and spectrum **you own**. No third-party callers, bystanders, or spectrum users
may be within range of any test transmission.

1. **Isolate** - Put the DUT in a shielded/Faraday enclosure or a room with no
   third-party devices in range. Use attenuators on any transmit path.
2. **Own devices only** - Every target (AP, remote, tag, GPS module, drone FC,
   receiver) must be your own hardware.
3. **Lowest power, shortest duration** - Start at minimum TX power / duty cycle
   and use only the seconds needed.
4. **Record** - Save before/after logs to `reports/` (git-ignored). Never
   capture or store third-party traffic.
5. **Cleanup** - Restore placeholder SSIDs (`lab-*`), MACs (`00:11:22:33:44:55`),
   example.com / RFC5737 addresses, and clear any captured data from the device.

> Jammer / spoofer / replay projects are **proofs for study and simulation**
> only. They refuse live interference scenarios: a live bench trigger requires
> the `LAB_*` allowlist environment variable AND explicit `--yes` confirmation,
> and even then only against your own hardware in a shielded bench.

## Metrics

| Metric | Target | Where |
|---|---|---|
| Firmware compile | `arduino-cli compile --fqbn esp32:esp32:esp32 firmware/h12_drone_ctrl` PASS | CI/local |
| Host helper | `python3 host/h12_cli.py --demo` exits 0 (offline) | host/ |
| Unit tests | `python3 -m unittest discover -s tests` passes | tests/ |
| py_compile | every `host/*.py` compiles clean | CI/local |

## License

MIT
