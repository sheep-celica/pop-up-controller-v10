#include "services/commands/command_definitions.h"

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

    void log_print_illumination_fault_reporting_usage()
    {
        LOG("Usage: printIlluminationFaultReporting");
    }

    void handle_print_illumination_fault_reporting_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* extra_token = next_token(cursor);
        if (extra_token)
        {
            LOG("printIlluminationFaultReporting rejected: this command does not take arguments.");
            log_print_illumination_fault_reporting_usage();
            return;
        }

        LOG(
            "ILLUMINATION_FAULT_REPORTING=%s",
            is_illumination_fault_reporting_enabled() ? "TRUE" : "FALSE");
    }
}

extern const CommandDefinition kPrintIlluminationFaultReportingCommandDefinition = {
    "printIlluminationFaultReporting",
    "printIlluminationFaultReporting",
    handle_print_illumination_fault_reporting_command
};
