#pragma once

#include <cstddef>

#include "services/commands/command_definition.h"

const CommandDefinition* get_command_registry(size_t& count);

