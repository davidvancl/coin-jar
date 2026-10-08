#pragma once
#include <Arduino.h>

// Piny závory – lze přepsat v config.h
#ifndef PIN_IR_LED
#define PIN_IR_LED  32
#endif
#ifndef PIN_IR_SENS
#define PIN_IR_SENS 34
#endif

namespace CoinSensor {
  bool begin();          // zapne IR LED, změří hodnoty vyp/zap; false = závora nefunguje
  bool update();         // volej v loop() co nejčastěji; vrátí true při průchodu mince
  int  value();          // poslední naměřená hodnota (0–4095)
  int  baseline();       // klidová hodnota (paprsek nepřerušen)
  int  count();          // počet detekovaných mincí od startu
}
