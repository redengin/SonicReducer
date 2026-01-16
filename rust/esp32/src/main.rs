#![no_std]
#![no_main]

// provide panic handler
use sonic_reducer_esp32::{self as _};
// use esp_backtrace as _;  // use the esp32 supplied panic handler

// provide logging primitives
use log::*;

// provide heap allocator
use sonic_reducer_esp32::{create_heap};

#[esp_rtos::main]
async fn main(_spawner: embassy_executor::Spawner) -> ! {

    // initialize logging
    esp_println::logger::init_logger_from_env();
    trace!("initializing");

    create_heap!();

    // #[allow(unused)]
    // let peripherals = esp_hal::init(esp_hal::Config::default());
    // let delay = Delay::new();

    // esp_println::logger::init_logger_from_env();

    loop {
    //     log::info!("Hello world!");
    //     delay.delay(500.millis());
    }
}

