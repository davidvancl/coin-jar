#include "Sound.h"
#include <driver/i2s.h>
#include <freertos/ringbuf.h>
#include <math.h>

namespace {
  const i2s_port_t I2S_PORT   = I2S_NUM_0;
  const int        SAMPLE_RATE = 22050;
  const bool       SWAP_LR     = false;   // když hraje opačný reproduktor, dej true
  uint16_t         volume      = 2000;

  const size_t   STREAM_BUFFER_BYTES    = 48000;
  const size_t   STREAM_PREBUFFER_BYTES = 8820;
  const size_t   STREAM_CHUNK_BYTES     = 512;
  const uint32_t STREAM_BYTES_PER_S     = SAMPLE_RATE * 2;

  RingbufHandle_t   ring          = nullptr;
  TaskHandle_t      streamTask    = nullptr;
  volatile bool     streamOn      = false;
  volatile uint32_t lastDataAt    = 0;
  uint8_t           receivedParity = 0;
  uint8_t           queuedParity   = 0;

  size_t bufferedBytes() {
    if (!ring) return 0;
    size_t free = xRingbufferGetCurFreeSize(ring);
    return free < STREAM_BUFFER_BYTES ? STREAM_BUFFER_BYTES - free : 0;
  }

  void drain() {
    if (!ring) return;
    size_t got;
    void* item;
    while ((item = xRingbufferReceiveUpTo(ring, &got, 0, STREAM_BUFFER_BYTES))) vRingbufferReturnItem(ring, item);
  }

  void streamLoop(void*) {
    int16_t out[2 * (STREAM_CHUNK_BYTES / 2 + 1)];
    uint8_t carry = 0;
    bool hasCarry = false;
    bool playing = false;

    while (streamOn) {
      if (!playing) {
        if (bufferedBytes() < STREAM_PREBUFFER_BYTES) {
          vTaskDelay(pdMS_TO_TICKS(10));
          continue;
        }
        playing = true;
      }

      size_t got = 0;
      uint8_t* data = (uint8_t*)xRingbufferReceiveUpTo(ring, &got, pdMS_TO_TICKS(50), STREAM_CHUNK_BYTES);
      if (!data) {
        playing = false;
        continue;
      }

      int32_t gain = volume;
      int frames = 0;
      auto put = [&](int16_t sample) {
        int16_t v = (int16_t)((int32_t)sample * gain / 32767);
        out[2 * frames] = v;
        out[2 * frames + 1] = v;
        frames++;
      };

      size_t i = 0;
      if (hasCarry) {
        put((int16_t)(carry | (data[0] << 8)));
        hasCarry = false;
        i = 1;
      }
      for (; i + 1 < got; i += 2) put((int16_t)(data[i] | (data[i + 1] << 8)));
      if (i < got) {
        carry = data[i];
        hasCarry = true;
      }
      vRingbufferReturnItem(ring, data);

      size_t written;
      i2s_write(I2S_PORT, out, frames * 2 * sizeof(int16_t), &written, portMAX_DELAY);
    }

    i2s_zero_dma_buffer(I2S_PORT);
    streamTask = nullptr;
    vTaskDelete(nullptr);
  }
}

namespace Sound {

void begin() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SAMPLE_RATE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.tx_desc_auto_clear = true;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = PIN_I2S_BCLK;
  pins.ws_io_num = PIN_I2S_LRC;
  pins.data_out_num = PIN_I2S_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

void setVolume(uint16_t v) { volume = min<uint16_t>(v, 32767); }

uint16_t getVolume() { return volume; }

void chime(bool left, bool right) {
  if (streamOn) return;
  if (SWAP_LR) { bool t = left; left = right; right = t; }
  const int frames = SAMPLE_RATE * 0.6f;   // 0,6 s
  int16_t buf[2 * 128];
  int done = 0;
  while (done < frames) {
    int n = min(128, frames - done);
    for (int i = 0; i < n; i++) {
      float t = (done + i) / (float)SAMPLE_RATE;
      float env = (t < 0.005f ? t / 0.005f : 1.0f) * expf(-t * 7.0f);
      float s = 0.6f * sinf(2 * PI * 1320 * t) + 0.4f * sinf(2 * PI * 1760 * t);
      int16_t v = (int16_t)(s * env * volume);
      buf[2 * i]     = right ? v : 0;   // ESP32 posílá v páru nejdřív pravý
      buf[2 * i + 1] = left  ? v : 0;
    }
    size_t w;
    i2s_write(I2S_PORT, buf, n * 2 * sizeof(int16_t), &w, portMAX_DELAY);
    done += n;
  }
  i2s_zero_dma_buffer(I2S_PORT);
}

bool startStream() {
  stopStream();
  if (!ring) ring = xRingbufferCreate(STREAM_BUFFER_BYTES, RINGBUF_TYPE_BYTEBUF);
  if (!ring) return false;
  drain();
  receivedParity = 0;
  queuedParity = 0;
  lastDataAt = millis();
  streamOn = true;
  if (xTaskCreatePinnedToCore(streamLoop, "audioStream", 4096, nullptr, 5, &streamTask, 1) != pdPASS) {
    streamOn = false;
    streamTask = nullptr;
    return false;
  }
  return true;
}

void stopStream() {
  streamOn = false;
  while (streamTask) delay(5);
  drain();
}

bool streaming() { return streamOn; }

void streamWrite(const uint8_t* data, size_t length) {
  if (!streamOn || !ring || !length) return;
  lastDataAt = millis();
  if (receivedParity != queuedParity) {
    receivedParity ^= 1;
    data++;
    length--;
    if (!length) return;
  }
  receivedParity ^= length & 1;
  if (xRingbufferSend(ring, data, length, 0) == pdTRUE) queuedParity ^= length & 1;
}

int bufferedMs() { return bufferedBytes() * 1000 / STREAM_BYTES_PER_S; }

uint32_t idleMs() { return millis() - lastDataAt; }

} // namespace Sound
