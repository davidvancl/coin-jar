# coin-jar

Firmware for a coin jar with lights and sound, running on an ESP32. An infrared light barrier detects every coin dropped in, the RGB LEDs flash gold and two small speakers play a short chime. The firmware updates itself from GitHub Releases.

<img src="docs/20261009_184932.jpg" alt="Finished coin jar with red LEDs shining through the pattern" width="360"> <img src="docs/20261009_000435.jpg" alt="Opened top part with the ESP32 and wiring" width="360">

*The finished jar and the electronics in its top part.*

The 3D printed jar is on [Printables](https://www.printables.com/model/1869046-smart-coin-jar-with-sound-rgb-lights-esp32).

> [!TIP]
> **Control it from your phone.** The jar works with the Android app [esp-control-app](https://github.com/davidvancl/esp-control-app): coin count, LED light and colors, volume and music streamed from the phone. Download the APK from its [releases](https://github.com/davidvancl/esp-control-app/releases/latest), see [Android app](#android-app) below.

## Hardware

- ESP32 DevKit (`esp32dev`)
- 5x WS2811 5 mm RGB LED in a chain, 330 Ω resistor in the data line to the first LED
- IR light barrier: IR LED and a phototransistor read by the ADC (10 kΩ resistor on the phototransistor)
- 2x MAX98357 I2S amplifier with a small speaker each, one set to the left channel, one to the right

Wiring:

| Signal | GPIO |
|---|---|
| LED data (DIN of LED 1) | 33 |
| IR LED | 32 |
| Phototransistor (ADC) | 34 |
| I2S BCLK | 26 |
| I2S LRC (WS) | 25 |
| I2S DIN | 22 |

The LED pin and count are in [include/config.h](include/config.h). The light barrier and I2S pins are defaults in [include/CoinSensor.h](include/CoinSensor.h) and [include/Sound.h](include/Sound.h) and can be overridden with build flags.

The LEDs use the RGB color order (`NEO_RGB` in [src/Led.cpp](src/Led.cpp)). If red and green are swapped, change it to `NEO_GRB`.

## Behavior

On startup the LEDs pulse orange while the device connects to WiFi and checks for updates. Then the light barrier is calibrated and the jar waits for coins.

| Event | LEDs | Sound |
|---|---|---|
| WiFi connection and update check | pulsing orange | – |
| Light barrier does not work | 3 red blinks | – |
| Coin dropped in | gold (color can be changed in the app) while the chime plays | chime from both speakers (0.6 s) |
| Permanent light switched on in the app | each LED in its own color, brightness adjustable; effects above return to it when they end | – |
| Pairing with the Android app | a sequence of 4 colors blinking repeatedly | – |

A coin is detected when the phototransistor value drops below 60 % of the idle value. The barrier is armed again once the value is back above 80 %. The idle value slowly follows changes of ambient light. The sensor is sampled about 2000 times per second.

Each coin is also logged to the serial monitor with the coin count, the measured value and the idle value. The count is saved to NVS (`Preferences`, namespace `coinjar`) after every coin and loaded at startup, so it is kept across reboots and firmware updates.

## Light barrier calibration

At startup the firmware measures the phototransistor with the IR LED off and on and prints both values to the serial monitor. The difference should ideally be above 1000. Below 200 the barrier is reported as not working (3 red blinks) and a hint is printed:

| Hint | Meaning |
|---|---|
| almost zero | the phototransistor, or the IR LED, is probably reversed |
| saturated | too much light, replace the 10 kΩ resistor with 4.7 kΩ |
| small difference | check the polarity of the IR LED and the phototransistor |

## Sound

The chime is two mixed tones (1320 Hz and 1760 Hz) with a short fade-out, generated directly in the firmware. Volume is set in [src/main.cpp](src/main.cpp) with `Sound::setVolume()`, the range is 0 to 32767 and the default 2000 is quiet (1000 to 6000 is a sensible range). If the left and right speakers are swapped, set `SWAP_LR` in [src/Sound.cpp](src/Sound.cpp) to `true`.

### Music from the phone

The Android app can stream music to the jar: 16-bit mono PCM at 22050 Hz over WiFi. The jar buffers about 1 s of audio in a ring buffer and a separate FreeRTOS task plays it through I2S on both speakers, so the coin detection keeps running. The volume setting applies to the music as well. While the music plays, a dropped coin only flashes the LEDs; the chime is skipped. The stream stops when the app stops it, or by itself after 3 s without data.

## Android app

> [!NOTE]
> App: [github.com/davidvancl/esp-control-app](https://github.com/davidvancl/esp-control-app) – the signed APK is in its [latest release](https://github.com/davidvancl/esp-control-app/releases/latest).
> The phone and the jar must be on the same WiFi network.

The jar is controlled over the local WiFi network using the [esp-device-control](https://github.com/davidvancl/esp-device-control) library. The app finds it automatically.

<img src="docs/app-coins.jpg" alt="App: coin count and the light section" width="240"> <img src="docs/app-light.jpg" alt="App: permanent light, LED colors, brightness and flash color" width="240"> <img src="docs/app-sound.jpg" alt="App: volume, chime and music from the phone" width="240">

*Coin count, LED light and sound in the app.*

| Part | What the app can do | Stored in the jar |
|---|---|---|
| Coins | show the total count live, correct it, reset the counter | yes, in NVS – survives reboots, power loss and OTA updates |
| Light | switch on a permanent light, set the color of each of the 5 LEDs, brightness, color of the coin flash | no, back to defaults after a reboot |
| Sound | chime volume, play the chime | no |
| Music | stream a song from the phone to the speakers, also in the background | – |
| Diagnostics | light barrier state, current and idle value | – |

**Pairing:** start pairing in the app; the LEDs blink a sequence of 4 colors (red, green, blue, yellow, purple) over and over, tap the same colors in the same order in the app.

The exposed values are listed in the library's [DESIGN.md](https://github.com/davidvancl/esp-device-control/blob/main/DESIGN.md#coin-jar).

## WiFi

Copy [include/secrets.example.h](include/secrets.example.h) to `include/secrets.h` (it is in `.gitignore`) and fill it in. Up to 4 networks are supported, remove the ones you don't need:

```cpp
#define SECRET_SSID "wifi-name"
#define SECRET_PASS "wifi-password"

#define SECRET_SSID_2 "second-wifi-name"
#define SECRET_PASS_2 "second-wifi-password"
```

The networks visible in a scan are tried first, in this order. On the first upload over USB all networks are saved to EEPROM. Firmware built by GitHub Actions has no `secrets.h` and uses the saved networks. To change them, edit `secrets.h` and upload the firmware over USB again.

## Build and upload

Requires [PlatformIO](https://platformio.org/) (CLI or the VS Code extension).

```
pio run -t upload
pio device monitor
```

The firmware prints its version, the result of the update check and the light barrier calibration to the serial monitor.

## Source layout

- `src/main.cpp` – `setup()` and `loop()`, reaction to a coin, values exposed to the Android app
- `src/Led.cpp` – WS2811 LEDs: startup pulsing, pairing color sequence, permanent light and brightness
- `src/CoinSensor.cpp` – IR light barrier, calibration, coin detection and the stored coin count
- `src/Sound.cpp` – I2S output, the chime and the music stream from the app
- `include/config.h` – LED pin, count, brightness and pulsing
- `include/WifiCredentials.h` – uses `secrets.h` if it exists

## Firmware update (OTA)

Uses the [esp-ota-updater](https://github.com/davidvancl/esp-ota-updater) library. On startup the device checks the latest release and updates itself if a newer version exists.

Releasing a new version:
1. Increase `custom_version` in `platformio.ini`.
2. Commit and push to `main`.
3. GitHub Actions publish a release. The device updates after its next restart.

Do not push a locally changed version in `platformio.ini` unless you want to publish a release.

## License

[MIT](LICENSE)
