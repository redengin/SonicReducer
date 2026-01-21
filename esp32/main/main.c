#include <stdbool.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <esp_bt.h>

#include "bt_a2dp.h"
#include "modulator.h"

const char LOG_TAG[] = "SonicReducer";

void app_main(void)
{
    // initialize NVS — it is used to store PHY calibration data
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGV(LOG_TAG, "initialized nvs flash");

    // reclaim memory use for BLE
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // initialize bluetooth audio receiver
    // FIXME use esp32 autoconf to configure device name and pin code
    const char device_name[] = "SonicReducer";
    const pin_code_t pin_code = {0, 0, 0, 0};
    bt_a2dp_init(device_name, pin_code);

    // initialize the modulator
    modulator_init();

    // TEST USE ONLY
    // const modulator_config_t mod_config = {
    //     .pcm_sample_rate_hz = 8000,
    //     .bits_per_sample = 8,
    // };
    // modulator_config(&mod_config);
    // modulator_start();
    // modulator_stop();

    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}
