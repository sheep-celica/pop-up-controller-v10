#pragma once

#include "helpers/ADS7138.h"
#include "PCF8574.h"
#include "services/io/types/io_expander_pin.h"

enum class FaultExpanderSignal : uint8_t
{
    RH_MOTOR_FAULT,
    LH_MOTOR_FAULT,
    RH_SENSE_FAULT,
    LH_SENSE_FAULT,
    ILLUMINATION_FAULT
};

// Declared classes
extern PCF8574 remote_pcf;
extern ADS7138 internal_ads;


// Public functions
void setup_io_expanders();
void update_external_expander_runtime_state();
void update_fault_expander_runtime_state();
bool is_external_expander_connected();
uint8_t get_external_expander_i2c_address();
bool read_external_expander_pin(IoExpanderPin pin);
bool is_fault_expander_connected();
bool read_fault_expander_pin(IoExpanderPin pin);
bool is_fault_expander_signal_active(FaultExpanderSignal signal);
bool is_illumination_fault_reporting_enabled();
bool set_illumination_fault_reporting_enabled(bool enabled);
const char* fault_expander_signal_name(FaultExpanderSignal signal);
void log_fault_expander_status();
