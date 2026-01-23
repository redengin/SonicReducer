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
// bt event callbacks
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

    // configure the A2DP to use bluedroid codec
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb));

    // register for delay reporting
    // ESP_ERROR_CHECK(esp_a2d_sink_get_delay_value());

    // TODO - not sure what this does
    // ESP_ERROR_CHECK(esp_bt_gap_get_device_name());

    // set discoverable and connectable mode, wait to be connected
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));

    ESP_LOGI(LOG_TAG, "initialized");
}

// bluetooth event callback handlers
//------------------------------------------------------------------------------
#include "bt_a2dp_worker.h"

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

// static uint32_t s_pkt_cnt = 0;
static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    write_ringbuf(data, len);

    /* log the number every 100 packets */
    // if (++s_pkt_cnt % 100 == 0)
    // {
    //     ESP_LOGI(LOG_TAG, "Audio packet count: %" PRIu32, s_pkt_cnt);
    // }
}

// #include <freertos/ringbuf.h>
// #include <freertos/semphr.h>
// enum
// {
//     RINGBUFFER_MODE_PROCESSING,  ///< ringbuffer is buffering incoming audio data, I2S is working
//     RINGBUFFER_MODE_PREFETCHING, ///< ringbuffer is buffering incoming audio data, I2S is waiting
//     RINGBUFFER_MODE_DROPPING     ///< ringbuffer is not buffering (dropping) incoming audio data, I2S is working
// };
// static uint16_t ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;
// #define RINGBUF_HIGHEST_WATER_LEVEL (32 * 1024)
// #define RINGBUF_PREFETCH_WATER_LEVEL (20 * 1024)
// /// handle of ringbuffer for I2S
// static RingbufHandle_t s_ringbuf_i2s = NULL;
// static SemaphoreHandle_t s_i2s_write_semaphore = NULL;

// static size_t write_ringbuf(const uint8_t *data, size_t size)
// {
//     size_t item_size = 0;
//     BaseType_t done = pdFALSE;

//     if (ringbuffer_mode == RINGBUFFER_MODE_DROPPING)
//     {
//         ESP_LOGW(LOG_TAG, "ringbuffer is full, drop this packet!");
//         vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &item_size);
//         if (item_size <= RINGBUF_PREFETCH_WATER_LEVEL)
//         {
//             ESP_LOGI(LOG_TAG, "ringbuffer data decreased! mode changed: RINGBUFFER_MODE_PROCESSING");
//             ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;
//         }
//         return 0;
//     }

//     done = xRingbufferSend(s_ringbuf_i2s, (void *)data, size, (TickType_t)0);

//     if (!done)
//     {
//         ESP_LOGW(LOG_TAG, "ringbuffer overflowed, ready to decrease data! mode changed: RINGBUFFER_MODE_DROPPING");
//         ringbuffer_mode = RINGBUFFER_MODE_DROPPING;
//     }

//     if (ringbuffer_mode == RINGBUFFER_MODE_PREFETCHING)
//     {
//         vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &item_size);
//         if (item_size >= RINGBUF_PREFETCH_WATER_LEVEL)
//         {
//             ESP_LOGI(LOG_TAG, "ringbuffer data increased! mode changed: RINGBUFFER_MODE_PROCESSING");
//             ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;
//             if (pdFALSE == xSemaphoreGive(s_i2s_write_semaphore))
//             {
//                 ESP_LOGE(LOG_TAG, "semphore give failed");
//             }
//         }
//     }

//     return done ? size : 0;
// }

// // task management
// //------------------------------------------------------------------------------

// // forward declarations
// /// signal for `bt_app_work_dispatch`
// #define BT_APP_SIG_WORK_DISPATCH (0x01)
// typedef struct
// {
//     uint16_t sig;   /*!< signal to bt_app_task */
//     uint16_t event; /*!< message event id */
//     bt_app_cb_t cb; /*!< context switch callback */
//     void *param;    /*!< parameter area needs to be last */
// } bt_app_msg_t;
// static bool bt_app_send_msg(bt_app_msg_t *msg);

// /// handle of work queue
// static QueueHandle_t s_bt_app_task_queue = NULL;

// static bool bt_app_send_msg(bt_app_msg_t *msg)
// {
//     if (msg == NULL)
//     {
//         return false;
//     }

//     /* send the message to work queue */
//     if (xQueueSend(s_bt_app_task_queue, msg, 10 / portTICK_PERIOD_MS) != pdTRUE)
//     {
//         ESP_LOGE(LOG_TAG, "%s xQueue send failed", __func__);
//         return false;
//     }
//     return true;
// }

// static bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback)
// {
//     ESP_LOGD(LOG_TAG, "%s event: 0x%x, param len: %d", __func__, event, param_len);

//     bt_app_msg_t msg;
//     memset(&msg, 0, sizeof(bt_app_msg_t));

//     msg.sig = BT_APP_SIG_WORK_DISPATCH;
//     msg.event = event;
//     msg.cb = p_cback;

//     if (param_len == 0)
//     {
//         return bt_app_send_msg(&msg);
//     }
//     else if (p_params && param_len > 0)
//     {
//         if ((msg.param = malloc(param_len)) != NULL)
//         {
//             memcpy(msg.param, p_params, param_len);
//             /* check if caller has provided a copy callback to do the deep copy */
//             if (p_copy_cback)
//             {
//                 p_copy_cback(msg.param, p_params, param_len);
//             }
//             return bt_app_send_msg(&msg);
//         }
//     }

//     return false;
// }

// // task handlers
// //------------------------------------------------------------------------------

// static void bt_av_hdl_avrc_tg_evt(uint16_t event, void *p_param)
// {
//     ESP_LOGD(LOG_TAG, "%s event: %d", __func__, event);

//     esp_avrc_tg_cb_param_t *rc = (esp_avrc_tg_cb_param_t *)(p_param);

//     switch (event)
//     {
//     /* when connection state changed, this event comes */
//     case ESP_AVRC_TG_CONNECTION_STATE_EVT:
//     {
//         uint8_t *bda = rc->conn_stat.remote_bda;
//         ESP_LOGI(LOG_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
//                  rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

//         // FIXME DISABLED
//         // if (rc->conn_stat.connected) {
//         //     /* create task to simulate volume change */
//         //     xTaskCreate(volume_change_simulation, "vcsTask", 2048, NULL, 5, &s_vcs_task_hdl);
//         // } else {
//         //     vTaskDelete(s_vcs_task_hdl);
//         //     ESP_LOGI(LOG_TAG, "Stop volume change simulation");
//         // }
//         break;
//     }
//     /* when passthrough commanded, this event comes */
//     case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
//     {
//         ESP_LOGI(LOG_TAG, "AVRC passthrough cmd: key_code 0x%x, key_state %d", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
//         break;
//     }
//     /* when absolute volume command from remote device set, this event comes */
//     case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
//     {
//         ESP_LOGI(LOG_TAG, "AVRC set absolute volume: %d%%", (int)rc->set_abs_vol.volume * 100 / 0x7f);
//         // FIXME DISABLED
//         // volume_set_by_controller(rc->set_abs_vol.volume);
//         break;
//     }
//     /* when notification registered, this event comes */
//     case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
//     {
//         ESP_LOGI(LOG_TAG, "AVRC register event notification: %d, param: 0x%" PRIx32, rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
//         // FIXME DISABLED
//         // if (rc->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
//         //     s_volume_notify = true;
//         //     esp_avrc_rn_param_t rn_param;
//         //     rn_param.volume = s_volume;
//         //     esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
//         // }
//         break;
//     }
//     /* when feature of remote device indicated, this event comes */
//     case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
//     {
//         ESP_LOGI(LOG_TAG, "AVRC remote features: %" PRIx32 ", CT features: %x", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
//         break;
//     }
//     /* when avrcp target init or deinit completed, this event comes */
//     case ESP_AVRC_TG_PROF_STATE_EVT:
//     {
//         if (ESP_AVRC_INIT_SUCCESS == rc->avrc_tg_init_stat.state)
//         {
//             ESP_LOGI(LOG_TAG, "AVRCP TG STATE: Init Complete");
//         }
//         else if (ESP_AVRC_DEINIT_SUCCESS == rc->avrc_tg_init_stat.state)
//         {
//             ESP_LOGI(LOG_TAG, "AVRCP TG STATE: Deinit Complete");
//         }
//         else
//         {
//             ESP_LOGE(LOG_TAG, "AVRCP TG STATE error: %d", rc->avrc_tg_init_stat.state);
//         }
//         break;
//     }
//     /* others */
//     default:
//         ESP_LOGE(LOG_TAG, "%s unhandled event: %d", __func__, event);
//         break;
//     }
// }

// static void bt_av_hdl_a2d_evt(uint16_t event, void *p_param)
// {
//     ESP_LOGD(LOG_TAG, "%s event: %d", __func__, event);

//     esp_a2d_cb_param_t *a2d = NULL;

//     switch (event)
//     {
//     /* when connection state changed, this event comes */
//     case ESP_A2D_CONNECTION_STATE_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         uint8_t *bda = a2d->conn_stat.remote_bda;
//         const char *s_a2d_conn_state_str[] = {"Disconnected", "Connecting", "Connected", "Disconnecting"};
//         ESP_LOGI(LOG_TAG, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
//                  s_a2d_conn_state_str[a2d->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
//         if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
//         {
//             esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
//             bt_i2s_driver_uninstall();
//             bt_i2s_task_shut_down();
//         }
//         else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
//         {
//             esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
//             bt_i2s_task_start_up();
//         }
//         else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING)
//         {
//             bt_i2s_driver_install();
//         }
//         break;
//     }
//     /* when audio stream transmission state changed, this event comes */
//     case ESP_A2D_AUDIO_STATE_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         const char *s_a2d_audio_state_str[] = {"Suspended", "Started"};
//         ESP_LOGI(LOG_TAG, "A2DP audio state: %s", s_a2d_audio_state_str[a2d->audio_stat.state]);
//         static esp_a2d_audio_state_t s_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
//         s_audio_state = a2d->audio_stat.state;
//         if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state)
//         {
//             s_pkt_cnt = 0;
//         }
//         break;
//     }
//     /* when audio codec is configured, this event comes */
//     case ESP_A2D_AUDIO_CFG_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         esp_a2d_mcc_t *p_mcc = &a2d->audio_cfg.mcc;
//         ESP_LOGI(LOG_TAG, "A2DP audio stream configuration, codec type: %d", p_mcc->type);
//         /* for now only SBC stream is supported */
//         if (p_mcc->type == ESP_A2D_MCT_SBC)
//         {
//             int sample_rate = 16000;
//             int ch_count = 2;
//             if (p_mcc->cie.sbc_info.samp_freq & ESP_A2D_SBC_CIE_SF_32K)
//             {
//                 sample_rate = 32000;
//             }
//             else if (p_mcc->cie.sbc_info.samp_freq & ESP_A2D_SBC_CIE_SF_44K)
//             {
//                 sample_rate = 44100;
//             }
//             else if (p_mcc->cie.sbc_info.samp_freq & ESP_A2D_SBC_CIE_SF_48K)
//             {
//                 sample_rate = 48000;
//             }

//             if (p_mcc->cie.sbc_info.ch_mode & ESP_A2D_SBC_CIE_CH_MODE_MONO)
//             {
//                 ch_count = 1;
//             }

//             // FIXME configure I2S output
//             // i2s_channel_disable(tx_chan);
//             // i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
//             // i2s_std_slot_config_t slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, ch_count);
//             // i2s_channel_reconfig_std_clock(tx_chan, &clk_cfg);
//             // i2s_channel_reconfig_std_slot(tx_chan, &slot_cfg);
//             // i2s_channel_enable(tx_chan);

//             ESP_LOGI(LOG_TAG, "Configure audio player: 0x%x-0x%x-0x%x-0x%x-0x%x-%d-%d",
//                      p_mcc->cie.sbc_info.samp_freq,
//                      p_mcc->cie.sbc_info.ch_mode,
//                      p_mcc->cie.sbc_info.block_len,
//                      p_mcc->cie.sbc_info.num_subbands,
//                      p_mcc->cie.sbc_info.alloc_mthd,
//                      p_mcc->cie.sbc_info.min_bitpool,
//                      p_mcc->cie.sbc_info.max_bitpool);
//             ESP_LOGI(LOG_TAG, "Audio player configured, sample rate: %d", sample_rate);
//         }
//         break;
//     }
//     /* when a2dp init or deinit completed, this event comes */
//     case ESP_A2D_PROF_STATE_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         if (ESP_A2D_INIT_SUCCESS == a2d->a2d_prof_stat.init_state)
//         {
//             ESP_LOGI(LOG_TAG, "A2DP PROF STATE: Init Complete");
//         }
//         else
//         {
//             ESP_LOGI(LOG_TAG, "A2DP PROF STATE: Deinit Complete");
//         }
//         break;
//     }
//     /* when using external codec, after sep registration done, this event comes */
//     case ESP_A2D_SEP_REG_STATE_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         if (a2d->a2d_sep_reg_stat.reg_state == ESP_A2D_SEP_REG_SUCCESS)
//         {
//             ESP_LOGI(LOG_TAG, "A2DP register SEP success, seid: %d", a2d->a2d_sep_reg_stat.seid);
//         }
//         else
//         {
//             ESP_LOGI(LOG_TAG, "A2DP register SEP fail, seid: %d, state: %d", a2d->a2d_sep_reg_stat.seid, a2d->a2d_sep_reg_stat.reg_state);
//         }
//         break;
//     }
//     /* When protocol service capabilities configured, this event comes */
//     case ESP_A2D_SNK_PSC_CFG_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         ESP_LOGI(LOG_TAG, "protocol service capabilities configured: 0x%x ", a2d->a2d_psc_cfg_stat.psc_mask);
//         if (a2d->a2d_psc_cfg_stat.psc_mask & ESP_A2D_PSC_DELAY_RPT)
//         {
//             ESP_LOGI(LOG_TAG, "Peer device support delay reporting");
//         }
//         else
//         {
//             ESP_LOGI(LOG_TAG, "Peer device unsupported delay reporting");
//         }
//         break;
//     }
//     /* when set delay value completed, this event comes */
//     case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         if (ESP_A2D_SET_INVALID_PARAMS == a2d->a2d_set_delay_value_stat.set_state)
//         {
//             ESP_LOGI(LOG_TAG, "Set delay report value: fail");
//         }
//         else
//         {
//             ESP_LOGI(LOG_TAG, "Set delay report value: success, delay_value: %u * 1/10 ms", a2d->a2d_set_delay_value_stat.delay_value);
//         }
//         break;
//     }
//     /* when get delay value completed, this event comes */
//     case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
//     {
//         a2d = (esp_a2d_cb_param_t *)(p_param);
//         ESP_LOGI(LOG_TAG, "Get delay report value: delay_value: %u * 1/10 ms", a2d->a2d_get_delay_value_stat.delay_value);
//         /* Default delay value plus delay caused by application layer */
//         esp_a2d_sink_set_delay_value(a2d->a2d_get_delay_value_stat.delay_value + APP_DELAY_VALUE);
//         break;
//     }
//     /* others */
//     default:
//         ESP_LOGE(LOG_TAG, "%s unhandled event: %d", __func__, event);
//         break;
//     }
// }
