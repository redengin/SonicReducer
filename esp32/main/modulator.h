// inspired by https://github.com/Alexxdal/ESP32FMRadio/

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void modulator_create(void);

void modulator_destroy(void);

typedef struct {
    uint32_t pcm_sample_rate_hz;
} modulator_config_t;

/// configure the modulator
void modulator_config(const modulator_config_t* const);

/// start the ouptput
void modulator_start(void);

/// send pcm data to the modulator
void modulator_write(const uint8_t* const data, const size_t sz);

/// stop the output
void modulator_stop(void);

