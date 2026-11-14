#include "services/commands/command_definitions.h"

#include <cstdlib>

#include "services/io/remote_module.h"
#include "services/logging/logging.h"

namespace {
    constexpr uint16_t kDefaultDurationSeconds = 15;
    constexpr uint16_t kMaximumDurationSeconds = 15 * 60;

    void handle_test_remote_wor_command(char* remaining_args)
    {
        uint16_t duration_seconds = kDefaultDurationSeconds;
        if (remaining_args && remaining_args[0] != '\0') {
            char* end = nullptr;
            const unsigned long parsed = strtoul(remaining_args, &end, 10);
            while (end && (*end == ' ' || *end == '\t')) ++end;
            if (end == remaining_args || (end && *end != '\0') ||
                parsed == 0 || parsed > kMaximumDurationSeconds) {
                LOG("testRemoteWor rejected: duration must be 1-%u seconds.",
                    static_cast<unsigned>(kMaximumDurationSeconds));
                LOG("Usage: testRemoteWor [duration_seconds]");
                return;
            }
            duration_seconds = static_cast<uint16_t>(parsed);
        }

        if (!test_remote_wor(duration_seconds)) {
            LOG("Remote WOR test could not run; check module connection and pairing state.");
        }
    }
}

extern const CommandDefinition kTestRemoteWorCommandDefinition = {
    "testRemoteWor",
    "testRemoteWor [duration_seconds]",
    handle_test_remote_wor_command,
    true
};
