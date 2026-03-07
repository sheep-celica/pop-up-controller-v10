#include "services/commands/command_definitions.h"

#include <cstring>

#include "services/logging/logging.h"

namespace {
    bool is_space_char(char c)
    {
        return c == ' ' || c == '\t';
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

    void handle_write_manufacture_data_command(char* remaining_args)
    {
        if (!remaining_args) {
            LOG("writeManufactureData rejected: missing serial number argument.");
            LOG("Usage: writeManufactureData <serial_number...>");
            return;
        }

        trim_in_place(remaining_args);

        if (remaining_args[0] == '\0') {
            LOG("writeManufactureData rejected: missing serial number argument.");
            LOG("Usage: writeManufactureData <serial_number...>");
            return;
        }

        LOG("writeManufactureData command received.");
        (void)save_manufacture_data_once(remaining_args);
    }
}

extern const CommandDefinition kWriteManufactureDataCommandDefinition = {
    "writeManufactureData",
    "writeManufactureData <serial_number...>",
    handle_write_manufacture_data_command
};

