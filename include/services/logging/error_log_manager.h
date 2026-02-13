#pragma once

#include <Preferences.h>
#include "services/logging/error_log.h"


class ErrorLogManager
{
public:
    explicit ErrorLogManager(Preferences& prefs);

    // Initialize NVS preferences (must be called from setup(), not global init)
    void initialize();

    void load_error_log_entries();
    void save_error_log_entries();

    void add_error_log_entry(const ErrorLog& log);
    void print_error_log_entries() const;
    void clear_error_log_entries();

private:
    static constexpr uint8_t MAX_LOGS = 64;

    Preferences& preferences_;

    ErrorLog logs_[MAX_LOGS];
    uint8_t write_index_;
    uint8_t count_;

    void reset_runtime_state();
};
