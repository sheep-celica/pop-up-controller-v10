#include "services/commands/command_definitions.h"

#include <cstring>

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

    void trim_in_place(char* text)
    {
        if (!text) return;

        size_t len = strlen(text);
        while (len > 0 && is_space_char(text[len - 1])) {
            text[len - 1] = '\0';
            --len;
        }

        size_t lead = 0;
        while (text[lead] != '\0' && is_space_char(text[lead])) {
            ++lead;
        }

        if (lead > 0) {
            memmove(text, text + lead, strlen(text + lead) + 1);
        }
    }

    void log_write_manufacture_data_usage()
    {
        LOG("Usage: writeManufactureData <serial_number> <board_serial> <board_revision> <car_model...>");
    }

    void handle_write_manufacture_data_command(char* remaining_args)
    {
        if (!remaining_args) {
            LOG("writeManufactureData rejected: missing required arguments.");
            log_write_manufacture_data_usage();
            return;
        }

        trim_in_place(remaining_args);

        if (remaining_args[0] == '\0') {
            LOG("writeManufactureData rejected: missing required arguments.");
            log_write_manufacture_data_usage();
            return;
        }

        char* cursor = remaining_args;
        char* serial_number = next_token(cursor);
        char* board_serial = next_token(cursor);
        char* board_revision = next_token(cursor);
        trim_in_place(cursor);
        char* car_model = cursor;

        if (!serial_number || !board_serial || !board_revision || !car_model || car_model[0] == '\0') {
            LOG("writeManufactureData rejected: expected serial number, board serial, board revision, and car model.");
            log_write_manufacture_data_usage();
            return;
        }

        LOG("writeManufactureData command received.");
        (void)save_manufacture_data_once(serial_number, board_serial, board_revision, car_model);
    }
}

extern const CommandDefinition kWriteManufactureDataCommandDefinition = {
    "writeManufactureData",
    "writeManufactureData <serial_number> <board_serial> <board_revision> <car_model...>",
    handle_write_manufacture_data_command
};
