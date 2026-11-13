#include "services/commands/command_definitions.h"

#include <cstdlib>

#include "services/io/remote_module.h"
#include "services/logging/logging.h"

namespace {
    constexpr uint8_t kDefaultDurationSeconds = 10;
    constexpr uint8_t kMaximumDurationSeconds = 30;

    void handle_test_remote_carrier_sense_command(char* remaining_args)
    {
        uint8_t duration_seconds = kDefaultDurationSeconds;
        if (remaining_args && remaining_args[0] != '\0') {
            char* end = nullptr;
            const unsigned long parsed = strtoul(remaining_args, &end, 10);
            while (end && (*end == ' ' || *end == '\t')) ++end;
            if (end == remaining_args || (end && *end != '\0') ||
                parsed == 0 || parsed > kMaximumDurationSeconds) {
                LOG("testRemoteCarrierSense rejected: duration must be 1-%u seconds.",
                    static_cast<unsigned>(kMaximumDurationSeconds));
                LOG("Usage: testRemoteCarrierSense [duration_seconds]");
                return;
            }
            duration_seconds = static_cast<uint8_t>(parsed);
        }

        if (!test_remote_carrier_sense(duration_seconds)) {
            LOG("Remote carrier-sense test could not run; check module connection and pairing state.");
        }
    }
}

extern const CommandDefinition kTestRemoteCarrierSenseCommandDefinition = {
    "testRemoteCarrierSense",
    "testRemoteCarrierSense [duration_seconds]",
    handle_test_remote_carrier_sense_command,
    true
};
