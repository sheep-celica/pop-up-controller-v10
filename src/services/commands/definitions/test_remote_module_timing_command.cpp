#include "services/commands/command_definitions.h"

#include <Arduino.h>
#include <cstdlib>

#include "services/io/remote_module.h"
#include "services/logging/logging.h"

namespace {
    constexpr uint16_t kDefaultSamples = 100;
    constexpr uint16_t kMaximumSamples = 1000;

    void log_usage()
    {
        LOG("Usage: testRemoteModuleTiming [samples]");
    }

    void handle_test_remote_module_timing_command(char* remaining_args)
    {
        uint16_t samples = kDefaultSamples;
        if (remaining_args && remaining_args[0] != '\0') {
            char* end = nullptr;
            const unsigned long parsed = strtoul(remaining_args, &end, 10);
            while (end && (*end == ' ' || *end == '\t')) ++end;
            if (end == remaining_args || (end && *end != '\0') ||
                parsed == 0 || parsed > kMaximumSamples) {
                LOG("testRemoteModuleTiming rejected: samples must be 1-%u.",
                    static_cast<unsigned>(kMaximumSamples));
                log_usage();
                return;
            }
            samples = static_cast<uint16_t>(parsed);
        }

        if (!test_remote_module_communication_timing(samples)) {
            LOG("Remote-module timing test could not run; the module is not connected.");
        }
    }
}

extern const CommandDefinition kTestRemoteModuleTimingCommandDefinition = {
    "testRemoteModuleTiming",
    "testRemoteModuleTiming [samples]",
    handle_test_remote_module_timing_command,
    true
};
