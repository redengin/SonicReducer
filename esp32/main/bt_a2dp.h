#pragma once

#include <stdint.h>

#define PIN_CODE_LENGTH 4
typedef char pin_code_t[PIN_CODE_LENGTH];
/// initialize bluetooth A2DP
void bt_a2dp_init(
    const char* const device_name,  ///< bluetooth published name
    const pin_code_t pin_code       ///< pairing pin code
);