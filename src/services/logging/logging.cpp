#include <Arduino.h>
#include <Preferences.h>
#include "helpers/pop_up.h"
#include "config.h"
#include "services/logging/logging.h"
#include "services/logging/error_log_manager.h"
#include "services/logging/statistics_manager.h"
#include "services/utilities/utilities.h"
#include "services/utilities/temperature.h"
#include "services/io/leds.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>


static Preferences error_log_preferences_storage;
static Preferences statistics_preferences_storage;
static Preferences manufacturing_preferences_storage;
ErrorLogManager error_log_manager(error_log_preferences_storage);
StatisticsManager statistics_manager(statistics_preferences_storage);

namespace {
    // NVS keys are intentionally short due to ESP32 Preferences key length limits.
    constexpr const char* KEY_MFG_LOCK = "mfg_lock";
    constexpr const char* KEY_MFG_SN   = "serial";
    constexpr const char* KEY_MFG_DATE = "mfg_date";
    constexpr const char* KEY_MFG_FW   = "init_fw";

    struct ManufacturingDataCache
    {
        bool loaded = false;
        bool locked = false;
        char serial_number[32] = "";
        char manufacture_date[16] = "";
        char initial_fw_version[24] = "";
    };

    ManufacturingDataCache g_manufacture_data;
    char g_current_build_version[24] = "";
    char g_current_build_timestamp[32] = "";

    void copy_cstr(char* dst, size_t dst_size, const char* src)
    {
        if (dst_size == 0) return;
        if (!src) src = "";
        snprintf(dst, dst_size, "%s", src);
    }

    void extract_manufacture_date(char* out_date, size_t out_size, const char* build_timestamp)
    {
        if (out_size == 0) return;
        out_date[0] = '\0';

        if (!build_timestamp || build_timestamp[0] == '\0') {
            return;
        }

        // Expected build timestamp format is ISO-8601 UTC: YYYY-MM-DDTHH:MM:SSZ
        if (strlen(build_timestamp) >= 10) {
            snprintf(out_date, out_size, "%.10s", build_timestamp);
            return;
        }

        snprintf(out_date, out_size, "%s", build_timestamp);
    }

    void load_manufacture_data_from_nvs()
    {
        g_manufacture_data.locked = manufacturing_preferences_storage.getBool(KEY_MFG_LOCK, false);
        g_manufacture_data.serial_number[0] = '\0';
        g_manufacture_data.manufacture_date[0] = '\0';
        g_manufacture_data.initial_fw_version[0] = '\0';

        // Missing manufacturing fields are normal before provisioning; avoid
        // calling getString() on absent keys because Preferences logs NOT_FOUND.
        if (manufacturing_preferences_storage.isKey(KEY_MFG_SN)) {
            (void)manufacturing_preferences_storage.getString(
                KEY_MFG_SN, g_manufacture_data.serial_number, sizeof(g_manufacture_data.serial_number)
            );
        }
        if (manufacturing_preferences_storage.isKey(KEY_MFG_DATE)) {
            (void)manufacturing_preferences_storage.getString(
                KEY_MFG_DATE, g_manufacture_data.manufacture_date, sizeof(g_manufacture_data.manufacture_date)
            );
        }
        if (manufacturing_preferences_storage.isKey(KEY_MFG_FW)) {
            (void)manufacturing_preferences_storage.getString(
                KEY_MFG_FW, g_manufacture_data.initial_fw_version, sizeof(g_manufacture_data.initial_fw_version)
            );
        }

        g_manufacture_data.loaded = true;
    }
}

void initialize_logging()
{
    initialize_logging("", "");
}

void initialize_logging(const char* build_version, const char* build_timestamp)
{
    copy_cstr(g_current_build_version, sizeof(g_current_build_version), build_version);
    copy_cstr(g_current_build_timestamp, sizeof(g_current_build_timestamp), build_timestamp);

    manufacturing_preferences_storage.begin(config::utilities::MANUFACTURING_NAMESPACE, false);
    load_manufacture_data_from_nvs();

    statistics_manager.initialize();
    statistics_manager.increment_boot_count();
    error_log_manager.initialize();
}


void log_message(const char* fmt, ...)
{
    static constexpr size_t BUFFER_SIZE = 128;
    char buffer[BUFFER_SIZE];

    // Format user message
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, BUFFER_SIZE, fmt, args);
    va_end(args);

    // Print timestamp + message
    Serial.printf("[%lu] %s\n", millis(), buffer);
}

void report_pop_up_timeout(PopUpId pop_up_id)
{
    switch (pop_up_id)
    {
        case PopUpId::RH:
            report_error_code(ErrorCode::RH_POP_UP_TIMEOUT);
            break;

        case PopUpId::LH:
            report_error_code(ErrorCode::LH_POP_UP_TIMEOUT);
            break;
    }
    set_led_state(LedId::ERROR_LED, true);
}

void report_pop_up_overcurrent(PopUpId pop_up_id)
{
    switch (pop_up_id)
    {
        case PopUpId::RH:
            report_error_code(ErrorCode::RH_POP_UP_OVERCURRENT);
            break;

        case PopUpId::LH:
            report_error_code(ErrorCode::LH_POP_UP_OVERCURRENT);
            break;
    }
    set_led_state(LedId::ERROR_LED, true);
}

bool save_manufacture_data_once(const char* serial_number)
{
    if (!g_manufacture_data.loaded) {
        load_manufacture_data_from_nvs();
    }

    if (g_manufacture_data.locked)
    {
        LOG("Manufacture data already locked. Ignoring save request.");
        return false;
    }

    if (!serial_number || serial_number[0] == '\0')
    {
        LOG("Manufacture data save rejected: serial number is empty.");
        return false;
    }

    char manufacture_date[sizeof(g_manufacture_data.manufacture_date)];
    extract_manufacture_date(manufacture_date, sizeof(manufacture_date), g_current_build_timestamp);

    if (manufacture_date[0] == '\0')
    {
        LOG("Manufacture data save rejected: build timestamp is not available.");
        return false;
    }

    const char* initial_fw_version =
        (g_current_build_version[0] != '\0') ? g_current_build_version : "unknown";

    manufacturing_preferences_storage.putString(KEY_MFG_SN, serial_number);
    manufacturing_preferences_storage.putString(KEY_MFG_DATE, manufacture_date);
    manufacturing_preferences_storage.putString(KEY_MFG_FW, initial_fw_version);
    manufacturing_preferences_storage.putBool(KEY_MFG_LOCK, true); // lock set last to avoid partial lock on failed writes

    load_manufacture_data_from_nvs();

    LOG("Manufacture data saved and locked.");
    LOG("Manufacture SN=%s Date=%s InitialFW=%s",
        g_manufacture_data.serial_number,
        g_manufacture_data.manufacture_date,
        g_manufacture_data.initial_fw_version);

    return true;
}

void print_manufacture_data()
{
    if (!g_manufacture_data.loaded) {
        load_manufacture_data_from_nvs();
    }

    LOG("---- Manufacture Data ----");
    LOG("Locked: %s", g_manufacture_data.locked ? "true" : "false");
    LOG("Serial Number: %s", g_manufacture_data.serial_number[0] ? g_manufacture_data.serial_number : "<empty>");
    LOG("Manufacture Date: %s", g_manufacture_data.manufacture_date[0] ? g_manufacture_data.manufacture_date : "<empty>");
    LOG("Initial FW Version: %s", g_manufacture_data.initial_fw_version[0] ? g_manufacture_data.initial_fw_version : "<empty>");
    LOG("--------------------------");
}


// Moved from services/pop_up_control/error_reporting.cpp
void report_error_code(ErrorCode code)
{
    // Placeholder implementation: create an ErrorLog entry and store it
    ErrorLog entry{};
    entry.timestamp_ms = millis();
    entry.boot_count = statistics_manager.get_boot_count();
    entry.error_code = code;
    entry.battery_voltage_mv = volts_to_mv(read_battery_voltage());
    entry.temperature_decic = celsius_to_decic(read_temperature());

    switch (code)
    {
        case ErrorCode::RH_POP_UP_TIMEOUT:
        case ErrorCode::RH_POP_UP_OVERCURRENT:
            statistics_manager.record_pop_up_error(PopUpId::RH);
            break;

        case ErrorCode::LH_POP_UP_TIMEOUT:
        case ErrorCode::LH_POP_UP_OVERCURRENT:
            statistics_manager.record_pop_up_error(PopUpId::LH);
            break;

        default:
            break;
    }

    error_log_manager.add_error_log_entry(entry);

    LOG("Reported error: %s", error_code_to_string(code));
}
