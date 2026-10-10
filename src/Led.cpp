#include "Led.h"
#include "config.h"

namespace Led {

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

static TaskHandle_t pulseTask = nullptr;
static volatile bool pulsing = false;
static uint8_t pulseR = 0, pulseG = 0, pulseB = 0;

static TaskHandle_t sequenceTask = nullptr;
static volatile bool sequencing = false;
static uint32_t sequence[SEQUENCE_MAX];
static int sequenceLength = 0;

static void fill(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
  strip.show();
}

static void pulseLoop(void*) {
  uint32_t start = millis();
  while (pulsing) {
    float phase = (float)((millis() - start) % PULSE_PERIOD_MS) / PULSE_PERIOD_MS;
    float level = 0.5f - 0.5f * cosf(phase * 2.0f * PI);
    uint8_t scale = (uint8_t)(level * 255.0f);
    fill(strip.gamma32(strip.Color(pulseR * scale / 255, pulseG * scale / 255, pulseB * scale / 255)));
    vTaskDelay(pdMS_TO_TICKS(PULSE_FRAME_MS));
  }
  pulseTask = nullptr;
  vTaskDelete(nullptr);
}

static bool holdSequence(uint32_t ms) {
  uint32_t start = millis();
  while (sequencing && millis() - start < ms) vTaskDelay(pdMS_TO_TICKS(PULSE_FRAME_MS));
  return sequencing;
}

static void sequenceLoop(void*) {
  while (sequencing) {
    for (int i = 0; i < sequenceLength; i++) {
      fill(sequence[i]);
      if (!holdSequence(SEQUENCE_ON_MS)) break;
      fill(0);
      if (!holdSequence(SEQUENCE_OFF_MS)) break;
    }
    holdSequence(SEQUENCE_PAUSE_MS);
  }
  sequenceTask = nullptr;
  vTaskDelete(nullptr);
}

void begin() {
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();
}

void startPulse(uint8_t r, uint8_t g, uint8_t b) {
  pulseR = r;
  pulseG = g;
  pulseB = b;
  if (pulseTask) return;
  pulsing = true;
  xTaskCreate(pulseLoop, "ledPulse", 2048, nullptr, 1, &pulseTask);
}

void stopPulse() {
  pulsing = false;
  while (pulseTask) delay(PULSE_FRAME_MS);
  strip.clear();
  strip.show();
}

void startSequence(const uint32_t* colors, int count) {
  if (sequenceTask) return;
  sequenceLength = min(count, SEQUENCE_MAX);
  for (int i = 0; i < sequenceLength; i++) sequence[i] = colors[i];
  sequencing = true;
  xTaskCreate(sequenceLoop, "ledSequence", 2048, nullptr, 1, &sequenceTask);
}

void stopSequence() {
  sequencing = false;
  while (sequenceTask) delay(PULSE_FRAME_MS);
  strip.clear();
  strip.show();
}

}
