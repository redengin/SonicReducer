#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void bt_a2dp_worker_init(void);

typedef void (*bt_app_cb_t)(uint16_t event, void *param);
typedef void (*bt_app_copy_cb_t)(void *p_dest, void *p_src, int len);
bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);

// callback handlers for bt_app_work_dispatch
// void bt_av_hdl_avrc_tg_evt(uint16_t event, void *p_param);
void bt_av_hdl_a2d_evt(uint16_t event, void *p_param);

/// add data to the i2S ringbuffer
size_t write_ringbuf(const uint8_t *data, size_t size);
