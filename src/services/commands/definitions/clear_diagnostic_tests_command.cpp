#include "services/commands/command_definitions.h"
#include "services/diagnostics/diagnostics.h"
#include "services/logging/logging.h"

namespace {
    void handle(char* args)
    {
        if (args && args[0] != '\0') { LOG("Usage: clearDiagnosticTests"); return; }
        LOG("clearDiagnosticTests %s.", diagnostics_clear() ? "succeeded" : "failed or test active");
    }
}

extern const CommandDefinition kClearDiagnosticTestsCommandDefinition = {
    "clearDiagnosticTests", "clearDiagnosticTests", handle
};
