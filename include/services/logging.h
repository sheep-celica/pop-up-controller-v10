#pragma once


void log_message(const char* fmt, ...);
#define LOG(fmt, ...) log_message(fmt, ##__VA_ARGS__)