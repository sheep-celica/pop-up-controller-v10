#include "services/commands/command_definitions.h"

#include <cstdlib>

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

    bool try_parse_i2c_address(char* token, uint8_t& address)
    {
        if (!token || token[0] == '\0') {
            return false;
        }

        char* end = nullptr;
        const unsigned long parsed = strtoul(token, &end, 0);
        if (end == token || !end || *end != '\0' || parsed > 0x7F) {
            return false;
        }

        address = static_cast<uint8_t>(parsed);
        return true;
    }

    void log_write_external_expander_i2c_address_usage()
    {
        LOG("Usage: writeExternalExpanderI2cAddress <address>");
        LOG("Address may be decimal or hex (for example 60 or 0x3C).");
    }

    void handle_write_external_expander_i2c_address_command(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* address_token = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!address_token || extra_token) {
            LOG("writeExternalExpanderI2cAddress rejected: expected exactly one address argument.");
            log_write_external_expander_i2c_address_usage();
            return;
        }

        uint8_t address = 0;
        if (!try_parse_i2c_address(address_token, address)) {
            LOG("writeExternalExpanderI2cAddress rejected: invalid I2C address.");
            log_write_external_expander_i2c_address_usage();
            return;
        }

        if (!is_valid_external_expander_i2c_address(address)) {
            LOG("writeExternalExpanderI2cAddress rejected: supported addresses are 0x20-0x27 and 0x38-0x3F.");
            return;
        }

        if (!set_external_expander_i2c_address(address)) {
            LOG("writeExternalExpanderI2cAddress failed: could not contact the expander at 0x%02X or persist the value.", address);
            return;
        }

        LOG("External expander I2C address updated to 0x%02X.", get_external_expander_i2c_address());
    }
}

extern const CommandDefinition kWriteExternalExpanderI2cAddressCommandDefinition = {
    "writeExternalExpanderI2cAddress",
    "writeExternalExpanderI2cAddress <address>",
    handle_write_external_expander_i2c_address_command
};
