#pragma once
#include <Arduino.h>

// Piny I2S pro 2x MAX98357 – lze přepsat v config.h
#ifndef PIN_I2S_BCLK
#define PIN_I2S_BCLK 26
#endif
#ifndef PIN_I2S_LRC
#define PIN_I2S_LRC  25
#endif
#ifndef PIN_I2S_DIN
#define PIN_I2S_DIN  22
#endif

namespace Sound {
  void begin();
  void setVolume(uint16_t volume);           // 0–32767, výchozí 2000 (tiše)
  void chime(bool left = true, bool right = true);  // krátké jemné "cink"
}
