#include "services/io/io_expanders.h"
#include "config.h"
#include "services/io/leds.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

#include <Preferences.h>
#include <Wire.h>

namespace {
    constexpr uint8_t kExternalExpanderInactiveInputs = 0xFF;
    constexpr uint8_t kFaultExpanderInactiveInputs = 0xFF;
    constexpr uint8_t kTca6408aInputPortRegister = 0x00;
    constexpr uint8_t kTca6408aPolarityInversionRegister = 0x02;
    constexpr uint8_t kTca6408aConfigurationRegister = 0x03;
    constexpr const char* kIlluminationFaultReportingEnabledKey = "illum_r";

    bool s_external_expander_connected = false;
    bool s_external_expander_disconnect_latched = false;
    uint8_t s_external_expander_i2c_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
    uint32_t s_next_external_expander_probe_ms = 0;
    bool s_external_expander_input_cache_valid = false;
    uint8_t s_external_expander_input_cache = kExternalExpanderInactiveInputs;

    bool s_fault_expander_connected = false;
    bool s_fault_expander_input_cache_valid = false;
    uint8_t s_fault_expander_input_cache = kFaultExpanderInactiveInputs;
    uint32_t s_next_fault_expander_poll_ms = 0;
    bool s_fault_signal_reported[5] = {};
    Preferences s_fault_configuration_preferences;
    bool s_fault_configuration_preferences_initialized = false;
    bool s_illumination_fault_reporting_enabled_loaded = false;
    bool s_illumination_fault_reporting_enabled = true;

    void invalidate_external_expander_input_cache()
    {
        s_external_expander_input_cache_valid = false;
        s_external_expander_input_cache = kExternalExpanderInactiveInputs;
    }

    bool refresh_external_expander_input_cache()
    {
        s_external_expander_input_cache = remote_pcf.read8();
        const int error_code = remote_pcf.lastError();

        if (error_code != PCF8574_OK)
        {
            invalidate_external_expander_input_cache();
            return false;
        }

        s_external_expander_input_cache_valid = true;
        return true;
    }

    void ensure_fault_configuration_preferences()
    {
        if (!s_fault_configuration_preferences_initialized)
        {
            s_fault_configuration_preferences.begin(config::utilities::FAULT_CONFIGURATION_NAMESPACE, false);
            s_fault_configuration_preferences_initialized = true;
        }

        if (!s_illumination_fault_reporting_enabled_loaded)
        {
            if (s_fault_configuration_preferences.isKey(kIlluminationFaultReportingEnabledKey))
            {
                s_illumination_fault_reporting_enabled = s_fault_configuration_preferences.getBool(
                    kIlluminationFaultReportingEnabledKey,
                    true);
            }
            else
            {
                s_illumination_fault_reporting_enabled = true;
            }

            s_illumination_fault_reporting_enabled_loaded = true;
        }
    }

    bool probe_current_external_expander_connection()
    {
        return remote_pcf.isConnected();
    }

    bool try_begin_external_expander_at_address(uint8_t address)
    {
        return remote_pcf.setAddress(address) && remote_pcf.begin();
    }

    bool try_probe_external_expander_at_address(uint8_t address)
    {
        return remote_pcf.setAddress(address);
    }

    bool try_probe_external_expander()
    {
        const uint8_t primary_external_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
        const uint8_t fallback_external_address = config::pins::external_expander::FALLBACK_I2C_ADDRESS;

        if (try_probe_external_expander_at_address(primary_external_address))
        {
            return true;
        }

        return fallback_external_address != primary_external_address &&
               try_probe_external_expander_at_address(fallback_external_address);
    }

    void latch_external_expander_disconnect()
    {
        if (s_external_expander_disconnect_latched)
        {
            return;
        }

        s_external_expander_connected = false;
        s_external_expander_disconnect_latched = true;
        invalidate_external_expander_input_cache();
        report_error_code(ErrorCode::REMOTE_EXPANDER_DISCONNECTED);
        set_led_state(LedId::ERROR_LED, true);
        LOG("External expander disconnected during runtime. Remote inputs disabled until power cycle.");
    }

    void invalidate_fault_expander_input_cache()
    {
        s_fault_expander_input_cache_valid = false;
        s_fault_expander_input_cache = kFaultExpanderInactiveInputs;
    }

    bool write_fault_expander_register(uint8_t reg, uint8_t value)
    {
        Wire.beginTransmission(config::pins::fault_expander::I2C_ADDRESS);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    bool read_fault_expander_register(uint8_t reg, uint8_t& value)
    {
        Wire.beginTransmission(config::pins::fault_expander::I2C_ADDRESS);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0)
        {
            return false;
        }

        const uint8_t bytes_read = Wire.requestFrom(
            config::pins::fault_expander::I2C_ADDRESS,
            static_cast<uint8_t>(1));
        if (bytes_read != 1 || Wire.available() < 1)
        {
            return false;
        }

        value = Wire.read();
        return true;
    }

    bool probe_fault_expander_connection()
    {
        Wire.beginTransmission(config::pins::fault_expander::I2C_ADDRESS);
        return Wire.endTransmission() == 0;
    }

    bool refresh_fault_expander_input_cache()
    {
        uint8_t inputs = kFaultExpanderInactiveInputs;
        if (!read_fault_expander_register(kTca6408aInputPortRegister, inputs))
        {
            invalidate_fault_expander_input_cache();
            return false;
        }

        s_fault_expander_input_cache = inputs;
        s_fault_expander_input_cache_valid = true;
        return true;
    }

    IoExpanderPin fault_expander_signal_pin(FaultExpanderSignal signal)
    {
        switch (signal)
        {
            case FaultExpanderSignal::RH_MOTOR_FAULT:
                return config::pins::fault_expander::RH_MOTOR_FAULT_PIN;
            case FaultExpanderSignal::LH_MOTOR_FAULT:
                return config::pins::fault_expander::LH_MOTOR_FAULT_PIN;
            case FaultExpanderSignal::RH_SENSE_FAULT:
                return config::pins::fault_expander::RH_SENSE_FAULT_PIN;
            case FaultExpanderSignal::LH_SENSE_FAULT:
                return config::pins::fault_expander::LH_SENSE_FAULT_PIN;
            case FaultExpanderSignal::ILLUMINATION_FAULT:
                return config::pins::fault_expander::ILLUMINATION_FAULT_PIN;
        }

        return IoExpanderPin::PIN_NC;
    }

    uint8_t fault_expander_signal_index(FaultExpanderSignal signal)
    {
        return static_cast<uint8_t>(signal);
    }

    ErrorCode fault_expander_signal_error_code(FaultExpanderSignal signal)
    {
        switch (signal)
        {
            case FaultExpanderSignal::RH_MOTOR_FAULT:
                return ErrorCode::RH_MOTOR_FAULT;
            case FaultExpanderSignal::LH_MOTOR_FAULT:
                return ErrorCode::LH_MOTOR_FAULT;
            case FaultExpanderSignal::RH_SENSE_FAULT:
                return ErrorCode::RH_SENSING_FAULT;
            case FaultExpanderSignal::LH_SENSE_FAULT:
                return ErrorCode::LH_SENSING_FAULT;
            case FaultExpanderSignal::ILLUMINATION_FAULT:
                return ErrorCode::ILLUMINATION_FAULT;
        }

        return ErrorCode::ILLUMINATION_FAULT;
    }

    bool fault_expander_signal_disables_motor_movement(FaultExpanderSignal signal, PopUpId& pop_up_id)
    {
        switch (signal)
        {
            case FaultExpanderSignal::RH_MOTOR_FAULT:
            case FaultExpanderSignal::RH_SENSE_FAULT:
                pop_up_id = PopUpId::RH;
                return true;

            case FaultExpanderSignal::LH_MOTOR_FAULT:
            case FaultExpanderSignal::LH_SENSE_FAULT:
                pop_up_id = PopUpId::LH;
                return true;

            case FaultExpanderSignal::ILLUMINATION_FAULT:
                return false;
        }

        return false;
    }

    constexpr FaultExpanderSignal kFaultSignals[] = {
        FaultExpanderSignal::RH_MOTOR_FAULT,
        FaultExpanderSignal::LH_MOTOR_FAULT,
        FaultExpanderSignal::RH_SENSE_FAULT,
        FaultExpanderSignal::LH_SENSE_FAULT,
        FaultExpanderSignal::ILLUMINATION_FAULT,
    };

    bool fault_expander_signal_active_from_cache(FaultExpanderSignal signal)
    {
        const IoExpanderPin pin = fault_expander_signal_pin(signal);
        if (pin == IoExpanderPin::PIN_NC)
        {
            return false;
        }

        const uint8_t bit = static_cast<uint8_t>(pin);
        const bool pin_state = (s_fault_expander_input_cache & (1u << bit)) != 0u;
        return config::pins::fault_expander::FAULT_INPUT_ACTIVE_LOW ? !pin_state : pin_state;
    }

    void reset_fault_signal_report_state()
    {
        for (bool& reported : s_fault_signal_reported)
        {
            reported = false;
        }
    }

    void setup_fault_expander()
    {
        ensure_fault_configuration_preferences();
        LOG(
            "ILLUMINATION_FAULT reporting: %s.",
            s_illumination_fault_reporting_enabled ? "ENABLED" : "DISABLED");

        if (!config::features::HAS_FAULT_EXPANDER)
        {
            s_fault_expander_connected = false;
            s_next_fault_expander_poll_ms = 0;
            reset_fault_signal_report_state();
            invalidate_fault_expander_input_cache();
            return;
        }

        s_fault_expander_connected = probe_fault_expander_connection();
        s_next_fault_expander_poll_ms = 0;
        reset_fault_signal_report_state();
        invalidate_fault_expander_input_cache();

        LOG(
            "Fault TCA6408A expander at 0x%02X: %s.",
            config::pins::fault_expander::I2C_ADDRESS,
            s_fault_expander_connected ? "present" : "not present");

        if (!s_fault_expander_connected)
        {
            return;
        }

        if (!write_fault_expander_register(kTca6408aPolarityInversionRegister, 0x00))
        {
            s_fault_expander_connected = false;
            LOG("TCA6408A polarity setup failed.");
            return;
        }

        if (!write_fault_expander_register(kTca6408aConfigurationRegister, 0xFF))
        {
            s_fault_expander_connected = false;
            LOG("TCA6408A input configuration failed.");
            return;
        }

        if (config::pins::fault_expander::INTERRUPT_PIN != GPIO_NUM_NC)
        {
            pinMode(config::pins::fault_expander::INTERRUPT_PIN, INPUT_PULLUP);
        }
        (void)refresh_fault_expander_input_cache();
    }
}


// Initiate the helper classes
PCF8574 remote_pcf(config::pins::external_expander::DEFAULT_I2C_ADDRESS);
ADS7138 internal_ads(config::pins::internal_expander::I2C_ADDRESS);

void setup_io_expanders()
{
    const uint8_t primary_external_address = config::pins::external_expander::DEFAULT_I2C_ADDRESS;
    const uint8_t fallback_external_address = config::pins::external_expander::FALLBACK_I2C_ADDRESS;

    bool external_expander_connected = try_begin_external_expander_at_address(primary_external_address);
    if (!external_expander_connected && fallback_external_address != primary_external_address)
    {
        external_expander_connected = try_begin_external_expander_at_address(fallback_external_address);
    }

    s_external_expander_connected = external_expander_connected;
    s_external_expander_disconnect_latched = false;
    s_external_expander_i2c_address = remote_pcf.getAddress();
    s_next_external_expander_probe_ms = 0;
    invalidate_external_expander_input_cache();

    if (external_expander_connected)
    {
        LOG("External expander detected at I2C address 0x%02X.", remote_pcf.getAddress());
    }
    else
    {
        LOG(
            "External expander not detected at I2C addresses 0x%02X or 0x%02X.",
            primary_external_address,
            fallback_external_address);
    }

    // Ugly I know, but setting up pin configurations of the internal ADS7138 here
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::BATTERY_VOLTAGE_PIN      ));
    internal_ads.setAnalogInput(    static_cast<uint8_t> (config::pins::internal_expander::LED_ADJUST_POT_PIN       ));
    if (config::features::HAS_RH_POP_UP_OFFSET_POT)
    {
        internal_ads.setAnalogInput(static_cast<uint8_t>(config::pins::internal_expander::POP_UP_OFFSET_POT_PIN));
    }
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::INPUT_LED_PIN            ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::ERROR_LED_PIN            ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::STATUS_LED_PIN           ), true);
    internal_ads.setDigitalOutput(  static_cast<uint8_t> (config::pins::internal_expander::SLEEPY_EYE_LED_PIN       ), true);
    internal_ads.setDigitalInput(   static_cast<uint8_t> (config::pins::internal_expander::DEBUG_BUTTON_PIN         ));

    // Beginning the IO expanders
    internal_ads.begin();
    setup_fault_expander();
}

void update_external_expander_runtime_state()
{
    if (s_external_expander_disconnect_latched || !are_pop_ups_idle_or_timed_out())
    {
        return;
    }

    const uint32_t now_ms = millis();
    if (now_ms < s_next_external_expander_probe_ms)
    {
        return;
    }

    s_next_external_expander_probe_ms =
        now_ms + config::pins::external_expander::RUNTIME_PROBE_INTERVAL_MS;

    if (s_external_expander_connected)
    {
        if (!probe_current_external_expander_connection())
        {
            latch_external_expander_disconnect();
            return;
        }

        if (refresh_external_expander_input_cache())
        {
            return;
        }

        latch_external_expander_disconnect();
        return;
    }

    if (!try_probe_external_expander())
    {
        return;
    }

    if (!remote_pcf.begin())
    {
        return;
    }

    s_external_expander_connected = true;
    s_external_expander_i2c_address = remote_pcf.getAddress();
    invalidate_external_expander_input_cache();
    (void)refresh_external_expander_input_cache();
    LOG(
        "External expander detected at I2C address 0x%02X during runtime.",
        static_cast<unsigned>(s_external_expander_i2c_address));
}

void update_fault_expander_runtime_state()
{
    if (!config::features::HAS_FAULT_EXPANDER || !s_fault_expander_connected)
    {
        return;
    }

    const uint32_t now_ms = millis();
    if (now_ms < s_next_fault_expander_poll_ms)
    {
        return;
    }

    s_next_fault_expander_poll_ms = now_ms + config::pins::fault_expander::RUNTIME_POLL_INTERVAL_MS;

    if (!refresh_fault_expander_input_cache())
    {
        s_fault_expander_connected = false;
        LOG("Fault TCA6408A input read failed. Fault monitoring disabled.");
        return;
    }

    for (FaultExpanderSignal signal : kFaultSignals)
    {
        const uint8_t index = fault_expander_signal_index(signal);
        const bool active = fault_expander_signal_active_from_cache(signal);
        const bool reporting_disabled =
            signal == FaultExpanderSignal::ILLUMINATION_FAULT &&
            !is_illumination_fault_reporting_enabled();

        if (reporting_disabled)
        {
            s_fault_signal_reported[index] = false;
            continue;
        }

        if (!active)
        {
            s_fault_signal_reported[index] = false;
            continue;
        }

        PopUpId affected_pop_up = PopUpId::RH;
        if (fault_expander_signal_disables_motor_movement(signal, affected_pop_up))
        {
            latch_pop_up_motion_disable(affected_pop_up, fault_expander_signal_name(signal));
        }

        if (s_fault_signal_reported[index])
        {
            continue;
        }

        s_fault_signal_reported[index] = true;
        LOG("%s detected.", fault_expander_signal_name(signal));
        report_error_code(fault_expander_signal_error_code(signal));
        set_led_state(LedId::ERROR_LED, true);
    }
}

bool is_external_expander_connected()
{
    return s_external_expander_connected;
}

uint8_t get_external_expander_i2c_address()
{
    return s_external_expander_i2c_address;
}

bool read_external_expander_pin(IoExpanderPin pin)
{
    if (pin == IoExpanderPin::PIN_NC)
    {
        return true;
    }

    if (!s_external_expander_connected || !s_external_expander_input_cache_valid)
    {
        return true;
    }

    const uint8_t bit = static_cast<uint8_t>(pin);
    return (s_external_expander_input_cache & (1u << bit)) != 0u;
}

bool is_fault_expander_connected()
{
    return s_fault_expander_connected;
}

bool read_fault_expander_pin(IoExpanderPin pin)
{
    if (pin == IoExpanderPin::PIN_NC)
    {
        return true;
    }

    if (!s_fault_expander_connected)
    {
        return true;
    }

    if (!refresh_fault_expander_input_cache())
    {
        s_fault_expander_connected = false;
        LOG("Fault TCA6408A input read failed. Fault-expander inputs disabled.");
        return true;
    }

    const uint8_t bit = static_cast<uint8_t>(pin);
    return (s_fault_expander_input_cache & (1u << bit)) != 0u;
}

bool is_fault_expander_signal_active(FaultExpanderSignal signal)
{
    const bool pin_state = read_fault_expander_pin(fault_expander_signal_pin(signal));
    return config::pins::fault_expander::FAULT_INPUT_ACTIVE_LOW ? !pin_state : pin_state;
}

bool is_illumination_fault_reporting_enabled()
{
    ensure_fault_configuration_preferences();
    return s_illumination_fault_reporting_enabled;
}

bool set_illumination_fault_reporting_enabled(bool enabled)
{
    ensure_fault_configuration_preferences();

    const size_t bytes_written = s_fault_configuration_preferences.putBool(
        kIlluminationFaultReportingEnabledKey,
        enabled);
    if (bytes_written != sizeof(uint8_t))
    {
        return false;
    }

    s_illumination_fault_reporting_enabled = enabled;
    s_illumination_fault_reporting_enabled_loaded = true;
    s_fault_signal_reported[fault_expander_signal_index(FaultExpanderSignal::ILLUMINATION_FAULT)] = false;
    return true;
}

bool reset_illumination_fault_reporting_to_default()
{
    ensure_fault_configuration_preferences();

    if (!s_fault_configuration_preferences.clear())
    {
        return false;
    }

    s_illumination_fault_reporting_enabled = true;
    s_illumination_fault_reporting_enabled_loaded = true;
    s_fault_signal_reported[fault_expander_signal_index(FaultExpanderSignal::ILLUMINATION_FAULT)] = false;
    return true;
}

const char* fault_expander_signal_name(FaultExpanderSignal signal)
{
    switch (signal)
    {
        case FaultExpanderSignal::RH_MOTOR_FAULT:
            return "RH_MOTOR_FAULT";
        case FaultExpanderSignal::LH_MOTOR_FAULT:
            return "LH_MOTOR_FAULT";
        case FaultExpanderSignal::RH_SENSE_FAULT:
            return "RH_SENSE_FAULT";
        case FaultExpanderSignal::LH_SENSE_FAULT:
            return "LH_SENSE_FAULT";
        case FaultExpanderSignal::ILLUMINATION_FAULT:
            return "ILLUMINATION_FAULT";
    }

    return "UNKNOWN_FAULT_SIGNAL";
}

void log_fault_expander_status()
{
    if (!config::features::HAS_FAULT_EXPANDER)
    {
        LOG("Fault expander: Not supported on this board.");
        return;
    }

    if (!s_fault_expander_connected)
    {
        LOG("Fault expander: Not Connected");
        return;
    }

    LOG("Fault expander: Connected");
    LOG(
        "ILLUMINATION_FAULT reporting: %s",
        is_illumination_fault_reporting_enabled() ? "ENABLED" : "DISABLED");

    for (FaultExpanderSignal signal : kFaultSignals)
    {
        const bool active = is_fault_expander_signal_active(signal);
        const bool reporting_disabled =
            signal == FaultExpanderSignal::ILLUMINATION_FAULT &&
            !is_illumination_fault_reporting_enabled();
        if (!active)
        {
            LOG(
                "%s: inactive%s",
                fault_expander_signal_name(signal),
                reporting_disabled ? " (reporting disabled)" : "");
            continue;
        }

        LOG(
            "%s: ACTIVE%s",
            fault_expander_signal_name(signal),
            reporting_disabled ? " (reporting disabled)" : "");
    }
}
