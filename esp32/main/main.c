#include <stdbool.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include <esp_bt.h>

#include "bt_a2dp.h"

static const char *LOG_TAG = "SonicReducer";

/// retrieve the pin code from the sdkconfig
static esp_err_t pin_code_from_sdkconfig(
    pin_code_t*   ///< [OUT]
);

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
    pin_code_t pin_code;
    ESP_ERROR_CHECK(pin_code_from_sdkconfig(&pin_code));
    bt_a2dp_init(CONFIG_SONIC_REDUCER_DEVICE_NAME, pin_code);

    ESP_LOGI(LOG_TAG, "Initialized bluetooth speaker [%s] with pin [%s]",
        CONFIG_SONIC_REDUCER_DEVICE_NAME,
        CONFIG_SONIC_REDUCER_PIN_CODE
    );

    while (true)
        vTaskDelay(pdMS_TO_TICKS(1000));
}

static esp_err_t pin_code_from_sdkconfig(pin_code_t *pin_code)
{
    const size_t pin_code_str_len = strlen(CONFIG_SONIC_REDUCER_PIN_CODE);
    if (pin_code_str_len != PIN_CODE_LENGTH)
    {
        ESP_LOGE(LOG_TAG, "invalid pin code length (%d required, found %d) [%s]",
            PIN_CODE_LENGTH,
            pin_code_str_len,
            CONFIG_SONIC_REDUCER_PIN_CODE);
        return ESP_FAIL;
    }

    // translate the string into integers
    for (int i = 0; i < PIN_CODE_LENGTH; ++i)
    {
        // const uint8_t digit = pin_code_str[i] - '0';
        const uint8_t digit = CONFIG_SONIC_REDUCER_PIN_CODE[i] - '0';
        assert(digit <= 9);
        if (digit > 9)
        {
            ESP_LOGE(LOG_TAG, "invalid pin code [%s]", CONFIG_SONIC_REDUCER_PIN_CODE);
            return ESP_FAIL;
        }
        *pin_code[i] = digit;
    }

    return ESP_OK;
}
