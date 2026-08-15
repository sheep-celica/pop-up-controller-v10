#include "services/maintenance/shipment_finalization.h"

#include "services/io/io_expanders.h"
#include "services/io/leds.h"
#include "services/io/motors.h"
#include "services/inputs/remote_input_pins.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"
#include "services/io/power.h"

namespace {
    bool s_shipment_finalization_mode = false;
}

bool is_shipment_finalization_mode()
{
    return s_shipment_finalization_mode;
}

bool finalize_for_shipment()
{
    if (s_shipment_finalization_mode)
    {
        return false;
    }

    LOG("Shipment finalization started: clearing manufacturing-test data.");

    if (!error_log_manager.clear_error_log_entries())
    {
        LOG("Shipment finalization failed: could not clear the error log.");
        return false;
    }

    if (!statistics_manager.clear_all_statistics())
    {
        LOG("Shipment finalization failed: could not clear statistical data.");
        return false;
    }

    if (!reset_illumination_fault_reporting_to_default())
    {
        LOG("Shipment finalization failed: could not reset illumination fault reporting.");
        return false;
    }

    if (!reset_remote_input_configuration_to_defaults())
    {
        LOG("Shipment finalization failed: could not reset remote-input configuration.");
        return false;
    }

    if (!reset_pop_up_configuration_to_defaults())
    {
        LOG("Shipment finalization failed: could not reset pop-up configuration.");
        return false;
    }

    if (!reset_power_configuration_to_default())
    {
        LOG("Shipment finalization failed: could not reset power configuration.");
        return false;
    }

    RH_POP_UP.reset_timeout();
    LH_POP_UP.reset_timeout();
    clear_motor_stall_faults();
    set_led_state(LedId::ERROR_LED, false);

    s_shipment_finalization_mode = true;

    LOG("Shipment finalization succeeded.");
    LOG("Preserved: manufacturing data, battery calibration, motor calibration, and pop-up timing calibration.");
    LOG("Cleared: errors, all statistics, and manufacturing-test configuration.");
    LOG("Read-only maintenance mode active. Disconnect power when inspection is complete.");
    return true;
}
