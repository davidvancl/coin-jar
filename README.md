# coin-jar

ESP32 firmware for the coin jar with 5 WS2811 RGB LEDs (data on GPIO33).

## Setup

1. Copy `include/secrets.example.h` to `include/secrets.h` and fill in the WiFi credentials (up to 4 networks, the extra ones are optional).
2. Flash over USB once: `pio run -t upload`.

## OTA updates

On startup the device connects to WiFi and checks GitHub Releases of `davidvancl/coin-jar` for a newer version ([esp-ota-updater](https://github.com/davidvancl/esp-ota-updater)). The LEDs pulse orange while the check is running.

To release a new version, bump `custom_version` in `platformio.ini` and push to `main`. The device installs it on the next restart.

WiFi credentials are stored in EEPROM after the first USB flash, so CI builds (without `secrets.h`) keep working.
