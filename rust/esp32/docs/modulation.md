ESP32 FM Modulation
================================================================================

Background
--------------------------------------------------------------------------------
[ESP32FMRadio](https://github.com/Alexxdal/ESP32FMRadio) demonstrates how to
generate an FM modulated GPIO signal.

Our usage is a 40 KHz carrier. The ESP32FMRadio leverages the APLL for higher
precision - the APLL only supports 16-128 MHz (outside our required frequency).
As the precision of esp32 clocks for 40 KHz signals are sufficient, we don't use
APLL.


ESP32 Implementation
--------------------------------------------------------------------------------
The [I2S][I2S] (Inter-IC Sound) peripheral produces a clock (MCLK) to drive a
GPIO (the input to the amplifier).


<!-- References -->
[I2S]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2s.html
