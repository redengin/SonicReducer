#include "bt_a2dp.h"

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

    // register GAP connection callbacks
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_app_gap_cb));
    // register for GAP client name
    ESP_ERROR_CHECK(esp_bt_gap_get_device_name());

    // initialize A2DP and regsiter callbacks
    ESP_ERROR_CHECK(esp_a2d_sink_init());
    // deprecated
    // ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb));
    esp_a2d_mcc_t mcc = {0};
    mcc.cie.sbc_info.ch_mode = ESP_A2D_SBC_CIE_CH_MODE_MONO;
    mcc.cie.sbc_info.samp_freq = ESP_A2D_SBC_CIE_SF_16K;
    mcc.cie.sbc_info.alloc_mthd = ESP_A2D_SBC_CIE_ALLOC_MTHD_SRN;
    mcc.cie.sbc_info.num_subbands = ESP_A2D_SBC_CIE_NUM_SUBBANDS_8;
    ESP_ERROR_CHECK(esp_a2d_sink_register_stream_endpoint(0 /*seid*/, &mcc));

    // register for PCM data
    ESP_ERROR_CHECK(esp_a2d_register_callback(&bt_app_a2d_cb));
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

#include "modulator.h"
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *a2d)
{
    switch (event)
    {
    /* when connection state changed, this event comes */
    case ESP_A2D_CONNECTION_STATE_EVT:
    {
        uint8_t *bda = a2d->conn_stat.remote_bda;
        const char *s_a2d_conn_state_str[] = {"Disconnected", "Connecting", "Connected", "Disconnecting"};
        ESP_LOGI(LOG_TAG, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 s_a2d_conn_state_str[a2d->conn_stat.state],
                 bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        switch (a2d->conn_stat.state)
        {
        case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
        {
            // TODO destroy the modulator
            modulator_destroy();

            // make the speaker discoverable
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            break;
        }
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
        {
            // ignore connection attempts
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

            // create the modulator
            modulator_create();
            break;
        }
        default:
            ESP_LOGD(LOG_TAG, "ignonred ESP_A2D_CONNECTION_STATE_EVT [%d]", event);
        };
        break;
    }
    /* when audio codec is configured, this event comes */
    case ESP_A2D_AUDIO_CFG_EVT:
    {
        const esp_a2d_mcc_t *const p_mcc = &a2d->audio_cfg.mcc;
        switch (p_mcc->type)
        {
        case ESP_A2D_MCT_SBC:
        {
            // determine the sample rate
            int sample_rate = 16000;
            if (p_mcc->cie.sbc_info.samp_freq & ESP_A2D_SBC_CIE_SF_32K)
                sample_rate = 32000;
            else if (p_mcc->cie.sbc_info.samp_freq & ESP_A2D_SBC_CIE_SF_44K)
                sample_rate = 44100;
            else if (p_mcc->cie.sbc_info.samp_freq & ESP_A2D_SBC_CIE_SF_48K)
                sample_rate = 48000;

            // determine the number of channels
            int ch_count = 2;
            if (p_mcc->cie.sbc_info.ch_mode & ESP_A2D_SBC_CIE_CH_MODE_MONO)
                ch_count = 1;

            ESP_LOGI(LOG_TAG,
                     "Audio player config, sample rate: %d, channels: %d,",
                     sample_rate, ch_count);

            // configure modulator
            const modulator_config_t config = {
                .pcm_sample_rate_hz = sample_rate,
            };
            modulator_config(&config);
            break;
        }
        default:
            ESP_LOGW(LOG_TAG, "Unacceptable A2DP codec [%d]", p_mcc->type);
            break;
        }
        break;
    }
    /* when audio stream transmission state changed, this event comes */
    case ESP_A2D_AUDIO_STATE_EVT:
    {
        const char *s_a2d_audio_state_str[] = {"Suspended", "Started"};
        ESP_LOGI(LOG_TAG, "A2DP audio state: %s", s_a2d_audio_state_str[a2d->audio_stat.state]);

        if (a2d->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED)
            // start the modulator
            modulator_start();
        else
            // stop the modulator
            modulator_stop();
        break;
    }
    /* when get delay value completed, this event comes */
    case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
    {
        ESP_LOGI(LOG_TAG, "Get delay report value: delay_value: %u * 1/10 ms", a2d->a2d_get_delay_value_stat.delay_value);
        /* Default delay value plus delay caused by application layer */
        static const size_t APP_DELAY_VALUE_ms = 50;
        esp_a2d_sink_set_delay_value(a2d->a2d_get_delay_value_stat.delay_value + APP_DELAY_VALUE_ms);
        break;
    }

    /* others */
    default:
        ESP_LOGD(LOG_TAG, "ignored ESP_A2D event: %d", event);
        break;
    }
}

/// send the PCM data to the modulator
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    ESP_LOGD(LOG_TAG, "a2dp data sz:%d", len);
    modulator_write(data, len);
}