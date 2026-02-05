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
[SBC] -> [PCM] decoding, creating 16bit PCM at the A2DP sample rate.

Modulator
--------------------------------------------------------------------------------
This implementation supports using an H-bridge or an amplifier.

### Amplifier
The ESP32 DAC output is 0.08 V - 3.16 V. As these are only positive voltages,
the amplifier must be designed to these levels, or additional circuitry used
to convert the DAC output to the amplifier input.

<!-- footnotes -->
[A2DP]:https://en.wikipedia.org/wiki/List_of_Bluetooth_profiles#Advanced_Audio_Distribution_Profile_(A2DP)

[SBC]:https://en.wikipedia.org/wiki/SBC_(codec)

[PCM]:https://en.wikipedia.org/wiki/Pulse-code_modulation