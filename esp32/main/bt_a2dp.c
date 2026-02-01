#include "bt_a2dp.h"
#include "bt_a2dp_worker.h"

#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_avrc_api.h>
#include <esp_a2dp_api.h>

static const char *LOG_TAG = "bt-a2dp";

// forward declarations of bluetooth event callbacks
static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
static void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);

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
    // disable SSP (use a 4 digit pin code)
    bluedroid_cfg.ssp_en = false;
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // configure the pin code
    esp_bt_pin_code_t _pin_code;
    memcpy(_pin_code, pin_code, sizeof(pin_code_t));
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(
        ESP_BT_PIN_TYPE_FIXED, sizeof(pin_code_t), _pin_code));

    // set the published bluetooth nane
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(device_name));

    // initialize the a2dp worker
    bt_a2dp_worker_init();

    // register GAP connection callbacks
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_app_gap_cb));
    // register for GAP client name
    ESP_ERROR_CHECK(esp_bt_gap_get_device_name());

    // intialize AVRC and register callbacks
    // ESP_ERROR_CHECK(esp_avrc_ct_init());
    // ESP_ERROR_CHECK(esp_avrc_tg_register_callback(bt_app_rc_tg_cb));
    // ESP_ERROR_CHECK(esp_avrc_tg_init());
    // esp_avrc_rn_evt_cap_mask_t evt_set = {0};
    // assert(esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE));
    // ESP_ERROR_CHECK(esp_avrc_tg_set_rn_evt_cap(&evt_set));

    // initialize A2DP and regsiter callbacks
    // configure the A2DP to use bluedroid codec (SBC -> PCM)
    ESP_ERROR_CHECK(esp_a2d_sink_init());
    ESP_ERROR_CHECK(esp_a2d_register_callback(&bt_app_a2d_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb));
    // register for A2DP delay support
    ESP_ERROR_CHECK(esp_a2d_sink_get_delay_value());

    // set discoverable and connectable mode, wait to be connected
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));

    ESP_LOGI(LOG_TAG, "initialized");
}

// bluetooth event callback handlers
//------------------------------------------------------------------------------
static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    uint8_t *bda = NULL;

    switch (event)
    {
    /* when authentication completed, this event comes */
    case ESP_BT_GAP_AUTH_CMPL_EVT:
    {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(LOG_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            // ESP_LOG_BUFFER_HEX_LEVEL(LOG_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN, ESP_LOG_INFO);
            // TODO store pairing in nvm
        }
        else
            ESP_LOGW(LOG_TAG, "authentication failed, status: %d", param->auth_cmpl.stat);
        break;
    }

    /* others */
    default:
    {
        ESP_LOGD(LOG_TAG, "Ignored ESP_BT_GAP event: %d", event);
        break;
    }
    }
}

static void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    switch (event)
    {
    case ESP_AVRC_TG_CONNECTION_STATE_EVT:
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
    case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
    case ESP_AVRC_TG_SET_PLAYER_APP_VALUE_EVT:
    case ESP_AVRC_TG_PROF_STATE_EVT:
        bt_app_work_dispatch(bt_av_hdl_avrc_tg_evt, event, param, sizeof(esp_avrc_tg_cb_param_t), NULL);
        break;
    default:
        ESP_LOGE(LOG_TAG, "Invalid AVRC event: %d", event);
        break;
    }
}

static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_PROF_STATE_EVT:
    case ESP_A2D_SEP_REG_STATE_EVT:
    case ESP_A2D_SNK_PSC_CFG_EVT:
    case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
    case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
    {
        bt_app_work_dispatch(bt_av_hdl_a2d_evt, event, param, sizeof(esp_a2d_cb_param_t), NULL);
        break;
    }
    default:
        ESP_LOGE(LOG_TAG, "Invalid A2DP event: %d", event);
        break;
    }
}

static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    // write the PCM encoded data to the ring-buffer
    write_ringbuf(data, len);
}