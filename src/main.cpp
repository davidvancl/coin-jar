#include <Arduino.h>
#include <DeviceControl.h>
#include <OtaUpdater.h>
#include "config.h"
#include "Led.h"
#include "Sound.h"
#include "CoinSensor.h"
#include "WifiCredentials.h"

Adafruit_NeoPixel& leds = Led::strip;

static const char* const PAIR_COLORS[] = {"#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF"};

static bool barrierOk = false;

void allColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_COUNT; i++) leds.setPixelColor(i, leds.Color(r, g, b));
  leds.show();
}

static void celebrate() {
  allColor(255, 160, 0);
  Sound::chime(true, true);
  leds.clear(); leds.show();
}

static DeviceControl::Setter number(int min, int max, std::function<void(int)> apply) {
  return [min, max, apply](JsonVariantConst value) {
    if (!value.is<int>()) return false;
    int n = value.as<int>();
    if (n < min || n > max) return false;
    apply(n);
    return true;
  };
}

static void showPairing(const char* pin) {
  uint32_t colors[SEQUENCE_MAX];
  int count = 0;
  for (; pin[count] && count < SEQUENCE_MAX; count++) {
    colors[count] = strtoul(PAIR_COLORS[pin[count] - '0'] + 1, nullptr, 16);
  }
  Led::startSequence(colors, count);
}

static void setupControl() {
  DeviceControl::begin("Coin Jar", "coin-jar");

  DeviceControl::get<int>("coins", CoinSensor::count);
  DeviceControl::set("coins", number(0, 1000000, CoinSensor::setCount));
  DeviceControl::get<int>("volume", [] { return (int)Sound::getVolume(); });
  DeviceControl::set("volume", number(0, 32767, [](int v) { Sound::setVolume(v); }));
  DeviceControl::set("chime", [](JsonVariantConst) {
    celebrate();
    return true;
  });
  DeviceControl::get<bool>("barrier", [] { return barrierOk; });
  DeviceControl::get<int>("sensor", CoinSensor::value);
  DeviceControl::get<int>("baseline", CoinSensor::baseline);

  DeviceControl::pairWithPin(showPairing, Led::stopSequence,
                             {PAIR_COLORS[0], PAIR_COLORS[1], PAIR_COLORS[2], PAIR_COLORS[3], PAIR_COLORS[4]});
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();

  Led::begin();
  Led::startPulse(BOOT_R, BOOT_G, BOOT_B);

  Serial.print("Firmware version: ");
  Serial.println(FW_VERSION);
  OtaUpdater::run(WIFI_CREDENTIALS);

  Led::stopPulse();
  leds.clear(); leds.show();

  Sound::begin();
  Sound::setVolume(2000);

  barrierOk = CoinSensor::begin();
  if (!barrierOk) {
    for (int i = 0; i < 3; i++) {
      allColor(80, 0, 0); delay(300);
      leds.clear(); leds.show(); delay(300);
    }
  }

  setupControl();
}

void loop() {
  DeviceControl::loop();

  if (CoinSensor::update()) {
    Serial.printf(">>> MINCE! (#%d)  hodnota %d / klid %d\n",
                  CoinSensor::count(), CoinSensor::value(), CoinSensor::baseline());
    DeviceControl::notify("coins");
    celebrate();
  }
}
