#include "services/commands/commands.h"

#include <Arduino.h>
#include <cstring>

#include "services/commands/commands_registry.h"
#include "services/logging/logging.h"

namespace {
    constexpr size_t COMMAND_BUFFER_SIZE = 128;

    char s_command_buffer[COMMAND_BUFFER_SIZE];
    size_t s_command_length = 0;
    bool s_discard_until_newline = false;

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

    void handle_command_line(char* line)
    {
        trim_in_place(line);
        if (line[0] == '\0') {
            return;
        }

        char* cursor = line;
        char* command = next_token(cursor);
        if (!command) {
            return;
        }

        size_t command_count = 0;
        const CommandDefinition* commands = get_command_registry(command_count);

        for (size_t i = 0; i < command_count; ++i)
        {
            if (strcmp(command, commands[i].name) == 0)
            {
                commands[i].handler(cursor);
                return;
            }
        }

        LOG("Unknown command: %s", command);
        LOG("Type 'help' for available commands.");
    }
}

void update_commands()
{
    while (Serial.available() > 0)
    {
        int raw = Serial.read();
        if (raw < 0) {
            return;
        }

        char c = static_cast<char>(raw);

        if (c == '\r') {
            continue;
        }

        if (c == '\n')
        {
            if (s_discard_until_newline) {
                s_discard_until_newline = false;
                s_command_length = 0;
                LOG("Command rejected: line too long (max %u chars).", static_cast<unsigned>(COMMAND_BUFFER_SIZE - 1));
                continue;
            }

            s_command_buffer[s_command_length] = '\0';
            handle_command_line(s_command_buffer);
            s_command_length = 0;
            continue;
        }

        if (s_discard_until_newline) {
            continue;
        }

        if (s_command_length >= COMMAND_BUFFER_SIZE - 1)
        {
            s_discard_until_newline = true;
            continue;
        }

        s_command_buffer[s_command_length++] = c;
    }
}
