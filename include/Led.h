#pragma once

#include <Adafruit_NeoPixel.h>

namespace Led {

extern Adafruit_NeoPixel strip;

void begin();
void startPulse(uint8_t r, uint8_t g, uint8_t b);
void stopPulse();
void startSequence(const uint32_t* colors, int count);
void stopSequence();
void setLight(bool on);
bool light();
void setLightColor(int index, uint32_t color);
uint32_t lightColor(int index);
void setBrightness(uint8_t level);
uint8_t brightness();
void restore();

}
