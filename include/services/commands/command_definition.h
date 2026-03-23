#pragma once

#include <cstddef>

using CommandHandler = void (*)(char* remaining_args);

struct CommandDefinition
{
    const char* name;
    const char* usage;
    CommandHandler handler;
    bool visible_in_help = true;
};
