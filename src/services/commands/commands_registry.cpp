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
        kClearAllNvsCommandDefinition,
        kFinalizeForShipmentCommandDefinition,
        kWriteManufactureDataCommandDefinition,
        kWriteBatteryVoltageCalibrationCommandDefinition,
        kReadBatteryVoltageCommandDefinition,
        kReadPotValuesCommandDefinition,
        kReadTemperatureCommandDefinition,
        kReadFaultsCommandDefinition,
        kWriteIdleTimeToPowerOffSecondsCommandDefinition,
        kWritePopUpMinStatePersistMsCommandDefinition,
        kWritePopUpSensingDelayUsCommandDefinition,
        kWriteSleepyEyeModeWithHeadlightsCommandDefinition,
        kPrintPopUpMinStatePersistMsCommandDefinition,
        kPrintPopUpSensingDelayUsCommandDefinition,
        kPrintSleepyEyeModeWithHeadlightsCommandDefinition,
        kPrintIlluminationFaultReportingCommandDefinition,
        kPrintBuildInfoCommandDefinition,
        kPrintRemoteInputPinsCommandDefinition,
        kPrintRemoteInputsWithHeadlightsCommandDefinition,
        kGetControllerStatusCommandDefinition,
        kGetExternalExpanderCommandDefinition,
        kGetIdleTimeToPowerOffCommandDefinition,
        kForceSleepCommandDefinition,
        kRebootCommandDefinition,
        kSetRemoteInputPinsCommandDefinition,
        kWriteRemoteInputsWithHeadlightsCommandDefinition,
        kWriteIlluminationFaultReportingCommandDefinition,
        kWinkCommandDefinition,
        kToggleCommandDefinition,
        kToggleSleepyEyeModeCommandDefinition,
        kClearPopUpTimingCalibrationCommandDefinition,
        kCalibrateMotorCurrentCommandDefinition,
        kSaveMotorCurrentCalibrationCommandDefinition,
        kPrintMotorCurrentCalibrationCommandDefinition,
        kTestMotorCurrentCommandDefinition,
        kPrintMotorStallProtectionCommandDefinition,
        kWriteMotorStallProtectionCommandDefinition,
    };
}

const CommandDefinition* get_command_registry(size_t& count)
{
    count = sizeof(kCommands) / sizeof(kCommands[0]);
    return kCommands;
}
