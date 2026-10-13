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

static const uint32_t MUSIC_IDLE_MS = 3000;
static const uint32_t MUSIC_REPORT_MS = 500;
static const uint32_t FLASH_MS = 300;

static bool barrierOk = false;
static uint32_t lastMusicReport = 0;
static uint32_t flashColor = FLASH_COLOR;

void allColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_COUNT; i++) leds.setPixelColor(i, leds.Color(r, g, b));
  leds.show();
}

static void fillColor(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) leds.setPixelColor(i, color);
  leds.show();
}

static bool parseColor(JsonVariantConst value, uint32_t& out) {
  const char* text = value.as<const char*>();
  if (!text || strlen(text) != 7 || text[0] != '#') return false;
  for (int i = 1; i < 7; i++) {
    if (!isxdigit((unsigned char)text[i])) return false;
  }
  out = strtoul(text + 1, nullptr, 16);
  return true;
}

static String colorText(uint32_t color) {
  char text[8];
  snprintf(text, sizeof(text), "#%06X", (unsigned)(color & 0xFFFFFF));
  return String(text);
}

static bool setLightColors(JsonVariantConst value) {
  uint32_t colors[LED_COUNT];
  if (parseColor(value, colors[0])) {
    for (int i = 1; i < LED_COUNT; i++) colors[i] = colors[0];
  } else {
    JsonArrayConst list = value.as<JsonArrayConst>();
    if (list.isNull() || list.size() != LED_COUNT) return false;
    for (int i = 0; i < LED_COUNT; i++) {
      if (!parseColor(list[i], colors[i])) return false;
    }
  }
  for (int i = 0; i < LED_COUNT; i++) Led::setLightColor(i, colors[i]);
  return true;
}

static void celebrate() {
  fillColor(flashColor);
  if (Sound::streaming()) {
    delay(FLASH_MS);
  } else {
    Sound::chime(true, true);
  }
  Led::restore();
}

static void setMusic(bool on) {
  if (on) {
    Sound::startStream();
  } else {
    Sound::stopStream();
  }
}

static void maintainMusic() {
  if (!Sound::streaming()) return;
  if (Sound::idleMs() > MUSIC_IDLE_MS) {
    Sound::stopStream();
    DeviceControl::notify("music");
    return;
  }
  uint32_t now = millis();
  if (now - lastMusicReport < MUSIC_REPORT_MS) return;
  lastMusicReport = now;
  DeviceControl::notify("musicBuffer");
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
  DeviceControl::get<bool>("music", Sound::streaming);
  DeviceControl::set<bool>("music", setMusic);
  DeviceControl::get<int>("musicBuffer", Sound::bufferedMs);
  DeviceControl::onData(Sound::streamWrite);
  DeviceControl::get<bool>("light", Led::light);
  DeviceControl::set<bool>("light", Led::setLight);
  DeviceControl::get("lightColors", [](JsonVariant out) {
    JsonArray colors = out.to<JsonArray>();
    for (int i = 0; i < LED_COUNT; i++) colors.add(colorText(Led::lightColor(i)));
  });
  DeviceControl::set("lightColors", setLightColors);
  DeviceControl::get<int>("brightness", [] { return (int)Led::brightness(); });
  DeviceControl::set("brightness", number(1, 255, [](int v) { Led::setBrightness(v); }));
  DeviceControl::get("flashColor", [](JsonVariant out) { out.set(colorText(flashColor)); });
  DeviceControl::set("flashColor", [](JsonVariantConst value) { return parseColor(value, flashColor); });
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
  maintainMusic();

  if (CoinSensor::update()) {
    Serial.printf(">>> MINCE! (#%d)  hodnota %d / klid %d\n",
                  CoinSensor::count(), CoinSensor::value(), CoinSensor::baseline());
    DeviceControl::notify("coins");
    celebrate();
  }
}
