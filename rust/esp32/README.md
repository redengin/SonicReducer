# Building/Flashing ESP32
```sh
# make sure you've setup esp rust environment (aka export-esp.sh)
cargo run
```




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