# Drone Controller Firmware

## Purpose

Study MAVLink heartbeat/RC-channel framing and drone-signature detection. SIMULATION ONLY for live control; real bench link requires authorization.

## Board

- **Board**: ESP32 + flight-controller telemetry link (lab bench)
- **FQBN**: `esp32:esp32:esp32`
- **Sketch**: `h12_drone_ctrl/h12_drone_ctrl.ino`

## Wiring

```
Serial2 TX(17)->FC RX, RX(16)<-FC TX, GND common. ESP-NOW antenna for detection experiments with OWN lab modules.
```

## Build

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/h12_drone_ctrl
# upload (example, ESP32-C6):
# arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyACM0 firmware/h12_drone_ctrl
```

## Runtime

See the root README "IMPORTANT" section before powering on. This firmware is
for authorized own-lab study. Serial console exposes the interactive command
set described in the root README. All identifiers in the sketch are
placeholders (`lab-*` SSIDs, `00:11:22:33:44:55`, RFC 5737 / example.com).
