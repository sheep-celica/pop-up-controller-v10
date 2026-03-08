#include "services/logging/error_log_manager.h"
#include <Arduino.h>
#include "config.h"


namespace {
    constexpr const char* KEY_LOGS      = "logs";
    constexpr const char* KEY_INDEX     = "index";
    constexpr const char* KEY_COUNT     = "count";
}

ErrorLogManager::ErrorLogManager(Preferences& prefs)
    : preferences_(prefs),
      write_index_(0),
      count_(0)
{
    reset_runtime_state();
}

void ErrorLogManager::initialize()
{
    preferences_.begin(config::utilities::ERROR_LOG_NAMESPACE, false);
    load_error_log_entries();
}

void ErrorLogManager::reset_runtime_state()
{
    memset(logs_, 0, sizeof(logs_));
    write_index_ = 0;
    count_ = 0;
}

void ErrorLogManager::load_error_log_entries()
{
    reset_runtime_state();

    // Avoid loading partial/corrupted state if metadata/blob keys are missing.
    const bool has_count_key = preferences_.isKey(KEY_COUNT);
    const bool has_index_key = preferences_.isKey(KEY_INDEX);
    const bool has_logs_key = preferences_.isKey(KEY_LOGS);
    if (!has_count_key || !has_index_key || !has_logs_key) {
        return;
    }

    const uint8_t stored_count = preferences_.getUChar(KEY_COUNT, 0);
    const uint8_t stored_index = preferences_.getUChar(KEY_INDEX, 0);

    if (stored_count > MAX_LOGS || stored_index >= MAX_LOGS) {
        reset_runtime_state();
        return;
    }

    size_t expected_size = sizeof(logs_);
    size_t stored_size = preferences_.getBytes(KEY_LOGS, logs_, expected_size);

    if (stored_size != expected_size) {
        reset_runtime_state();
        return;
    }

    count_ = stored_count;
    write_index_ = stored_index;
}

void ErrorLogManager::save_error_log_entries()
{
    // Persist blob first so metadata never points at a not-yet-written blob.
    preferences_.putBytes(KEY_LOGS, logs_, sizeof(logs_));
    preferences_.putUChar(KEY_COUNT, count_);
    preferences_.putUChar(KEY_INDEX, write_index_);
}

void ErrorLogManager::add_error_log_entry(const ErrorLog& log)
{
    if (write_index_ >= MAX_LOGS) {
        write_index_ = 0;
    }
    if (count_ > MAX_LOGS) {
        count_ = 0;
    }

    logs_[write_index_] = log;

    write_index_ = (write_index_ + 1) % MAX_LOGS;

    if (count_ < MAX_LOGS) {
        ++count_;
    }

    save_error_log_entries();
}

void ErrorLogManager::print_error_log_entries() const
{
    Serial.println("---- Error Log ----");

    for (uint8_t i = 0; i < count_; ++i) {
        uint8_t index = (write_index_ + MAX_LOGS - count_ + i) % MAX_LOGS;
        const auto& log = logs_[index];

        Serial.printf(
            "[%lu] Boot=%lu Code=%s Vbat=%u mV Temp=%d.%d C\n",
            log.timestamp_ms,
            log.boot_count,
            error_code_to_string(log.error_code),
            log.battery_voltage_mv,
            log.temperature_decic / 10,
            abs(log.temperature_decic % 10)
        );
    }

    Serial.println("-------------------");
}

void ErrorLogManager::clear_error_log_entries()
{
    preferences_.clear();
    reset_runtime_state();
}

uint8_t ErrorLogManager::get_error_count() const
{
    return count_;
}
