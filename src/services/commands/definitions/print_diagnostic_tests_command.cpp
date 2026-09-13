#include "services/commands/command_definitions.h"
#include "services/diagnostics/diagnostics.h"
#include "services/logging/logging.h"

namespace {
    void handle(char* args)
    {
        if (args && args[0] != '\0') { LOG("Usage: printDiagnosticTests"); return; }
        diagnostics_print();
    }
}

extern const CommandDefinition kPrintDiagnosticTestsCommandDefinition = {
    "printDiagnosticTests", "printDiagnosticTests", handle
};
