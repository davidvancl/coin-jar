#include <Arduino.h>
#include <OtaUpdater.h>
#include "config.h"
#include "Led.h"
#include "Sound.h"
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

  Sound::begin();
  Sound::setVolume(2000);   // tiše; klidně zkus 1000–6000

  Serial.println("\n=== Test LED + zvuk ===");
}

void loop() {
  // --- LED ---
  Serial.println("Test 1: LED 1 az 5 postupne (bila)");
  for (int i = 0; i < LED_COUNT; i++) {
    leds.clear();
    leds.setPixelColor(i, leds.Color(255, 255, 255));
    leds.show();
    Serial.printf("  svitit ma LED %d\n", i + 1);
    delay(700);
  }

  Serial.println("Test 2: barvy");
  Serial.println("  CERVENA");  allColor(255, 0, 0);   delay(1500);
  Serial.println("  ZELENA");   allColor(0, 255, 0);   delay(1500);
  Serial.println("  MODRA");    allColor(0, 0, 255);   delay(1500);

  Serial.println("Test 3: duha");
  for (long hue = 0; hue < 65536L * 2; hue += 512) {
    for (int i = 0; i < LED_COUNT; i++)
      leds.setPixelColor(i, leds.gamma32(leds.ColorHSV(hue + i * 65536L / LED_COUNT)));
    leds.show();
    delay(10);
  }
  leds.clear();
  leds.show();

  // --- Zvuk ---
  Serial.println("Test 4: zvuk");
  Serial.println("  cink: LEVY");   allColor(40, 0, 0);  Sound::chime(true,  false); delay(800);
  Serial.println("  cink: PRAVY");  allColor(0, 0, 40);  Sound::chime(false, true);  delay(800);
  Serial.println("  cink: OBA");    allColor(0, 40, 0);  Sound::chime(true,  true);

  leds.clear();
  leds.show();
  delay(3000);
}