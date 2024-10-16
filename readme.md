## Prep

```bash
arduino-cli update
arduino-cli upgrade
arduino-cli core update-index
arduino-cli core install arduino:avr

arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

## Build

```bash
# install the libraries
cd Documents/Arduino/libraries
git clone https://github.com/Erriez/ErriezDS3231.git

arduino-cli lib install InputDebounce
arduino-cli lib install PsychicHttp
arduino-cli lib install ArduinoJson

# compile
arduino-cli compile -b esp32:esp32:esp32 --log --output-dir ./output

# upload (use Discovery to find address for _arduino._tcp - ALIGHT_xxx)
arduino-cli upload --protocol network --port <addr> --fqbn esp32:esp32:esp32 --verbose --upload-field 'password=<pwd>'
```
