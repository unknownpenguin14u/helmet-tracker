#include <driver/i2s.h>

#define I2S_WS 15
#define I2S_SD 33
#define I2S_SCK 14

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000
#define BUFFER_LEN 256

int32_t i2sBuffer[BUFFER_LEN];

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("INMP441 FIX TEST START");

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);

  Serial.println("Mic Ready...");
}

void loop() {

  size_t bytesRead = 0;
  i2s_read(I2S_PORT, &i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);

  if (bytesRead > 0) {

    int samples = bytesRead / 4;
    long total = 0;

    for (int i = 0; i < samples; i++) {
      int32_t sample = i2sBuffer[i];

      // Proper 24-bit extraction
      sample = sample >> 14;

      total += abs(sample);
    }

    long average = total / samples;

    Serial.println(average);
  }
}
