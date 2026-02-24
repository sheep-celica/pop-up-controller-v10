#pragma once
#include "services/logging/error_log_manager.h"
#include "services/logging/statistics_manager.h"

enum class PopUpId : uint8_t;

extern ErrorLogManager error_log_manager;
extern StatisticsManager statistics_manager;

// Initialize logging system (must be called from setup())
void initialize_logging();

void report_error_code(ErrorCode);
void report_pop_up_timeout(PopUpId pop_up_id);
void report_pop_up_overcurrent(PopUpId pop_up_id);
void log_message(const char* fmt, ...);

#define LOG(fmt, ...) log_message(fmt, ##__VA_ARGS__)
