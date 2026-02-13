#pragma once
#include "services/logging/error_log_manager.h"


extern ErrorLogManager error_log_manager;

void report_error_code(ErrorCode);
void log_message(const char* fmt, ...);

#define LOG(fmt, ...) log_message(fmt, ##__VA_ARGS__)
