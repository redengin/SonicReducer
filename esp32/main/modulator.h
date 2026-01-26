// inspired by https://github.com/Alexxdal/ESP32FMRadio/

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/// route GPIO_NUM_0 for modulator ouptut
void modulator_init(void);

typedef struct {
    uint32_t pcm_sample_rate_hz;
    size_t ch_count;    // number of channels [1, 2]
} modulator_config_t;

/// configure the modulator
void modulator_config(const modulator_config_t* const);

/// start the modulator
void modulator_start(void);

/// start the modulator
void modulator_stop(void);

