#include "services/io/motors.h"

#include <cmath>
#include <Preferences.h>

#include "config.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
namespace {
    Preferences s_motor_calibration_preferences;
    bool s_motor_calibration_preferences_initialized = false;
    Preferences s_motor_stall_preferences;
    bool s_motor_stall_preferences_initialized = false;

    constexpr uint32_t MOTOR_STALL_CONFIG_VERSION = 1;

    struct PersistedMotorStallConfig {
        uint32_t version;
        uint8_t enabled;
        uint8_t reserved[3];
        float current_a;
        uint32_t duration_ms;
        uint32_t startup_blanking_ms;
    };

    struct CurrentTestStartupSample {
        uint32_t time_ms;
        uint16_t raw;
        float current_a;
    };

    struct CurrentTestReport {
        uint32_t sample_time_ms;
        uint16_t sample_raw;
        float sample_current_a;
        float minimum_current_a;
        uint32_t minimum_time_ms;
        float maximum_current_a;
        uint32_t maximum_time_ms;
    };

    constexpr size_t CURRENT_TEST_MAX_REPORTS =
        (config::motors::drv8243::CURRENT_TEST_MAX_DURATION_MS +
            config::motors::drv8243::CURRENT_TEST_REPORT_PERIOD_MS - 1) /
        config::motors::drv8243::CURRENT_TEST_REPORT_PERIOD_MS;
    constexpr size_t CURRENT_TEST_STARTUP_SAMPLE_COUNT =
        config::motors::drv8243::CURRENT_TEST_REPORT_PERIOD_MS /
        config::motors::drv8243::CURRENT_TEST_SAMPLE_PERIOD_MS;
    CurrentTestStartupSample
        s_current_test_startup_samples[CURRENT_TEST_STARTUP_SAMPLE_COUNT];
    CurrentTestReport s_current_test_reports[CURRENT_TEST_MAX_REPORTS];

    void ensure_motor_calibration_preferences()
    {
        if (!s_motor_calibration_preferences_initialized) {
            s_motor_calibration_preferences.begin(
                config::motors::drv8243::CURRENT_CALIBRATION_NAMESPACE,
                false);
            s_motor_calibration_preferences_initialized = true;
        }
    }

    void ensure_motor_stall_preferences()
    {
        if (!s_motor_stall_preferences_initialized) {
            s_motor_stall_preferences.begin(
                config::motors::drv8243::STALL_CONFIG_NAMESPACE,
                false);
            s_motor_stall_preferences_initialized = true;
        }
    }

    bool is_valid_stall_config(const MotorStallProtectionConfig& stall_config)
    {
        return std::isfinite(stall_config.current_a) &&
            stall_config.current_a >= config::motors::drv8243::STALL_MIN_CURRENT_A &&
            stall_config.current_a <= config::motors::drv8243::STALL_MAX_CURRENT_A &&
            stall_config.duration_ms >= config::motors::drv8243::STALL_MIN_DURATION_MS &&
            stall_config.duration_ms <= config::motors::drv8243::STALL_MAX_DURATION_MS &&
            stall_config.startup_blanking_ms <=
                config::motors::drv8243::STALL_MAX_STARTUP_BLANKING_MS;
    }

    void apply_stall_config(const MotorStallProtectionConfig& stall_config)
    {
        RH_DRV8243_MOTOR.set_stall_config(
            stall_config.current_a,
            stall_config.duration_ms,
            stall_config.startup_blanking_ms);
        LH_DRV8243_MOTOR.set_stall_config(
            stall_config.current_a,
            stall_config.duration_ms,
            stall_config.startup_blanking_ms);
        RH_DRV8243_MOTOR.set_stall_protection_enabled(stall_config.enabled);
        LH_DRV8243_MOTOR.set_stall_protection_enabled(stall_config.enabled);
    }

    MotorStallProtectionConfig default_stall_config()
    {
        return {
            config::motors::drv8243::STALL_PROTECTION_ENABLED,
            config::motors::drv8243::STALL_CURRENT_A,
            config::motors::drv8243::STALL_DURATION_MS,
            config::motors::drv8243::STALL_STARTUP_BLANKING_MS,
        };
    }

    MotorStallProtectionConfig load_stall_config()
    {
        ensure_motor_stall_preferences();
        PersistedMotorStallConfig persisted = {};
        const size_t bytes_read = s_motor_stall_preferences.getBytes(
            config::motors::drv8243::STALL_CONFIG_KEY,
            &persisted,
            sizeof(persisted));

        MotorStallProtectionConfig loaded = {
            persisted.enabled != 0,
            persisted.current_a,
            persisted.duration_ms,
            persisted.startup_blanking_ms,
        };
        if (bytes_read != sizeof(persisted) ||
            persisted.version != MOTOR_STALL_CONFIG_VERSION ||
            persisted.enabled > 1 ||
            !is_valid_stall_config(loaded)) {
            loaded = default_stall_config();
        }
        return loaded;
    }

    void load_current_calibration(
        DRV8243& motor,
        const char* name,
        const char* scale_key,
        const char* offset_key)
    {
        ensure_motor_calibration_preferences();
        const float scale = s_motor_calibration_preferences.getFloat(scale_key, 1.0f);
        const float offset = s_motor_calibration_preferences.getFloat(offset_key, 0.0f);
        motor.set_current_calibration(scale, offset);
        LOG(
            "%s motor current calibration loaded: scale=%.6f offset=%.4f A.",
            name,
            scale,
            offset);
    }

    bool save_current_calibration(
        DRV8243& motor,
        const char* name,
        const char* scale_key,
        const char* offset_key,
        float scale,
        float offset_a)
    {
        ensure_motor_calibration_preferences();
        const size_t scale_bytes = s_motor_calibration_preferences.putFloat(
            scale_key,
            scale);
        const size_t offset_bytes = s_motor_calibration_preferences.putFloat(
            offset_key,
            offset_a);
        const bool ok = scale_bytes == sizeof(float) && offset_bytes == sizeof(float);
        if (ok) {
            motor.set_current_calibration(scale, offset_a);
        }
        LOG(
            "%s motor current calibration %s: scale=%.6f offset=%.4f A.",
            name,
            ok ? "saved" : "save failed",
            scale,
            offset_a);
        return ok;
    }

    bool calibrate_one_motor(
        DRV8243& motor,
        const char* name,
        uint32_t duration_ms)
    {
        LOG(
            "%s motor current measurement started: duration=%lu ms.",
            name,
            static_cast<unsigned long>(duration_ms));

        const bool stall_protection_was_enabled = motor.stall_protection_enabled();
        motor.coast();
        motor.set_stall_protection_enabled(false);
        delay(config::motors::drv8243::CALIBRATION_SETTLE_MS);

        float zero_current_sum = 0.0f;
        constexpr uint8_t zero_sample_count = 5;
        for (uint8_t i = 0; i < zero_sample_count; ++i) {
            zero_current_sum += motor.read_current_a_uncalibrated();
            delay(10);
        }
        const float zero_current_a = zero_current_sum / zero_sample_count;

        motor.run(100.0f);
        // Allow the output and load current to settle before the first active sample.
        delay(config::motors::drv8243::CALIBRATION_SAMPLE_PERIOD_MS);

        float active_current_sum = 0.0f;
        uint32_t sample_count = 0;
        const uint32_t start_ms = millis();
        while (static_cast<uint32_t>(millis() - start_ms) <
               duration_ms) {
            const uint16_t raw = motor.read_current_raw();
            const float uncalibrated_current_a = motor.read_current_a_uncalibrated();
            active_current_sum += uncalibrated_current_a;
            ++sample_count;

            LOG(
                "%s motor calibration sample: t=%lu ms raw=%u uncalibrated=%.3f A.",
                name,
                static_cast<unsigned long>(millis() - start_ms),
                raw,
                uncalibrated_current_a);
            delay(config::motors::drv8243::CALIBRATION_SAMPLE_PERIOD_MS);
        }

        motor.coast();
        motor.set_stall_protection_enabled(stall_protection_was_enabled);

        if (sample_count == 0) {
            LOG("MOTOR_CAL_RESULT motor=%s status=failed reason=no_samples", name);
            return false;
        }

        const float active_current_a = active_current_sum / sample_count;
        const float measured_delta_a = active_current_a - zero_current_a;
        if (measured_delta_a <= 0.1f) {
            LOG(
                "MOTOR_CAL_RESULT motor=%s status=failed reason=signal_too_small zero_a=%.6f active_a=%.6f delta_a=%.6f samples=%lu",
                name,
                zero_current_a,
                active_current_a,
                measured_delta_a,
                static_cast<unsigned long>(sample_count));
            LOG(
                "%s motor current measurement failed: measured change %.3f A is too small.",
                name,
                measured_delta_a);
            return false;
        }

        LOG(
            "MOTOR_CAL_RESULT motor=%s status=ok duration_ms=%lu zero_a=%.6f active_a=%.6f delta_a=%.6f samples=%lu",
            name,
            static_cast<unsigned long>(duration_ms),
            zero_current_a,
            active_current_a,
            measured_delta_a,
            static_cast<unsigned long>(sample_count));
        LOG(
            "%s motor current measurement complete: zero=%.3f A active=%.3f A delta=%.3f A.",
            name,
            zero_current_a,
            active_current_a,
            measured_delta_a);
        return true;
    }

    bool test_one_motor_current(
        DRV8243& motor,
        const char* name,
        PopUpId pop_up_id,
        uint32_t duration_ms)
    {
        LOG(
            "%s motor current test started: duration=%lu ms.",
            name,
            static_cast<unsigned long>(duration_ms));

        motor.coast();
        delay(config::motors::drv8243::CURRENT_TEST_REPORT_PERIOD_MS);
        motor.run(100.0f);

        double current_sum_a = 0.0;
        float minimum_current_a = 0.0f;
        float maximum_current_a = 0.0f;
        uint32_t minimum_time_ms = 0;
        uint32_t maximum_time_ms = 0;
        uint32_t sample_count = 0;
        size_t startup_sample_count = 0;
        size_t report_count = 0;
        bool overcurrent = false;
        const uint32_t start_us = micros();
        uint32_t next_sample_us = start_us;
        const uint32_t duration_us = duration_ms * 1000u;

        while (static_cast<uint32_t>(micros() - start_us) < duration_us) {
            const int32_t wait_us = static_cast<int32_t>(next_sample_us - micros());
            if (wait_us > 0) {
                delayMicroseconds(static_cast<uint32_t>(wait_us));
            }

            const uint32_t sample_time_us = static_cast<uint32_t>(micros() - start_us);
            if (sample_time_us >= duration_us) {
                break;
            }
            const uint32_t sample_time_ms = sample_time_us / 1000u;
            const uint32_t now_ms = millis();
            motor.update(now_ms);
            if (motor.consume_stall_fault()) {
                report_pop_up_overcurrent(pop_up_id);
                latch_pop_up_motion_disable(pop_up_id, "MOTOR_CURRENT_TEST_OVERCURRENT");
                overcurrent = true;
                break;
            }

            const uint16_t raw = motor.read_current_raw();
            const float current_a = motor.current_a_from_raw(raw);
            if (sample_count == 0) {
                minimum_current_a = current_a;
                maximum_current_a = current_a;
                minimum_time_ms = sample_time_ms;
                maximum_time_ms = sample_time_ms;
            } else {
                if (current_a < minimum_current_a) {
                    minimum_current_a = current_a;
                    minimum_time_ms = sample_time_ms;
                }
                if (current_a > maximum_current_a) {
                    maximum_current_a = current_a;
                    maximum_time_ms = sample_time_ms;
                }
            }
            current_sum_a += current_a;
            ++sample_count;

            if (sample_time_ms <
                config::motors::drv8243::CURRENT_TEST_REPORT_PERIOD_MS) {
                if (startup_sample_count < CURRENT_TEST_STARTUP_SAMPLE_COUNT) {
                    CurrentTestStartupSample& startup_sample =
                        s_current_test_startup_samples[startup_sample_count];
                    startup_sample.time_ms = sample_time_ms;
                    startup_sample.raw = raw;
                    startup_sample.current_a = current_a;
                    ++startup_sample_count;
                }
            } else {
                const size_t report_index = sample_time_ms /
                    config::motors::drv8243::CURRENT_TEST_REPORT_PERIOD_MS;
                const bool starts_new_report = report_count == 0 ||
                    report_index !=
                        (s_current_test_reports[report_count - 1].sample_time_ms /
                            config::motors::drv8243::CURRENT_TEST_REPORT_PERIOD_MS);
                if (starts_new_report && report_count < CURRENT_TEST_MAX_REPORTS) {
                    CurrentTestReport& report = s_current_test_reports[report_count];
                    report.sample_time_ms = sample_time_ms;
                    report.sample_raw = raw;
                    report.sample_current_a = current_a;
                    report.minimum_current_a = current_a;
                    report.minimum_time_ms = sample_time_ms;
                    report.maximum_current_a = current_a;
                    report.maximum_time_ms = sample_time_ms;
                    ++report_count;
                } else if (report_count > 0) {
                    CurrentTestReport& report = s_current_test_reports[report_count - 1];
                    if (current_a < report.minimum_current_a) {
                        report.minimum_current_a = current_a;
                        report.minimum_time_ms = sample_time_ms;
                    }
                    if (current_a > report.maximum_current_a) {
                        report.maximum_current_a = current_a;
                        report.maximum_time_ms = sample_time_ms;
                    }
                }
            }

            next_sample_us +=
                config::motors::drv8243::CURRENT_TEST_SAMPLE_PERIOD_MS * 1000u;
            yield();
        }

        motor.coast();

        for (size_t i = 0; i < startup_sample_count; ++i) {
            const CurrentTestStartupSample& sample = s_current_test_startup_samples[i];
            LOG(
                "MOTOR_CURRENT_TEST_SAMPLE motor=%s t_ms=%lu raw=%u current_a=%.3f",
                name,
                static_cast<unsigned long>(sample.time_ms),
                sample.raw,
                sample.current_a);
        }

        for (size_t i = 0; i < report_count; ++i) {
            const CurrentTestReport& report = s_current_test_reports[i];
            LOG(
                "MOTOR_CURRENT_TEST_SAMPLE motor=%s t_ms=%lu raw=%u current_a=%.3f window_min_a=%.3f window_min_t_ms=%lu window_max_a=%.3f window_max_t_ms=%lu",
                name,
                static_cast<unsigned long>(report.sample_time_ms),
                report.sample_raw,
                report.sample_current_a,
                report.minimum_current_a,
                static_cast<unsigned long>(report.minimum_time_ms),
                report.maximum_current_a,
                static_cast<unsigned long>(report.maximum_time_ms));
        }

        if (overcurrent) {
            if (sample_count == 0) {
                LOG(
                    "MOTOR_CURRENT_TEST_RESULT motor=%s status=overcurrent duration_ms=%lu samples=0",
                    name,
                    static_cast<unsigned long>(duration_ms));
            } else {
                LOG(
                    "MOTOR_CURRENT_TEST_RESULT motor=%s status=overcurrent duration_ms=%lu samples=%lu average_a=%.3f min_a=%.3f min_t_ms=%lu max_a=%.3f max_t_ms=%lu",
                    name,
                    static_cast<unsigned long>(duration_ms),
                    static_cast<unsigned long>(sample_count),
                    static_cast<float>(current_sum_a / sample_count),
                    minimum_current_a,
                    static_cast<unsigned long>(minimum_time_ms),
                    maximum_current_a,
                    static_cast<unsigned long>(maximum_time_ms));
            }
            return false;
        }

        const float average_current_a = static_cast<float>(current_sum_a / sample_count);
        LOG(
            "MOTOR_CURRENT_TEST_RESULT motor=%s status=ok duration_ms=%lu samples=%lu average_a=%.3f min_a=%.3f min_t_ms=%lu max_a=%.3f max_t_ms=%lu",
            name,
            static_cast<unsigned long>(duration_ms),
            static_cast<unsigned long>(sample_count),
            average_current_a,
            minimum_current_a,
            static_cast<unsigned long>(minimum_time_ms),
            maximum_current_a,
            static_cast<unsigned long>(maximum_time_ms));
        LOG(
            "%s motor current test complete: average=%.3f A min=%.3f A max=%.3f A.",
            name,
            average_current_a,
            minimum_current_a,
            maximum_current_a);
        return true;
    }
}

namespace {
    DRV8243::Config make_motor_config(
        gpio_num_t control_pin,
        gpio_num_t coast_disable_pin,
        gpio_num_t current_pin,
        uint8_t ledc_channel)
    {
        return {
            control_pin,
            coast_disable_pin,
            current_pin,
            config::pins::motor_driver::SHARED_SLEEP_PIN,
            ledc_channel,
            config::motors::drv8243::PWM_FREQUENCY_HZ,
            config::motors::drv8243::PWM_RESOLUTION_BITS,
            config::motors::drv8243::NSLEEP_RESET_PULSE_US,
            config::motors::drv8243::DRIVER_READY_DELAY_MS,
            config::motors::drv8243::ADC_REFERENCE_V,
            config::motors::drv8243::ADC_MAX_RAW,
            config::motors::drv8243::IPROPI_SCALING_A_PER_A,
            config::motors::drv8243::IPROPI_TO_GND_OHMS,
            config::motors::drv8243::IPROPI_TO_ADC_OHMS,
            config::motors::drv8243::ADC_TO_GND_OHMS,
            config::motors::drv8243::SAFE_START_ENABLED,
            config::motors::drv8243::SAFE_START_DUTY_PERCENT,
            config::motors::drv8243::SAFE_START_DURATION_MS,
            config::motors::drv8243::STALL_CURRENT_A,
            config::motors::drv8243::STALL_DURATION_MS,
            config::motors::drv8243::STALL_STARTUP_BLANKING_MS,
        };
    }
}

DRV8243 RH_DRV8243_MOTOR(make_motor_config(
    config::pins::RH_MOTOR_ON_PIN,       // DRV8243 ON/BRAKE control pin.
    config::pins::RH_MOTOR_BRAKE_PIN,    // Revision E coast/disable pin; legacy config name kept for now.
    config::pins::RH_CURRENT,
    config::motors::drv8243::LEDC_CHANNEL_RH));

DRV8243 LH_DRV8243_MOTOR(make_motor_config(
    config::pins::LH_MOTOR_ON_PIN,       // DRV8243 ON/BRAKE control pin.
    config::pins::LH_MOTOR_BRAKE_PIN,    // Revision E coast/disable pin; legacy config name kept for now.
    config::pins::LH_CURRENT,
    config::motors::drv8243::LEDC_CHANNEL_LH));
#else
MotorController RH_MOTOR(
    static_cast<int>(config::pins::RH_MOTOR_ON_PIN),
    config::pop_up::ACTIVE_LOW_DRIVE,
    config::pop_up::braking::DEAD_TIME_MS,
    static_cast<int>(config::pins::RH_MOTOR_BRAKE_PIN),
    config::pop_up::braking::LEDC_CHANNEL_RH,
    config::pop_up::braking::FREQUENCY_HZ,
    config::pop_up::braking::PWM_RESOLUTION_BITS,
    config::pop_up::braking::TARGET_DUTY_CYCLE_RATIO,
    config::pop_up::braking::BRAKING_TIME_US,
    config::pop_up::braking::STEP_PERIOD_US,
    config::pop_up::braking::HOLD_TIME_MS);

MotorController LH_MOTOR(
    static_cast<int>(config::pins::LH_MOTOR_ON_PIN),
    config::pop_up::ACTIVE_LOW_DRIVE,
    config::pop_up::braking::DEAD_TIME_MS,
    static_cast<int>(config::pins::LH_MOTOR_BRAKE_PIN),
    config::pop_up::braking::LEDC_CHANNEL_LH,
    config::pop_up::braking::FREQUENCY_HZ,
    config::pop_up::braking::PWM_RESOLUTION_BITS,
    config::pop_up::braking::TARGET_DUTY_CYCLE_RATIO,
    config::pop_up::braking::BRAKING_TIME_US,
    config::pop_up::braking::STEP_PERIOD_US,
    config::pop_up::braking::HOLD_TIME_MS);
#endif

bool setup_motors()
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    const bool rh_ok = RH_DRV8243_MOTOR.begin();
    const bool lh_ok = LH_DRV8243_MOTOR.begin();
    if (rh_ok && lh_ok) {
        const MotorStallProtectionConfig stall_config = load_stall_config();
        apply_stall_config(stall_config);
        // The shared DRV8243 fault outputs can read active until the driver
        // sees its first nSLEEP wake/reset pulse. Prime both drivers here so
        // startup fault polling reflects real hardware faults instead of the
        // pre-wake state.
        RH_DRV8243_MOTOR.wake_up_impulse();
        load_current_calibration(
            RH_DRV8243_MOTOR,
            "RH",
            config::motors::drv8243::RH_CURRENT_SCALE_KEY,
            config::motors::drv8243::RH_CURRENT_OFFSET_KEY);
        load_current_calibration(
            LH_DRV8243_MOTOR,
            "LH",
            config::motors::drv8243::LH_CURRENT_SCALE_KEY,
            config::motors::drv8243::LH_CURRENT_OFFSET_KEY);
        LOG(
            "Revision E DRV8243 motor drivers initialized. Firmware stall protection=%u current=%.3f A duration=%lu ms startup_blanking=%lu ms.",
            stall_config.enabled ? 1u : 0u,
            stall_config.current_a,
            static_cast<unsigned long>(stall_config.duration_ms),
            static_cast<unsigned long>(stall_config.startup_blanking_ms));
    } else {
        LOG("Revision E DRV8243 motor driver initialization failed. RH=%u LH=%u.", rh_ok ? 1u : 0u, lh_ok ? 1u : 0u);
    }
    return rh_ok && lh_ok;
#else
    const bool rh_ok = RH_MOTOR.begin();
    const bool lh_ok = LH_MOTOR.begin();
    return rh_ok && lh_ok;
#endif
}

void update_motors()
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    const uint32_t now_ms = millis();
    RH_DRV8243_MOTOR.update(now_ms);
    LH_DRV8243_MOTOR.update(now_ms);

    if (RH_DRV8243_MOTOR.consume_stall_fault()) {
        report_pop_up_overcurrent(PopUpId::RH);
        latch_pop_up_motion_disable(PopUpId::RH, "RH_POP_UP_OVERCURRENT");
    }
    if (LH_DRV8243_MOTOR.consume_stall_fault()) {
        report_pop_up_overcurrent(PopUpId::LH);
        latch_pop_up_motion_disable(PopUpId::LH, "LH_POP_UP_OVERCURRENT");
    }
#endif
}

void clear_motor_stall_faults()
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    RH_DRV8243_MOTOR.clear_stall_fault();
    LH_DRV8243_MOTOR.clear_stall_fault();
#endif
}

bool get_motor_stall_protection_config(MotorStallProtectionConfig& stall_config)
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    stall_config = {
        RH_DRV8243_MOTOR.stall_protection_enabled(),
        RH_DRV8243_MOTOR.stall_current_a(),
        RH_DRV8243_MOTOR.stall_duration_ms(),
        RH_DRV8243_MOTOR.stall_startup_blanking_ms(),
    };
    return true;
#else
    (void)stall_config;
    return false;
#endif
}

bool save_motor_stall_protection_config(const MotorStallProtectionConfig& stall_config)
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    if (!is_valid_stall_config(stall_config)) {
        return false;
    }

    const PersistedMotorStallConfig persisted = {
        MOTOR_STALL_CONFIG_VERSION,
        static_cast<uint8_t>(stall_config.enabled ? 1 : 0),
        { 0, 0, 0 },
        stall_config.current_a,
        stall_config.duration_ms,
        stall_config.startup_blanking_ms,
    };
    ensure_motor_stall_preferences();
    const size_t bytes_written = s_motor_stall_preferences.putBytes(
        config::motors::drv8243::STALL_CONFIG_KEY,
        &persisted,
        sizeof(persisted));
    if (bytes_written != sizeof(persisted)) {
        return false;
    }

    apply_stall_config(stall_config);
    return true;
#else
    (void)stall_config;
    return false;
#endif
}

bool print_motor_stall_protection_config()
{
    MotorStallProtectionConfig stall_config = {};
    if (!get_motor_stall_protection_config(stall_config)) {
        LOG("MOTOR_STALL_CONFIG status=unsupported supported=false");
        return false;
    }

    LOG(
        "MOTOR_STALL_CONFIG status=ok supported=true enabled=%s current_a=%.3f duration_ms=%lu startup_blanking_ms=%lu",
        stall_config.enabled ? "true" : "false",
        stall_config.current_a,
        static_cast<unsigned long>(stall_config.duration_ms),
        static_cast<unsigned long>(stall_config.startup_blanking_ms));
    return true;
}

bool calibrate_motor_current(MotorCalibrationScope scope, uint32_t duration_ms)
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    bool ok = true;
    if (scope == MotorCalibrationScope::RH || scope == MotorCalibrationScope::BOTH) {
        ok = calibrate_one_motor(
            RH_DRV8243_MOTOR,
            "RH",
            duration_ms) && ok;
    }
    if (scope == MotorCalibrationScope::LH || scope == MotorCalibrationScope::BOTH) {
        ok = calibrate_one_motor(
            LH_DRV8243_MOTOR,
            "LH",
            duration_ms) && ok;
    }
    LOG("Motor current measurement %s.", ok ? "completed successfully" : "completed with errors");
    return ok;
#else
    (void)scope;
    (void)duration_ms;
    LOG("Motor current calibration is unsupported on this board.");
    return false;
#endif
}

bool save_motor_current_calibration(
    MotorCalibrationScope scope,
    float scale,
    float offset_a)
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    if (scope == MotorCalibrationScope::RH) {
        return save_current_calibration(
            RH_DRV8243_MOTOR,
            "RH",
            config::motors::drv8243::RH_CURRENT_SCALE_KEY,
            config::motors::drv8243::RH_CURRENT_OFFSET_KEY,
            scale,
            offset_a);
    }

    if (scope == MotorCalibrationScope::LH) {
        return save_current_calibration(
            LH_DRV8243_MOTOR,
            "LH",
            config::motors::drv8243::LH_CURRENT_SCALE_KEY,
            config::motors::drv8243::LH_CURRENT_OFFSET_KEY,
            scale,
            offset_a);
    }

    LOG("Motor current calibration save rejected: BOTH is not valid for this command.");
    return false;
#else
    (void)scope;
    (void)scale;
    (void)offset_a;
    LOG("Motor current calibration is unsupported on this board.");
    return false;
#endif
}

bool print_motor_current_calibration()
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    LOG(
        "MOTOR_CAL_CONFIG motor=RH status=ok scale=%.6f offset_a=%.6f",
        RH_DRV8243_MOTOR.current_calibration_scale(),
        RH_DRV8243_MOTOR.current_calibration_offset_a());
    LOG(
        "MOTOR_CAL_CONFIG motor=LH status=ok scale=%.6f offset_a=%.6f",
        LH_DRV8243_MOTOR.current_calibration_scale(),
        LH_DRV8243_MOTOR.current_calibration_offset_a());
    return true;
#else
    LOG("MOTOR_CAL_CONFIG status=unsupported board=RevisionC");
    return false;
#endif
}

bool test_motor_current(uint32_t duration_ms)
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    bool ok = test_one_motor_current(
        RH_DRV8243_MOTOR,
        "RH",
        PopUpId::RH,
        duration_ms);
    ok = test_one_motor_current(
        LH_DRV8243_MOTOR,
        "LH",
        PopUpId::LH,
        duration_ms) && ok;
    LOG("Motor current test %s.", ok ? "completed successfully" : "completed with errors");
    return ok;
#else
    (void)duration_ms;
    LOG("testMotorCurrent unsupported on Revision C: no current-readout path is implemented for the legacy motor controller.");
    return false;
#endif
}

void prepare_motors_for_sleep()
{
#if POPUP_CONTROLLER_BOARD_USES_DRV8243_MOTOR_DRIVER
    RH_DRV8243_MOTOR.disable();
    LH_DRV8243_MOTOR.disable();
#else
    RH_MOTOR.set_coast();
    LH_MOTOR.set_coast();
#endif
}
