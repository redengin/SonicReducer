#pragma once

#include <stdint.h>

#define SIZEOF_PIN_CODE 4
typedef uint8_t pin_code_t[SIZEOF_PIN_CODE];
/// initialize bluetooth A2DP
void bt_a2dp_init(
    const char* const device_name,  ///< bluetooth published name
    const pin_code_t pin_code       ///< pairing pin code (or NULL if SSP not used)
);