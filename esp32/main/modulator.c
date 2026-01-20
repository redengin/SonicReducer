#include "modulator.h"

#include <soc/io_mux_reg.h>
#include <driver/gpio.h>

const size_t CARRIER_HZ = 40 * 1000;    // 40 KHz

void modulator_init(void)
{
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 0);
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT));
    // Set maximum drive capability for GPIO0 to increase transmission power
    // NOT USED gpio_set_drive_capability(GPIO_NUM_0, GPIO_DRIVE_CAP_3);
}

#define USE_I2S_LEGACY
#ifdef USE_I2S_LEGACY
#include <driver/i2s.h>
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
#else
#include <driver/i2s_pdm.h>

static i2s_chan_handle_t modulator_chan_handle;
void modulator_config(const modulator_config_t* const config)
{
    // create an I2S channel
    const i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &modulator_chan_handle, NULL));

    // configure the channel for trasmit
    const i2s_pdm_tx_config_t pdm_tx_cfg = {
        .clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(CARRIER_HZ),
        /* The data bit-width of PDM mode is fixed to 16 */
        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    };
    ESP_ERROR_CHECK(i2s_channel_init_pdm_tx_mode(modulator_chan_handle, &pdm_tx_cfg));

    // enable the tx channel
    ESP_ERROR_CHECK(i2s_channel_enable(modulator_chan_handle));
}

#endif // USE_I2S_LEGACY