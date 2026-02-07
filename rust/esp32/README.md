Building/Flashing ESP32
--------------------------------------------------------------------------------
```sh
# make sure you've setup esp rust environment (aka export-esp.sh)
cargo run
```

Functions
--------------------------------------------------------------------------------
* [Modulation](docs/modulation.md) - FM modulation
    * uses MCPWM to generate a H-Bridge square wave at 40 KHz
<!--
    * at this this time esp32 rust doesn't support I2S for modulation
* [BlueTooth](docs/bluetooth.md)
    * at this time esp32 rust doesn't support BlueTooth legacy
-->

--------------------------------------------------------------------------------


### How this project was created
#### Prerequisites [Setting up for ESP32 Rust]()
* [RISC-V and Xtensa](https://docs.esp-rs.org/book/installation/riscv-and-xtensa.html)
* [RISC-V only](https://docs.esp-rs.org/book/installation/riscv.html)
* [ESP-FLASH](https://docs.esp-rs.org/book/tooling/espflash.html)

#### code generator command
```sh
cargo generate https://github.com/esp-rs/esp-template
# didn't enable advanced template configuration (we'll add the stuff as needed)
```


Wiring
================================================================================
* ESP32 powered by USB
* L298N powered by LIPO (share a ground with ESP32 for signal integrity)
    * can handle upto 35 V (8S 29.6 V)
* Modulated Signal Pins
    * ESP32 GPIO26 --> L298N IN1
    * ESP32 GPIO27 --> L298N IN2
* Ultrasonic speakers connected in parallel to L298N output