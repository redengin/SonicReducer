#![no_std]
#![no_main]

use esp_hal::mcpwm::operator::PwmPinConfig;
// provide panic handler
use sonic_reducer_esp32::{self as _};
// use esp_backtrace as _;  // use the esp32 supplied panic handler

// provide logging primitives
use log::*;

#[esp_hal::main]
fn main() -> ! {
    // initialize the SoC interface
    let peripherals = esp_hal::init(esp_hal::Config::default());

    // initialize logging
    esp_println::logger::init_logger_from_env();
    info!("initializing");

    // initialize the mcpwm peripheral
    use esp_hal::mcpwm::{McPwm, PeripheralClockConfig};
    use esp_hal::time::Rate;
    // use the default clock
    let clock_cfg = PeripheralClockConfig::with_prescaler(0);
    // TODO use a slower clock if it saves power
    // let clock_cfg = PeripheralClockConfig::with_frequency(
    //     Rate::from_khz(2 * OUTPUT_FREQ_KHZ) // double source clock for nyquist rate
    // ).unwrap();
    let mut mcpwm = McPwm::new(peripherals.MCPWM0, clock_cfg);

    // configure mcpwm for our output
    const OUTPUT_FREQ_KHZ:u32 = 40; 
    mcpwm.operator0.set_timer(&mcpwm.timer0);
    mcpwm.timer0.start(
        clock_cfg.timer_clock_with_frequency(
            99, // period [aka duty cycle 0-100]
            esp_hal::mcpwm::timer::PwmWorkingMode::Increase,
            Rate::from_khz(OUTPUT_FREQ_KHZ)
        ).unwrap()
    );

    // configure coupled pins for H-bridge
    use esp_hal::mcpwm::operator::DeadTimeCfg;
    let bridge_active = DeadTimeCfg::new_ahc(); // Active High Complementary
    use esp_hal::mcpwm::operator::PWMStream;
    let bridge_off = DeadTimeCfg::new_bypass().set_output_swap(PWMStream::PWMA, true);
    let mut pins = mcpwm.operator0.with_linked_pins(
        peripherals.GPIO26,
        PwmPinConfig::UP_DOWN_ACTIVE_HIGH,
        // complementary pin controlled by AHC
        peripherals.GPIO27, PwmPinConfig::EMPTY, bridge_off,
    );
    pins.set_deadtime_cfg(bridge_active);

    // start the output (50% duty cycle to produce square wave)
    pins.set_timestamp_a(50 /* duty cycle */);

    info!("emitting 40 KHz signal");

    loop {

    }
}
