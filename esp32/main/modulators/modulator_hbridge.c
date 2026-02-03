#include <sdkconfig.h>
#ifdef CONFIG_SONIC_REDUCER_MODULATOR_HBRIDGE

#include "../modulator.h"

/// implemented per https://www.reddit.com/r/arduino/comments/1gfatep/using_esp32_l298n_for_ultrasonic_speakers/

void modulator_create(void)
{
    // TODO implement
}

void modulator_destroy(void)
{
    // TODO implement
}

/// configure the modulator
void modulator_config(const modulator_config_t* const)
{
    // TODO implement
}

/// start the ouptput
void modulator_start(void)
{
    // TODO implement
}

/// send pcm data to the modulator
void modulator_write(const uint8_t* const data, const size_t sz)
{
    // TODO implement
}

/// stop the output
void modulator_stop(void)
{
    // TODO implement
}


#endif

