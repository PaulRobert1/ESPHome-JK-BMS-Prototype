# Publication validation

On 2026-10-27, `esphome config jkbmsprototype.yaml` passed with ESPHome 2026.9.1 and temporary generated credentials. The local dependencies were present. Temporary secrets were removed after validation.

Public source was checked for known source credentials and private network identifiers. Firmware compilation, flashing and physical hardware tests were not performed for this publication. Existing calibration/behavior is retained except for the publication changes listed in README.md.

## Existing configuration warnings

- 'total_battery_capacity_setting' is deprecated, use 'full_charge_capacity' instead. Will be removed in a future release.
- 'power_tube_temperature' is deprecated, use 'mosfet_temperature' instead. Will be removed in a future release.
- WiFi AP is configured but neither captive_portal nor web_server is enabled. The AP will not be usable for configuration or monitoring. Add 'captive_portal:' or 'web_server:' to your configuration.
- 'ota' password wastes significant flash and RAM (about 3.5 KB and 60 bytes plus the password on the heap): the device already offers encryption with the 'api' encryption key, which authenticates any uploader that takes it, and a password only matters for uploaders without encryption support; remove 'password' and add 'encryption' under 'ota' so uploads use the key and encryption is required
