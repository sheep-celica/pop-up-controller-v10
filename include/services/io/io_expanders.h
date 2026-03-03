#pragma once
#include <cstdint>

#include "helpers/ADS7138.h"
#include "PCF8574.h"


// Declared classes
extern PCF8574 remote_pcf;
extern ADS7138 internal_ads;


// Public functions
bool is_valid_external_expander_i2c_address(uint8_t address);
uint8_t get_external_expander_i2c_address();
bool set_external_expander_i2c_address(uint8_t address);
void setup_io_expanders();

