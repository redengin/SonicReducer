#include "modulator.h"

#include <soc/io_mux_reg.h>
#include <driver/i2s.h>

const size_t CARRIER_HZ = 40 * 1000;    // 40 KHz

void modulator_init(void)
{
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 0);
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT));
    // Set maximum drive capability for GPIO0 to increase transmission power
    // NOT USED gpio_set_drive_capability(GPIO_NUM_0, GPIO_DRIVE_CAP_3);
}

void modulator_config(const modulator_config_t* const config)
{
    const i2s_config_t cfg = {
        .mode                 = I2S_MODE_MASTER | I2S_MODE_TX,
        // .sample_rate          = WAV_SR_HZ,
        .sample_rate          = config->pcm_sample_rate_hz,
        // .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .bits_per_sample      = config->bits_per_sample,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT,
        // NOT USED .use_apll             = true,
        .fixed_mclk           = CARRIER_HZ,
        .dma_buf_count        = 4,
        .dma_buf_len          = 64,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL));
    // NOT USED ESP_ERROR_CHECK(i2s_start(I2S_NUM_0));
}

void modulator_start() {
    ESP_ERROR_CHECK(i2s_start(I2S_NUM_0));
}

void modulator_stop() {
    ESP_ERROR_CHECK(i2s_stop(I2S_NUM_0));
}
