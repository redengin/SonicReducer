#include "bt_a2dp.h"

#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>

static const char *LOG_TAG = "bt-a2dp";

void bt_a2dp_init(
    const char *const device_name, ///< bluetooth published name
    const pin_code_t pin_code      ///< pairing pin code (or NULL if SSP not used)
)
{
    // initialize the bluetooth hardware
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    // initialize bluetooth stack (bluedroid)
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    if (!pin_code)
        bluedroid_cfg.ssp_en = false;
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    if (pin_code)
    {
        // configure the pin code (SPP)
        esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
        esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
        ESP_ERROR_CHECK(esp_bt_gap_set_security_param(
            param_type, &iocap, sizeof(uint8_t)));
        esp_bt_pin_code_t _pin_code;
        memcpy(_pin_code, pin_code, SIZEOF_PIN_CODE);
        ESP_ERROR_CHECK(esp_bt_gap_set_pin(
            ESP_BT_PIN_TYPE_FIXED, SIZEOF_PIN_CODE, _pin_code));
    }

//     esp_bt_gap_set_device_name(local_device_name);
//     esp_bt_dev_register_callback(bt_app_dev_cb);
//     esp_bt_gap_register_callback(bt_app_gap_cb);

//     esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
//     assert(esp_avrc_ct_init() == ESP_OK);
//     esp_avrc_tg_register_callback(bt_app_rc_tg_cb);
//     assert(esp_avrc_tg_init() == ESP_OK);

//     esp_avrc_rn_evt_cap_mask_t evt_set = {0};
//     esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
//     assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

//     esp_a2d_register_callback(&bt_app_a2d_cb);
//     assert(esp_a2d_sink_init() == ESP_OK);

// #if CONFIG_EXAMPLE_A2DP_SINK_USE_EXTERNAL_CODEC == FALSE
//     esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
// #else
//     esp_a2d_mcc_t mcc = {0};
//     mcc.type = ESP_A2D_MCT_SBC;
//     mcc.cie.sbc_info.samp_freq = 0xf;
//     mcc.cie.sbc_info.ch_mode = 0xf;
//     mcc.cie.sbc_info.block_len = 0xf;
//     mcc.cie.sbc_info.num_subbands = 0x3;
//     mcc.cie.sbc_info.alloc_mthd = 0x3;
//     mcc.cie.sbc_info.max_bitpool = 250;
//     mcc.cie.sbc_info.min_bitpool = 2;
//     /* register stream end point, only support mSBC currently */
//     esp_a2d_sink_register_stream_endpoint(0, &mcc);
//     esp_a2d_sink_register_audio_data_callback(bt_app_a2d_audio_data_cb);
// #endif
//     /* Get the default value of the delay value */
//     esp_a2d_sink_get_delay_value();
//     /* Get local device name */
//     esp_bt_gap_get_device_name();

//     /* set discoverable and connectable mode, wait to be connected */
//     esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

}
