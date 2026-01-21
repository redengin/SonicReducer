#include "bt_a2dp.h"

#include <memory.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>

void bt_a2dp_init(
    const char *const device_name, ///< bluetooth published name
    const pin_code_t pin_code      ///< pairing pin code (or NULL if SSP not used)
)
{
    // FIXME implement
}
