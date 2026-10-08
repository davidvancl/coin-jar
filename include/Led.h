#pragma once

#include <Adafruit_NeoPixel.h>

namespace Led {

extern Adafruit_NeoPixel strip;

void begin();
void startPulse(uint8_t r, uint8_t g, uint8_t b);
void stopPulse();

}
