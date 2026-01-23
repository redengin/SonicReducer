#include "bt_a2dp.h"

#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <esp_avrc_api.h>
#include <esp_a2dp_api.h>

static const char *LOG_TAG = "bt-a2dp";

// forward declarations
static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
static void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
static void bt_app_a2d_audio_data_cb(esp_a2d_conn_hdl_t conn_hdl, esp_a2d_audio_buff_t *audio_buf);
typedef void (* bt_app_cb_t) (uint16_t event, void *param);
typedef void (* bt_app_copy_cb_t) (void *p_dest, void *p_src, int len);
bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);
static void bt_av_hdl_avrc_tg_evt(uint16_t event, void *p_param);


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

    // set the published bluetooth nane
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(device_name));

    // register callbacks
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_app_gap_cb));
    ESP_ERROR_CHECK(esp_avrc_ct_init());
    ESP_ERROR_CHECK(esp_avrc_tg_register_callback(bt_app_rc_tg_cb));
    ESP_ERROR_CHECK(esp_avrc_tg_init());
    esp_avrc_rn_evt_cap_mask_t evt_set = {0};
    assert(esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE));
    ESP_ERROR_CHECK(esp_avrc_tg_set_rn_evt_cap(&evt_set));
    ESP_ERROR_CHECK(esp_a2d_register_callback(&bt_app_a2d_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_init());

    // configure the A2DP support - ESP32 only supports mSBC currently
    esp_a2d_mcc_t mcc = {0};
    mcc.type = ESP_A2D_MCT_SBC;
    mcc.cie.sbc_info.samp_freq = 0xf;
    mcc.cie.sbc_info.ch_mode = 0xf;
    mcc.cie.sbc_info.block_len = 0xf;
    mcc.cie.sbc_info.num_subbands = 0x3;
    mcc.cie.sbc_info.alloc_mthd = 0x3;
    mcc.cie.sbc_info.max_bitpool = 250;
    mcc.cie.sbc_info.min_bitpool = 2;
    ESP_ERROR_CHECK(esp_a2d_sink_register_stream_endpoint(0, &mcc));
    ESP_ERROR_CHECK(esp_a2d_sink_register_audio_data_callback(bt_app_a2d_audio_data_cb));

    // register for delay reporting
    // ESP_ERROR_CHECK(esp_a2d_sink_get_delay_value());

    // TODO - not sure what this does
    // ESP_ERROR_CHECK(esp_bt_gap_get_device_name());

    // set discoverable and connectable mode, wait to be connected
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));

    ESP_LOGI(LOG_TAG, "initialized");
}

static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    uint8_t *bda = NULL;

    switch (event)
    {
    /* when authentication completed, this event comes */
    case ESP_BT_GAP_AUTH_CMPL_EVT:
    {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
            ESP_LOGI(LOG_TAG, "authentication success: %s", param->auth_cmpl.device_name);
        // ESP_LOG_BUFFER_HEX(LOG_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        else
            ESP_LOGW(LOG_TAG, "authentication failed, status: %d", param->auth_cmpl.stat);
        break;
    }
    case ESP_BT_GAP_ENC_CHG_EVT:
    {
        const char *str_enc[3] = {"OFF", "E0", "AES"};
        bda = (uint8_t *)param->enc_chg.bda;
        ESP_LOGI(LOG_TAG, "Encryption mode to [%02x:%02x:%02x:%02x:%02x:%02x] changed to %s",
                 bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], str_enc[param->enc_chg.enc_mode]);
        break;
    }

    /* when Security Simple Pairing user confirmation requested, this event comes */
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(LOG_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %06" PRIu32, param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    /* when Security Simple Pairing passkey notified, this event comes */
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(LOG_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey: %06" PRIu32, param->key_notif.passkey);
        break;
    /* when Security Simple Pairing passkey requested, this event comes */
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(LOG_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;

    /* when GAP mode changed, this event comes */
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(LOG_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode: %d, interval: %.2f ms",
                 param->mode_chg.mode, param->mode_chg.interval * 0.625);
        break;
    /* when ACL connection completed, this event comes */
    case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        bda = (uint8_t *)param->acl_conn_cmpl_stat.bda;
        ESP_LOGI(LOG_TAG, "ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT Connected to [%02x:%02x:%02x:%02x:%02x:%02x], status: 0x%x",
                 bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_conn_cmpl_stat.stat);
        break;
    /* when ACL disconnection completed, this event comes */
    case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        bda = (uint8_t *)param->acl_disconn_cmpl_stat.bda;
        ESP_LOGI(LOG_TAG, "ESP_BT_GAP_ACL_DISC_CMPL_STAT_EVT Disconnected from [%02x:%02x:%02x:%02x:%02x:%02x], reason: 0x%x",
                 bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], param->acl_disconn_cmpl_stat.reason);
        break;
    /* others */
    default:
    {
        ESP_LOGI(LOG_TAG, "event: %d", event);
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
    // TODO implement
}

static void bt_app_a2d_audio_data_cb(esp_a2d_conn_hdl_t conn_hdl, esp_a2d_audio_buff_t *audio_buf)
{
    // TODO implement
}

bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback)
{
    // TODO implement
    return false;
}

static void bt_av_hdl_avrc_tg_evt(uint16_t event, void *p_param)
{
    // ESP_LOGD(LOG_TAG, "%s event: %d", __func__, event);

    // esp_avrc_tg_cb_param_t *rc = (esp_avrc_tg_cb_param_t *)(p_param);

    // switch (event) {
    // /* when connection state changed, this event comes */
    // case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
    //     uint8_t *bda = rc->conn_stat.remote_bda;
    //     ESP_LOGI(LOG_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
    //              rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    //     if (rc->conn_stat.connected) {
    //         /* create task to simulate volume change */
    //         xTaskCreate(volume_change_simulation, "vcsTask", 2048, NULL, 5, &s_vcs_task_hdl);
    //     } else {
    //         vTaskDelete(s_vcs_task_hdl);
    //         ESP_LOGI(LOG_TAG, "Stop volume change simulation");
    //     }
    //     break;
    // }
    // /* when passthrough commanded, this event comes */
    // case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT: {
    //     ESP_LOGI(LOG_TAG, "AVRC passthrough cmd: key_code 0x%x, key_state %d", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
    //     break;
    // }
    // /* when absolute volume command from remote device set, this event comes */
    // case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
    //     ESP_LOGI(LOG_TAG, "AVRC set absolute volume: %d%%", (int)rc->set_abs_vol.volume * 100 / 0x7f);
    //     volume_set_by_controller(rc->set_abs_vol.volume);
    //     break;
    // }
    // /* when notification registered, this event comes */
    // case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
    //     ESP_LOGI(LOG_TAG, "AVRC register event notification: %d, param: 0x%"PRIx32, rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
    //     if (rc->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
    //         s_volume_notify = true;
    //         esp_avrc_rn_param_t rn_param;
    //         rn_param.volume = s_volume;
    //         esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
    //     }
    //     break;
    // }
    // /* when feature of remote device indicated, this event comes */
    // case ESP_AVRC_TG_REMOTE_FEATURES_EVT: {
    //     ESP_LOGI(LOG_TAG, "AVRC remote features: %"PRIx32", CT features: %x", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
    //     break;
    // }
    // /* when avrcp target init or deinit completed, this event comes */
    // case ESP_AVRC_TG_PROF_STATE_EVT: {
    //     if (ESP_AVRC_INIT_SUCCESS == rc->avrc_tg_init_stat.state) {
    //         ESP_LOGI(LOG_TAG, "AVRCP TG STATE: Init Complete");
    //     } else if (ESP_AVRC_DEINIT_SUCCESS == rc->avrc_tg_init_stat.state) {
    //         ESP_LOGI(LOG_TAG, "AVRCP TG STATE: Deinit Complete");
    //     } else {
    //         ESP_LOGE(LOG_TAG, "AVRCP TG STATE error: %d", rc->avrc_tg_init_stat.state);
    //     }
    //     break;
    // }
    // /* others */
    // default:
    //     ESP_LOGE(LOG_TAG, "%s unhandled event: %d", __func__, event);
    //     break;
    // }
}