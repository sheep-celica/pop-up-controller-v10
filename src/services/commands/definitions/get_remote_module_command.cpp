#include "services/commands/command_definitions.h"

#include "services/io/remote_module.h"
#include "services/io/power.h"
#include "services/logging/logging.h"

namespace {
    void handle_get_remote_module_command(char* remaining_args)
    {
        if (remaining_args && remaining_args[0] != '\0') {
            LOG("getRemoteModule rejected: this command does not take arguments.");
            LOG("Usage: getRemoteModule");
            return;
        }
        log_remote_module_status();
        LOG("Deep-sleep wake count: %lu",
            static_cast<unsigned long>(get_deep_sleep_wake_count()));
        if (is_two_stage_remote_sleep_supported()) {
            LOG("Two-stage remote sleep: %s, stage=%s, initial_remaining_s=%lu, evaluation=%s, unqualified_wakes=%lu",
                is_two_stage_remote_sleep_enabled() ? "Enabled" : "Disabled",
                get_remote_sleep_stage_name(),
                static_cast<unsigned long>(
                    get_initial_remote_sleep_remaining_seconds()),
                is_remote_wake_evaluation_active() ? "active" : "inactive",
                static_cast<unsigned long>(get_unqualified_remote_wake_count()));
        }
    }
}

extern const CommandDefinition kGetRemoteModuleCommandDefinition = {
    "getRemoteModule",
    "getRemoteModule",
    handle_get_remote_module_command
};
