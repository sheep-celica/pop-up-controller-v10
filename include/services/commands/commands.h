#pragma once

// Poll serial input, parse complete command lines, and dispatch command handlers.
// Intentionally not called automatically from main loop so callers can decide
// when serial command handling is safe.
void update_commands();

