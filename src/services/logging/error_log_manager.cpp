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
    // On first boot, the error log blob may not exist yet. 
    // Check metadata first to avoid triggering NVS errors.
    count_ = preferences_.getUChar(KEY_COUNT, 0);
    write_index_ = preferences_.getUChar(KEY_INDEX, 0);

    if (count_ == 0 && write_index_ == 0) {
        // First boot or empty log, skip loading blob to avoid NVS errors
        reset_runtime_state();
        return;
    }

    if (count_ > MAX_LOGS) {
        reset_runtime_state();
        return;
    }

    size_t expected_size = sizeof(logs_);
    size_t stored_size = preferences_.getBytes(KEY_LOGS, logs_, expected_size);

    if (stored_size != expected_size) {
        reset_runtime_state();
    }
}

void ErrorLogManager::save_error_log_entries()
{
    preferences_.putUChar(KEY_COUNT, count_);
    preferences_.putUChar(KEY_INDEX, write_index_);
    preferences_.putBytes(KEY_LOGS, logs_, sizeof(logs_));
}

void ErrorLogManager::add_error_log_entry(const ErrorLog& log)
{
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
