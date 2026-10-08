#include <Arduino.h>
#include <OtaUpdater.h>
#include "config.h"
#include "Led.h"
#include "Sound.h"
#include "CoinSensor.h"
#include "WifiCredentials.h"

Adafruit_NeoPixel& leds = Led::strip;

void allColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < LED_COUNT; i++) leds.setPixelColor(i, leds.Color(r, g, b));
  leds.show();
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
  Sound::setVolume(2000);   // tiše; klidně zkus 1000–6000

  // Když závora nefunguje (malý rozdíl IR vyp/zap), 3x bliknou LEDky červeně
  if (!CoinSensor::begin()) {
    for (int i = 0; i < 3; i++) {
      allColor(80, 0, 0); delay(300);
      leds.clear(); leds.show(); delay(300);
    }
  }
}

void loop() {
  if (CoinSensor::update()) {
    Serial.printf(">>> MINCE! (#%d)  hodnota %d / klid %d\n",
                  CoinSensor::count(), CoinSensor::value(), CoinSensor::baseline());
    allColor(255, 160, 0);          // zlatý záblesk po dobu zvuku
    Sound::chime(true, true);
    leds.clear(); leds.show();
  }
}