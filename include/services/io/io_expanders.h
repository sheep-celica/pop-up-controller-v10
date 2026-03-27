#pragma once

#include "helpers/ADS7138.h"
#include "PCF8574.h"
#include "services/io/types/io_expander_pin.h"


// Declared classes
extern PCF8574 remote_pcf;
extern ADS7138 internal_ads;


// Public functions
void setup_io_expanders();
void update_external_expander_runtime_state();
bool is_external_expander_connected();
uint8_t get_external_expander_i2c_address();
bool read_external_expander_pin(IoExpanderPin pin);

