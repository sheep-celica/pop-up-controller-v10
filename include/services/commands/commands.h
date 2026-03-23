#pragma once

#include <cstdint>

using PendingCommandLineHandler = void (*)(char* line, bool timed_out);

// Poll serial input, parse complete command lines, and dispatch command handlers.
// Intentionally not called automatically from main loop so callers can decide
// when serial command handling is safe.
void update_commands();

// Reserve the next complete serial line for a follow-up workflow such as a confirmation prompt.
// Returns false if another follow-up line is already pending or the request is invalid.
bool request_next_command_line(PendingCommandLineHandler handler, uint32_t timeout_ms);
