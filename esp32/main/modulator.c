// inspired by https://github.com/bitluni/ESP32AMRadioTransmitter

#include "modulator.h"

#include <driver/i2s.h>
// #include <driver/i2s_std.h>
#include <soc/i2s_reg.h>

// use I2S0 SoC peripheral
#define I2S I2S_NUM_0

#define I2S_BUFFER_LEN (1024)
static uint8_t I2S_BUFFER[I2S_BUFFER_LEN];

void modulator_init(void)
{
    // configure I2S driver
    static const i2s_config_t i2s_config = {
        // .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 1000000, // not really used
        .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = I2S_BUFFER_LEN // big buffers to avoid noise
    };
    ESP_ERROR_CHECK(
        i2s_driver_install(
            I2S, &i2s_config,
            0 /* queue size*/,
            NULL /* *queue */
            ));

    // configure pins
    const i2s_pin_config_t pin_config = {
        .mck_io_num = GPIO_NUM_NC,
        .bck_io_num = GPIO_NUM_NC,
        .ws_io_num = GPIO_NUM_NC,
        .data_out_num = CONFIG_SONIC_REDUCER_OUTPUT_PIN,
        .data_in_num = GPIO_NUM_NC,
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S, &pin_config));

    // configure the sample rate
    // dummy sample rate, since the function fails at high values
    ESP_ERROR_CHECK(i2s_set_sample_rates(I2S, 1000000 /*dummy value*/));
    // this is the hack that enables the highest sampling rate possible ~13MHz, have fun
    // https://documentation.espressif.com/esp32_technical_reference_manual_en.pdf#i2s
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(I2S), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
    SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(I2S), I2S_CLKM_DIV_NUM_V, 2, I2S_CLKM_DIV_NUM_S);
    SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(I2S), I2S_TX_BCK_DIV_NUM_V, 2, I2S_TX_BCK_DIV_NUM_S);
}

void modulator_config(const modulator_config_t *const config)
{
}

void modulator_start(void)
{
    /// FIXME implement
}

void modulator_write(const uint8_t* const data, const size_t sz)
{
    // FIXME implement
}


void modulator_stop(void)
{
    /// FIXME implement
}
