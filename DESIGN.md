# JK Bluetooth-module replacement — first ESPHome prototype

Status: prototype, not yet connected to the user's BMS. Device name `jkbmsprototype`.
The original Bluetooth firmware was NOT extracted. No BMS firmware or settings have been changed.

## Design and evidence

The direct UART transport reuses the decoder and register mappings from syssi/esphome-jk-bms
commit `58385c78dc7248bd7bc1040677b05a46a5be42d3`, Apache-2.0. BLE dependencies are removed.
The modified component is included under `components/jk_bms_uart`, with its upstream commit attribution and Apache-2.0 license. The full upstream research checkout is not included.

* Detached original module answered `JDY-18-V1.91`, baud setting 8 (115200). This does not prove the installed BMS-side protocol.
* User traced the module side-pad connections: TXD_DC, RXD_DD, VCC, GND, RESET, PWRC, ALED, STAT; pads 24/25 reported mechanical support.
* JDY-18 UART mode uses STAT low/disconnected, high/connected. SELECT high/floating selects UART; low selects I2C.
* Upstream discussion #402 documents a NEEY JDY-18 replacement and its AT/OK boot check; maintainer separately reports JK stream activation via a control pin. These are leads, not a capture of this board.
* User supplied model JK-B2A20S20P. Existing HA entities report JK-B2A24S20P / hardware 10.XW / firmware 10.09. They may be a different unit. Default expected model remains the user-supplied model; mismatch blocks controls.

Sources:
- https://github.com/syssi/esphome-jk-bms
- https://github.com/syssi/esphome-jk-bms/discussions/402
- https://github.com/syssi/esphome-jk-bms/issues/458 (B2A20S20P v11.284, JK02_32S)
- https://github.com/syssi/esphome-jk-bms/pull/456 (v10 support)
- https://manuals.plus/m/2bc7710855302ade0244c35e226ec65c88d766093ddd30671c06938e8ff58fd3

## Behaviour

On boot: UART reception active, session OFF, writes locked. STAT/ALED outputs are LOW and UART TX idles HIGH even with the session off; this is not an electrically passive tap. Complete voltage verification before wiring any outputs. The prototype does not restore control-enable state or resend BMS settings.
Enable `BMS UART session` only after electrical verification. It answers a small whitelist of AT queries and raises STAT/ALED after two seconds, then requests device info and settings. The timing/handshake is provisional and must be checked on the actual board. Unknown AT commands are not acknowledged. No Bluetooth radio is used.

Valid identity frames select JK02_24S for hardware major 6–10 or JK02_32S for 11–19. Other generations are blocked. Model must match `expected_bms_model` to unlock controls. Decoder selection is based on upstream family rules, not a guarantee of a particular unit's compatibility.

Controls: charge, discharge, balancer; rated capacity; balance trigger/start/current; charge/discharge current limits; cell over/undervoltage protection and recovery. They require live telemetry and a 60-second unlock. One change can be in flight. Only a subsequently reported matching value is called verified. A timeout reports unknown outcome, locks controls, and never retries automatically. No raw register writes, firmware update, calibration, emergency mode, temperature-sensor disable, cell-count changes or protection bypass are exposed.

Current controls are capped at 200 A and balancing at 2 A for the nominated model. These are module-rating ceilings, NOT recommendations for the pack/wiring. Voltage edits must preserve protection/recovery ordering. All values come from the BMS; YAML does not preload replacement protection values.

300-byte frames use a fixed-size parser, additive checksum, garbage resynchronisation and a 250 ms partial-frame timeout. Per-loop RX processing is bounded. Connectivity expires after 15 seconds without cell telemetry. Buffering/logging do not accumulate unbounded history. Raw identity packets/passcodes are not logged. Wi-Fi, API encryption and OTA secrets are in private `secrets.yaml`.

## Provisional pinout: ESP32-C3 Super Mini

User confirmed ESP32-C3 Super Mini. Profile uses esp32-c3-devkitm-1 with ESP-IDF. GPIO numbers below are board labels, not physical pin positions. Avoids boot-strapping GPIO2/8/9 and USB GPIO18/19. Hardware communication remains unverified.

| ESP32 | Original JDY-18 side pad | Direction at ESP32 |
|---|---|---|
| GPIO4 | 1 TXD_DC / BMS receive | output |
| GPIO5 | 2 RXD_DD / BMS transmit | input |
| GPIO6 | 16 STAT | output |
| GPIO7 | 15 ALED | output |
| GPIO0 | 11 RESET | input only, monitors module-reset request |
| GPIO1 | 14 PWRC | input only |
| GND | 13 GND | common reference, subject to isolation verification |
| Independent adequate supply | NOT directly from module pad 12 | power ESP32 independently |

TX-to-TX here is intentional: the ESP replaces the MODULE, so its TX goes where the module TX went.
RESET from the BMS is monitored in software; do NOT wire it to ESP32 EN.
Pads 17,18,19,20,24,25 are not ESP connections in this prototype. Existing SELECT wiring must be measured, not modified blindly.

Before live connection: measure BMS pad supply and logic highs (1.8 V versus 3.3 V), verify reset polarity and PWRC behaviour, verify safe ground reference/isolation, and use level conversion where needed. Avoid tying together USB power outputs. The original module and ESP must not both drive the same BMS inputs. Capture the original interface if the initial handshake fails.
