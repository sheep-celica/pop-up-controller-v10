#include "services/commands/command_definitions.h"

#include <cstring>

#include "services/commands/command_passwords.h"
#include "services/logging/logging.h"
#include "services/maintenance/shipment_finalization.h"

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

    void log_usage()
    {
        LOG("Usage: finalizeForShipment <password>");
    }

    void handle_finalize_for_shipment(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* password = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!password)
        {
            LOG("finalizeForShipment rejected: missing password argument.");
            log_usage();
            return;
        }

        if (extra_token)
        {
            LOG("finalizeForShipment rejected: unexpected extra argument.");
            log_usage();
            return;
        }

        if (strcmp(password, command_passwords::kProtectedClearPassword) != 0)
        {
            LOG("finalizeForShipment rejected: incorrect password.");
            return;
        }

        if (!finalize_for_shipment())
        {
            LOG("finalizeForShipment failed.");
            return;
        }

        LOG("finalizeForShipment succeeded. Manufacturing data and calibration were preserved; errors, statistics, and test configuration were cleared.");
        LOG("Read-only maintenance mode active. Inspect the board, then disconnect power.");
    }
}

extern const CommandDefinition kFinalizeForShipmentCommandDefinition = {
    "finalizeForShipment",
    "finalizeForShipment <password>",
    handle_finalize_for_shipment,
    false
};
