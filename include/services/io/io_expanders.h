#pragma once
#include "helpers/ADS7138.h"
#include "PCF8574.h"


// Declared classes
extern PCF8574 remote_pcf;
extern ADS7138 internal_ads;


// Public functions
void setup_io_expanders();

