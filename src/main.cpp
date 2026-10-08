// Tichý test zvuku – 2x MAX98357 (levý + pravý kanál) – kasička
// I2S: BCLK = GPIO26, LRC = GPIO25, DIN = GPIO22
// Krátké jemné "cink" vlevo, vpravo a v obou, pak 5 s ticho.
#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

#define PIN_BCLK 26
#define PIN_LRC  25
#define PIN_DIN  22

#define SAMPLE_RATE 22050
#define VOLUME      2000      // tiše (max 32767). Klidně zkus 1000–6000.
#define SWAP_LR     false     // když hraje opačný reproduktor, dej true

const i2s_port_t I2S_PORT = I2S_NUM_0;

void setupI2S() {
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
  pins.bck_io_num = PIN_BCLK;
  pins.ws_io_num = PIN_LRC;
  pins.data_out_num = PIN_DIN;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

// Jemné "cink": dva tóny (1320 + 1760 Hz) s rychlým náběhem a pomalým dozvukem
void chime(bool left, bool right) {
  if (SWAP_LR) { bool t = left; left = right; right = t; }
  const int frames = SAMPLE_RATE * 0.6;          // 0,6 s
  int16_t buf[2 * 128];
  int done = 0;
  while (done < frames) {
    int n = min(128, frames - done);
    for (int i = 0; i < n; i++) {
      float t = (done + i) / (float)SAMPLE_RATE;
      float env = (t < 0.005f ? t / 0.005f : 1.0f) * expf(-t * 7.0f);
      float s = 0.6f * sinf(2 * PI * 1320 * t) + 0.4f * sinf(2 * PI * 1760 * t);
      int16_t v = (int16_t)(s * env * VOLUME);
      buf[2 * i]     = right ? v : 0;   // ESP32 posílá v páru nejdřív pravý
      buf[2 * i + 1] = left  ? v : 0;
    }
    size_t w;
    i2s_write(I2S_PORT, buf, n * 2 * sizeof(int16_t), &w, portMAX_DELAY);
    done += n;
  }
  i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  setupI2S();
  Serial.println("\n=== Tichy test zvuku ===");
}

void loop() {
  Serial.println("cink: LEVY");   chime(true,  false); delay(800);
  Serial.println("cink: PRAVY");  chime(false, true);  delay(800);
  Serial.println("cink: OBA");    chime(true,  true);
  Serial.println("(5 s ticho)");  delay(5000);
}
