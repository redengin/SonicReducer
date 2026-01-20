/// inspired by https://github.com/espressif/esp-idf/blob/v5.5.2/examples/bluetooth/bluedroid/classic_bt/a2dp_sink/

#include "bt_a2dp.h"

#include <nvs_flash.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include "esp_gap_bt_api.h"

// #include "bt_app_core.h"
// #include "bt_app_av.h"
// #include "esp_bt_main.h"
// #include "esp_bt_device.h"
// #include "esp_a2dp_api.h"
// #include "esp_avrc_api.h"

void bt_a2dp_init(void)
{
    // char bda_str[18] = {0};

    /* initialize NVS — it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // reclaim memory use for BLE
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    // create the bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    // extend controller functionality with bluedroid
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // support simple bonding (SPP)
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    ESP_ERROR_CHECK(
        esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(esp_bt_io_cap_t)));
    esp_bt_pin_code_t pin_code = {0, 0, 0, 0};
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 4, pin_code));
}
