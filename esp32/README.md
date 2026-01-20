Implementation in C for ESP32
================================================================================
Unfortunately the current rust esp_hal doesn't provide
* BlueTooth Legacy
    * A2DP support for use as a bluetooth speaker
* I2S required functionality for modulation

Once the functionality is implemented in esp_hal, this implementation will be
deleted.

Usage
================================================================================
* Prerequisites
    * [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#installation)

### Flash an ESP32
```sh
idf.py flash
```

### Flash/Monitor an ESP32
```sh
idf.py flash monitor
```

