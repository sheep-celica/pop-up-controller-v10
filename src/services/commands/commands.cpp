#include "services/commands/commands.h"

#include <Arduino.h>
#include <cstring>

#include "config.h"
#include "services/commands/commands_registry.h"
#include "services/logging/logging.h"
#include "services/pop_up_control/pop_up_control.h"

namespace {
    constexpr size_t COMMAND_BUFFER_SIZE = 128;

    char s_command_buffer[COMMAND_BUFFER_SIZE];
    size_t s_command_length = 0;
    bool s_discard_until_newline = false;
    PendingCommandLineHandler s_pending_command_line_handler = nullptr;
    uint32_t s_pending_command_line_deadline_ms = 0;

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

    void clear_pending_command_line_state()
    {
        s_pending_command_line_handler = nullptr;
        s_pending_command_line_deadline_ms = 0;
    }

    void dispatch_pending_command_line(char* line, bool timed_out)
    {
        PendingCommandLineHandler handler = s_pending_command_line_handler;
        clear_pending_command_line_state();

        if (handler) {
            handler(line, timed_out);
        }
    }

    void expire_pending_command_line_if_needed(uint32_t now_ms)
    {
        if (!s_pending_command_line_handler) {
            return;
        }

        if (static_cast<int32_t>(now_ms - s_pending_command_line_deadline_ms) > 0) {
            dispatch_pending_command_line(nullptr, true);
        }
    }

    bool should_process_commands_now()
    {
        const bool pop_ups_idle =
            (RH_POP_UP.get_target() == PopUpState::IDLE ||
            RH_POP_UP.get_target() == PopUpState::TIMEOUT) &&
            (LH_POP_UP.get_target() == PopUpState::IDLE ||
            LH_POP_UP.get_target() == PopUpState::TIMEOUT);

        return pop_ups_idle;
    }
}

void update_commands()
{
    expire_pending_command_line_if_needed(millis());

    if (!should_process_commands_now())
    {
        return;
    }

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

                if (s_pending_command_line_handler) {
                    s_command_buffer[0] = '\0';
                    dispatch_pending_command_line(s_command_buffer, false);
                }
                continue;
            }

            s_command_buffer[s_command_length] = '\0';
            if (s_pending_command_line_handler) {
                trim_in_place(s_command_buffer);
                dispatch_pending_command_line(s_command_buffer, false);
            } else {
                handle_command_line(s_command_buffer);
            }
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

bool request_next_command_line(PendingCommandLineHandler handler, uint32_t timeout_ms)
{
    if (!handler || timeout_ms == 0u || s_pending_command_line_handler) {
        return false;
    }

    s_pending_command_line_handler = handler;
    s_pending_command_line_deadline_ms = millis() + timeout_ms;
    return true;
}
