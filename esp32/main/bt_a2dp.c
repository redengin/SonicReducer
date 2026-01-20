#include "bt_a2dp.h"

#include <nvs_flash.h>
#include <esp_bt.h>


void bt_a2dp_init(void)
{
    /* initialize NVS — it is used to store PHY calibration data */
    char bda_str[18] = {0};
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // reclaim memory use for BLE
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
}
