#include "services/commands/command_definitions.h"

#include "services/logging/logging.h"
#include "services/utilities/controller_status.h"

namespace {
    void handle_print_forced_bench_mode_command(char* remaining_args)
    {
        if (remaining_args && *remaining_args != '\0')
        {
            LOG("printForcedBenchMode rejected: this command does not take arguments.");
            LOG("Usage: printForcedBenchMode");
            return;
        }

        LOG(
            "FORCED_BENCH_MODE configured=%s active=%s reboot_required=%s",
            is_forced_bench_mode_configured() ? "true" : "false",
            is_forced_bench_mode_active() ? "true" : "false",
            is_forced_bench_mode_configured() != is_forced_bench_mode_active()
                ? "true"
                : "false");
    }
}

extern const CommandDefinition kPrintForcedBenchModeCommandDefinition = {
    "printForcedBenchMode",
    "printForcedBenchMode",
    handle_print_forced_bench_mode_command
};
