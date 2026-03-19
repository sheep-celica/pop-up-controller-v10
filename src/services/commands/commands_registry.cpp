#include "services/commands/commands_registry.h"

#include "services/commands/command_definitions.h"

namespace {
    const CommandDefinition kCommands[] = {
        kHelpCommandDefinition,
        kPrintStatisticalDataCommandDefinition,
        kPrintPopUpTimingCalibrationCommandDefinition,
        kSavePopUpTimingCalibrationCommandDefinition,
        kPrintBatteryVoltageCalibrationCommandDefinition,
        kPrintEverythingCommandDefinition,
        kPrintErrorsCommandDefinition,
        kClearErrorsCommandDefinition,
        kClearStatisticalDataCommandDefinition,
        kWriteManufactureDataCommandDefinition,
        kWriteBatteryVoltageCalibrationCommandDefinition,
        kReadBatteryVoltageCommandDefinition,
        kReadTemperatureCommandDefinition,
        kWriteIdleTimeToPowerOffSecondsCommandDefinition,
        kWritePopUpMinStatePersistMsCommandDefinition,
        kWritePopUpSensingDelayUsCommandDefinition,
        kWriteSleepyEyeModeWithHeadlightsCommandDefinition,
        kPrintPopUpMinStatePersistMsCommandDefinition,
        kPrintPopUpSensingDelayUsCommandDefinition,
        kPrintSleepyEyeModeWithHeadlightsCommandDefinition,
        kPrintBuildInfoCommandDefinition,
        kPrintRemoteInputPinsCommandDefinition,
        kPrintRemoteInputsWithHeadlightsCommandDefinition,
        kGetControllerStatusCommandDefinition,
        kGetExternalExpanderCommandDefinition,
        kGetIdleTimeToPowerOffCommandDefinition,
        kRebootCommandDefinition,
        kSetRemoteInputPinsCommandDefinition,
        kWriteRemoteInputsWithHeadlightsCommandDefinition,
        kWinkCommandDefinition,
        kToggleCommandDefinition,
        kToggleSleepyEyeModeCommandDefinition,
        kClearPopUpTimingCalibrationCommandDefinition,
    };
}

const CommandDefinition* get_command_registry(size_t& count)
{
    count = sizeof(kCommands) / sizeof(kCommands[0]);
    return kCommands;
}
