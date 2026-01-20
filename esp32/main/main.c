#include <stdbool.h>
#include <freertos/FreeRTOS.h>

#include "bt_a2dp.h"
#include "modulator.h"

void app_main(void)
{
    // initialize bluetooth audio receiver
    bt_a2dp_init();

    // initialize the modulator
    modulator_init();

    // TEST USE ONLY
    const modulator_config_t mod_config = {
        .pcm_sample_rate_hz = 8000,
        .bits_per_sample = 8,
    };
    modulator_config(&mod_config);
    modulator_start();
    modulator_stop();


    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}