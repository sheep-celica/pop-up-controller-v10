#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"

namespace {
    void handle_print_statistical_data_command(char* remaining_args)
    {
        (void)remaining_args;
        LOG("printStatisticalData command received.");
        statistics_manager.print_statistics();
    }
}

extern const CommandDefinition kPrintStatisticalDataCommandDefinition = {
    "printStatisticalData",
    "printStatisticalData",
    handle_print_statistical_data_command
};

