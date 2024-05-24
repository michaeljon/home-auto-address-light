# Address sign notes

## EEPROM / Flash requirements

WiFi access

- 32 characters + 1 for SSID (NULL-padded)
- 32 characters + 1 for password (NULL-padded)

The first byte (timing option) drives the remainder of the data. Think of this as a classic dynamic C struct.

- 1 byte light timing option (0 - auto, 1 - manual, 2 - always on)

If the light is set to manual then (see above for format)

- 4 bytes for "on" time
- 4 bytes for "off" time

If the light is set to auto then

- 2928 bytes for computed on / off times (2x 4 byte words per day, stores `HH * 3600 + MM * 60 + S`)
- 2 bytes (1 short) for day of year that table was last computed (1 - 366, 0 means not computed)
- 2 bytes timezone offset (±1440 minutes)
- sizeof(double) for latitude
- sizeof(double) for longitude
