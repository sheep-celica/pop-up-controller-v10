#pragma once

#include <Arduino.h>

#include "services/io/types/io_expander_pin.h"

bool setup_io_expander_test();
void write_internal_expander_led(IoExpanderPin pin, bool on);
void blink_internal_expander_led(IoExpanderPin pin, uint32_t duration_ms);
