#include "services/commands/commands_registry.h"

#include "services/commands/command_definitions.h"

namespace {
    const CommandDefinition kCommands[] = {
        kHelpCommandDefinition,
        kPrintStatisticalDataCommandDefinition,
        kPrintPopUpTimingCalibrationCommandDefinition,
        kSavePopUpTimingCalibrationCommandDefinition,
        kPrintBatteryVoltageCalibrationCommandDefinition,
        kPrintErrorsCommandDefinition,
        kClearErrorsCommandDefinition,
        kClearStatisticalDataCommandDefinition,
        kWriteManufactureDataCommandDefinition,
        kWriteBatteryVoltageCalibrationCommandDefinition,
        kWriteIdleTimeToPowerOffSecondsCommandDefinition,
        kSetRemoteInputPinsCommandDefinition,
    };
}

const CommandDefinition* get_command_registry(size_t& count)
{
    count = sizeof(kCommands) / sizeof(kCommands[0]);
    return kCommands;
}
