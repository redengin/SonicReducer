#![no_std]
#![no_main]

// provide panic handler
use sonic_reducer_esp32::{self as _};
// use esp_backtrace as _;  // use the esp32 supplied panic handler

// provide logging primitives
use log::*;

// provide heap allocator
// use sonic_reducer_esp32::{create_heap};

// provice scheduling primitives
use embassy_time::{Duration, Timer};

#[esp_rtos::main]
async fn main(spawner: embassy_executor::Spawner) -> ! {

    // initialize the SoC interface
    let peripherals = esp_hal::init(
        // max out clock to support radio
        esp_hal::Config::default()
            .with_cpu_clock(esp_hal::clock::CpuClock::max())
    );

    // initialize logging
    esp_println::logger::init_logger_from_env();
    info!("initializing");

    // initialize the rtos
    use esp_hal::timer::timg::TimerGroup;
    let timg0 = TimerGroup::new(peripherals.TIMG0);
    use esp_hal::interrupt::software::SoftwareInterruptControl;
    let sw_int = SoftwareInterruptControl::new(peripherals.SW_INTERRUPT);
    esp_rtos::start(timg0.timer0, sw_int.software_interrupt0);

    // start the modulator
    let i2s = esp_hal::i2s::master::I2s::new(
        peripherals.I2S0,
        peripherals.DMA_I2S0,
        esp_hal::i2s::master::Config::new_tdm_msb() // FIXME
    );
    spawner.spawn(task_modulator()).unwrap();
    // let (mut rx_buffer, rx_desriptors, _, _)  = dma_buffers!(4 * 4092, 0);


    // initialize the bluetooth hardware
    // use default 64K heap (required by radio)
    // create_heap!();
    // FIXME esp32_radio currently only supports BLE
    // https://github.com/esp-rs/esp-hal/issues/3401


    loop {
        info!("Hello world!");
        Timer::after(Duration::from_secs(1)).await;
    }
}


#[embassy_executor::task]
async fn task_modulator() -> ! {
    loop{
        info!("modulating");
        Timer::after(Duration::from_secs(1)).await;
    }
}
