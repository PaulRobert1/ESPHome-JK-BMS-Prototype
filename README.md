# JK BMS UART Prototype

ESP32-C3 prototype replacing a JK BMS Bluetooth module with guarded UART monitoring/control.

## What it does

Receives battery/cell telemetry over UART and exposes guarded BMS setting controls. Sessions start disabled; model checks, fresh telemetry and a timed unlock gate writes. Hardware communication remains unverified; see DESIGN.md for wiring, assumptions and control limits.

## Hardware and configuration

- ESPHome entry point: [`jkbmsprototype.yaml`](jkbmsprototype.yaml).
- Board/profile: `esp32-c3-devkitm-1`.
- GPIO assignments, calibration constants and exposed entities are defined in the YAML. Match them to your hardware before use.
- Included local dependencies: `components/jk_bms_uart`.

## Setup

1. Keep the repository directory structure when copying it to ESPHome Device Builder.
2. Copy `secrets.example.yaml` to `secrets.yaml`; enter your Wi-Fi settings, unique OTA password and API encryption key. Generate an API key with `openssl rand -base64 32`. Set any BSSID/static-network/web-login placeholders for your own installation.
3. Review pins, sensor calibration, network settings and any Home Assistant entity references.
4. Validate with `esphome config jkbmsprototype.yaml`, then build with `esphome compile jkbmsprototype.yaml`. Choose an ESPHome version compatible with the source; the publication check uses ESPHome 2026.9.1.
5. Flash only the intended board after checking its wiring and configuration.

`secrets.yaml` and build outputs are ignored by Git. Fonts referenced with `gfonts://` require download access during preparation/build.

## Source and publication notes

Snapshot of the stored Home Assistant ESPHome source taken on **2026-10-27**. This is not proof of which firmware is currently flashed. Creating this repository does not upload firmware or change a live device.

- Credentials and network settings moved to `!secret` references.

See [VALIDATION.md](VALIDATION.md) for the publication checks and their limits.
