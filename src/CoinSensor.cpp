#include "CoinSensor.h"

namespace {
  const float DROP_RATIO  = 0.60f;   // mince = pokles pod 60 % klidové hodnoty
  const float REARM_RATIO = 0.80f;   // znovu připraveno nad 80 %
  const uint32_t SAMPLE_US = 500;    // ~2000 měření za sekundu

  float    base    = 0;
  int      lastVal = 0;
  int      coins   = 0;
  bool     blocked = false;
  uint32_t lastSample = 0;

  int readAvg(int n) {
    long s = 0;
    for (int i = 0; i < n; i++) s += analogRead(PIN_IR_SENS);
    return s / n;
  }
}

namespace CoinSensor {

bool begin() {
  pinMode(PIN_IR_LED, OUTPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_IR_SENS, ADC_11db);

  digitalWrite(PIN_IR_LED, LOW);  delay(50);
  int off = readAvg(64);
  digitalWrite(PIN_IR_LED, HIGH); delay(50);
  int on = readAvg(64);

  Serial.println("--- Zavora ---");
  Serial.printf("IR LED vypnuta: %4d\n", off);
  Serial.printf("IR LED zapnuta: %4d\n", on);
  Serial.printf("Rozdil:         %4d  (idealne > 1000)\n", on - off);
  if (on - off < 200) {
    if (on < 300)        Serial.println("! Skoro nula – otoc fototranzistor, pak pripadne IR LED");
    else if (off > 3800) Serial.println("! Presyceno – zmensi 10k na 4k7");
    else                 Serial.println("! Maly rozdil – zkontroluj polaritu IR LED a fototranzistoru");
  }
  base = lastVal = on;
  return (on - off) >= 200;
}

bool update() {
  uint32_t now = micros();
  if (now - lastSample < SAMPLE_US) return false;
  lastSample = now;

  int v = analogRead(PIN_IR_SENS);
  lastVal = v;
  bool coin = false;

  if (!blocked && v < base * DROP_RATIO) {
    blocked = true;
    coins++;
    coin = true;
  } else if (blocked && v > base * REARM_RATIO) {
    blocked = false;
  }
  if (!blocked) base = base * 0.999f + v * 0.001f;   // pomalé dorovnání klidu
  return coin;
}

int value()    { return lastVal; }
int baseline() { return (int)base; }
int count()    { return coins; }

} // namespace CoinSensor
