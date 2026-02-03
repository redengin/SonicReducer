Implementation in C for ESP32
================================================================================
Unfortunately the current rust esp_hal doesn't provide
* BlueTooth Legacy
    * A2DP support for use as a bluetooth speaker

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

<!-- NOTE to self checkout https://github.com/ok-home/logic_analyzer -->

How it works
================================================================================
The esp-idf provides Legacy Bluetooth [A2DP] via the bluedroid stack. 

[A2DP] transfers sound using [SBC] encoding. The bluedroid stack provides
[SBC] -> [PCM] decoding.


To drive the LN298N

<!-- footnotes -->
[A2DP]:https://en.wikipedia.org/wiki/List_of_Bluetooth_profiles#Advanced_Audio_Distribution_Profile_(A2DP)

[SBC]:https://en.wikipedia.org/wiki/SBC_(codec)

[PCM]:https://en.wikipedia.org/wiki/Pulse-code_modulation