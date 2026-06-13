#include "services/commands/command_definitions.h"

#include <cstring>

#include "services/io/io_expanders.h"
#include "services/logging/logging.h"

namespace {
    bool is_space_char(char c)
    {
        return c == ' ' || c == '\t';
    }

    char* next_token(char*& cursor)
    {
        if (!cursor) return nullptr;

        while (*cursor != '\0' && is_space_char(*cursor)) {
            ++cursor;
        }

        if (*cursor == '\0') {
            return nullptr;
        }

        char* token = cursor;
        while (*cursor != '\0' && !is_space_char(*cursor)) {
            ++cursor;
        }

        if (*cursor != '\0') {
            *cursor = '\0';
            ++cursor;
        }

        return token;
    }

    char to_lower_ascii(char c)
    {
        if (c >= 'A' && c <= 'Z') {
            return static_cast<char>(c + ('a' - 'A'));
        }
        return c;
    }

    bool equals_ignore_case(const char* lhs, const char* rhs)
    {
        if (!lhs || !rhs) return false;

        size_t i = 0;
        while (lhs[i] != '\0' && rhs[i] != '\0')
        {
            if (to_lower_ascii(lhs[i]) != to_lower_ascii(rhs[i])) {
                return false;
            }
            ++i;
        }

        return lhs[i] == '\0' && rhs[i] == '\0';
    }

    bool try_parse_bool(char* token, bool& value)
    {
        if (!token || token[0] == '\0') {
            return false;
        }

        if (equals_ignore_case(token, "true") ||
            equals_ignore_case(token, "1") ||
            equals_ignore_case(token, "on"))
        {
            value = true;
            return true;
        }

        if (equals_ignore_case(token, "false") ||
            equals_ignore_case(token, "0") ||
            equals_ignore_case(token, "off"))
        {
            value = false;
            return true;
        }

        return false;
    }

    void log_write_illumination_fault_reporting_usage()
    {
        LOG("Usage: writeIlluminationFaultReporting <true|false>");
    }

    void handle_write_illumination_fault_reporting_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* bool_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!bool_token || extra_token)
        {
            LOG("writeIlluminationFaultReporting rejected: expected exactly one argument.");
            log_write_illumination_fault_reporting_usage();
            return;
        }

        bool requested_value = false;
        if (!try_parse_bool(bool_token, requested_value))
        {
            LOG("writeIlluminationFaultReporting rejected: expected true/false.");
            log_write_illumination_fault_reporting_usage();
            return;
        }

        if (!set_illumination_fault_reporting_enabled(requested_value))
        {
            LOG("writeIlluminationFaultReporting failed: could not persist value to NVS.");
            return;
        }

        LOG(
            "ILLUMINATION_FAULT_REPORTING set to %s.",
            is_illumination_fault_reporting_enabled() ? "TRUE" : "FALSE");
    }
}

extern const CommandDefinition kWriteIlluminationFaultReportingCommandDefinition = {
    "writeIlluminationFaultReporting",
    "writeIlluminationFaultReporting <true|false>",
    handle_write_illumination_fault_reporting_command
};
