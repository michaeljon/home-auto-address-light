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
arduino-cli upload --protocol network -p <addr> -b esp32:esp32:esp32 --upload-field 'password=<pwd>'
```
