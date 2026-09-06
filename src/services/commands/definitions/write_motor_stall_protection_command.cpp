#include "services/commands/command_definitions.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "services/io/motors.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

namespace {
    char* next_token(char*& cursor)
    {
        if (!cursor) return nullptr;
        while (*cursor == ' ' || *cursor == '\t') ++cursor;
        if (*cursor == '\0') return nullptr;

        char* token = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') ++cursor;
        if (*cursor != '\0') {
            *cursor = '\0';
            ++cursor;
        }
        return token;
    }

    bool parse_bool(const char* token, bool& value)
    {
        if (!token) return false;
        if (strcmp(token, "true") == 0 || strcmp(token, "1") == 0 || strcmp(token, "on") == 0) {
            value = true;
            return true;
        }
        if (strcmp(token, "false") == 0 || strcmp(token, "0") == 0 || strcmp(token, "off") == 0) {
            value = false;
            return true;
        }
        return false;
    }

    bool parse_float(const char* token, float& value)
    {
        if (!token) return false;
        char* end = nullptr;
        errno = 0;
        value = strtof(token, &end);
        return errno == 0 && end != token && *end == '\0' && std::isfinite(value);
    }

    bool parse_uint32(const char* token, uint32_t& value)
    {
        if (!token || token[0] == '-') return false;
        char* end = nullptr;
        errno = 0;
        const unsigned long parsed = strtoul(token, &end, 10);
        if (errno != 0 || end == token || *end != '\0' || parsed > UINT32_MAX) return false;
        value = static_cast<uint32_t>(parsed);
        return true;
    }

    void log_usage()
    {
        LOG("Usage: writeMotorStallProtection <true|false> <current_a> <duration_ms> <startup_blanking_ms>");
    }

    void handle_write_motor_stall_protection_command(char* remaining_args)
    {
        MotorStallProtectionConfig active_config = {};
        if (!get_motor_stall_protection_config(active_config)) {
            LOG("writeMotorStallProtection unsupported on this board.");
            return;
        }

        char* cursor = remaining_args;
        char* enabled_token = next_token(cursor);
        char* current_token = next_token(cursor);
        char* duration_token = next_token(cursor);
        char* blanking_token = next_token(cursor);
        char* extra_token = next_token(cursor);
        if (!enabled_token || !current_token || !duration_token || !blanking_token || extra_token) {
            LOG("writeMotorStallProtection rejected: expected exactly four arguments.");
            log_usage();
            return;
        }

        MotorStallProtectionConfig requested = {};
        if (!parse_bool(enabled_token, requested.enabled) ||
            !parse_float(current_token, requested.current_a) ||
            !parse_uint32(duration_token, requested.duration_ms) ||
            !parse_uint32(blanking_token, requested.startup_blanking_ms)) {
            LOG("writeMotorStallProtection rejected: invalid argument.");
            log_usage();
            return;
        }

        if (requested.current_a < config::motors::drv8243::STALL_MIN_CURRENT_A ||
            requested.current_a > config::motors::drv8243::STALL_MAX_CURRENT_A ||
            requested.duration_ms < config::motors::drv8243::STALL_MIN_DURATION_MS ||
            requested.duration_ms > config::motors::drv8243::STALL_MAX_DURATION_MS ||
            requested.startup_blanking_ms > config::motors::drv8243::STALL_MAX_STARTUP_BLANKING_MS) {
            LOG(
                "writeMotorStallProtection rejected: current must be %.1f-%.1f A, duration must be %lu-%lu ms, and startup blanking must be 0-%lu ms.",
                config::motors::drv8243::STALL_MIN_CURRENT_A,
                config::motors::drv8243::STALL_MAX_CURRENT_A,
                static_cast<unsigned long>(config::motors::drv8243::STALL_MIN_DURATION_MS),
                static_cast<unsigned long>(config::motors::drv8243::STALL_MAX_DURATION_MS),
                static_cast<unsigned long>(config::motors::drv8243::STALL_MAX_STARTUP_BLANKING_MS));
            log_usage();
            return;
        }

        if (!are_pop_ups_idle_or_timed_out()) {
            LOG("writeMotorStallProtection rejected: pop-ups must be idle or timed out.");
            return;
        }

        if (!save_motor_stall_protection_config(requested)) {
            LOG("writeMotorStallProtection failed: could not persist configuration to NVS.");
            return;
        }

        (void)print_motor_stall_protection_config();
    }
}

extern const CommandDefinition kWriteMotorStallProtectionCommandDefinition = {
    "writeMotorStallProtection",
    "writeMotorStallProtection <true|false> <current_a> <duration_ms> <startup_blanking_ms>",
    handle_write_motor_stall_protection_command
};
