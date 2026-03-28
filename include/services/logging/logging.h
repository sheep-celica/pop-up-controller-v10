#pragma once
#include "services/logging/error_log_manager.h"
#include "services/logging/statistics_manager.h"

enum class PopUpId : uint8_t;

extern ErrorLogManager error_log_manager;
extern StatisticsManager statistics_manager;

// Initialize logging system (must be called from setup())
void initialize_logging();
void initialize_logging(const char* build_version, const char* build_timestamp);
void log_startup_summary(float battery_voltage, bool bench_mode_enabled);

void report_error_code(ErrorCode);
void report_pop_up_timeout(PopUpId pop_up_id);
void report_pop_up_overcurrent(PopUpId pop_up_id);
const char* get_current_build_version();
const char* get_current_build_timestamp();
bool save_manufacture_data_once(
    const char* manufacture_date,
    const char* serial_number,
    const char* board_serial,
    const char* board_revision,
    const char* car_model);
void print_manufacture_data();
bool has_error_reported_since_boot();
void log_message(const char* fmt, ...);

#define LOG(fmt, ...) log_message(fmt, ##__VA_ARGS__)
