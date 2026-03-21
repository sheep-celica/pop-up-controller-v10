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
    constexpr const char* KEY_MFG_BSN  = "brd_ser";
    constexpr const char* KEY_MFG_REV  = "brd_rev";
    constexpr const char* KEY_MFG_CAR  = "car_mod";
    constexpr const char* KEY_MFG_DATE = "mfg_date";
    constexpr const char* KEY_MFG_FW   = "init_fw";

    constexpr size_t SERIAL_NUMBER_MAX_CHARS = 12;
    constexpr size_t BOARD_SERIAL_MAX_CHARS = 39;
    constexpr size_t BOARD_REVISION_MAX_CHARS = 23;
    constexpr size_t CAR_MODEL_MAX_CHARS = 39;
    constexpr size_t MANUFACTURE_DATE_MAX_CHARS = 31;

    struct ManufacturingDataCache
    {
        bool loaded = false;
        bool locked = false;
        char serial_number[SERIAL_NUMBER_MAX_CHARS + 1] = "";
        char board_serial[BOARD_SERIAL_MAX_CHARS + 1] = "";
        char board_revision[BOARD_REVISION_MAX_CHARS + 1] = "";
        char car_model[CAR_MODEL_MAX_CHARS + 1] = "";
        char manufacture_date[MANUFACTURE_DATE_MAX_CHARS + 1] = "";
        char initial_fw_version[24] = "";
    };

    ManufacturingDataCache g_manufacture_data;
    char g_current_build_version[24] = "";
    char g_current_build_timestamp[32] = "";
    bool g_error_reported_since_boot = false;

    void copy_cstr(char* dst, size_t dst_size, const char* src)
    {
        if (dst_size == 0) return;
        if (!src) src = "";
        snprintf(dst, dst_size, "%s", src);
    }

    bool is_ascii_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    bool is_ascii_alpha(char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }

    bool is_token_char(char c)
    {
        return is_ascii_digit(c) || is_ascii_alpha(c) || c == '_' || c == '-';
    }

    bool validate_max_length(const char* field_name, const char* value, size_t max_chars)
    {
        if (strlen(value) <= max_chars) {
            return true;
        }

        LOG(
            "Manufacture data save rejected: %s exceeds %u chars.",
            field_name,
            static_cast<unsigned>(max_chars));
        return false;
    }

    bool validate_numeric_serial_number(const char* serial_number)
    {
        for (size_t i = 0; serial_number[i] != '\0'; ++i)
        {
            if (!is_ascii_digit(serial_number[i])) {
                LOG("Manufacture data save rejected: serial number must use digits 0-9 only.");
                return false;
            }
        }
        return true;
    }

    bool validate_token_field(const char* field_name, const char* value)
    {
        for (size_t i = 0; value[i] != '\0'; ++i)
        {
            if (!is_token_char(value[i])) {
                LOG(
                    "Manufacture data save rejected: %s allows A-Z, a-z, 0-9, '_' and '-' only.",
                    field_name);
                return false;
            }
        }
        return true;
    }

    bool validate_car_model_field(const char* car_model)
    {
        for (size_t i = 0; car_model[i] != '\0'; ++i)
        {
            if (car_model[i] == ' ') {
                continue;
            }
            if (!is_token_char(car_model[i])) {
                LOG(
                    "Manufacture data save rejected: car model allows A-Z, a-z, 0-9, space, '_' and '-' only.");
                return false;
            }
        }
        return true;
    }

    void load_manufacture_data_from_nvs()
    {
        g_manufacture_data.locked = manufacturing_preferences_storage.getBool(KEY_MFG_LOCK, false);
        g_manufacture_data.serial_number[0] = '\0';
        g_manufacture_data.board_serial[0] = '\0';
        g_manufacture_data.board_revision[0] = '\0';
        g_manufacture_data.car_model[0] = '\0';
        g_manufacture_data.manufacture_date[0] = '\0';
        g_manufacture_data.initial_fw_version[0] = '\0';

        // Missing manufacturing fields are normal before provisioning; avoid
        // calling getString() on absent keys because Preferences logs NOT_FOUND.
        if (manufacturing_preferences_storage.isKey(KEY_MFG_SN)) {
            (void)manufacturing_preferences_storage.getString(
                KEY_MFG_SN, g_manufacture_data.serial_number, sizeof(g_manufacture_data.serial_number)
            );
        }
        if (manufacturing_preferences_storage.isKey(KEY_MFG_BSN)) {
            (void)manufacturing_preferences_storage.getString(
                KEY_MFG_BSN, g_manufacture_data.board_serial, sizeof(g_manufacture_data.board_serial)
            );
        }
        if (manufacturing_preferences_storage.isKey(KEY_MFG_REV)) {
            (void)manufacturing_preferences_storage.getString(
                KEY_MFG_REV, g_manufacture_data.board_revision, sizeof(g_manufacture_data.board_revision)
            );
        }
        if (manufacturing_preferences_storage.isKey(KEY_MFG_CAR)) {
            (void)manufacturing_preferences_storage.getString(
                KEY_MFG_CAR, g_manufacture_data.car_model, sizeof(g_manufacture_data.car_model)
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
    g_error_reported_since_boot = false;
}

const char* get_current_build_version()
{
    return g_current_build_version;
}

const char* get_current_build_timestamp()
{
    return g_current_build_timestamp;
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

bool save_manufacture_data_once(
    const char* manufacture_date,
    const char* serial_number,
    const char* board_serial,
    const char* board_revision,
    const char* car_model)
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

    if (!board_serial || board_serial[0] == '\0')
    {
        LOG("Manufacture data save rejected: board serial is empty.");
        return false;
    }

    if (!board_revision || board_revision[0] == '\0')
    {
        LOG("Manufacture data save rejected: board revision is empty.");
        return false;
    }

    if (!car_model || car_model[0] == '\0')
    {
        LOG("Manufacture data save rejected: car model is empty.");
        return false;
    }

    if (!manufacture_date || manufacture_date[0] == '\0')
    {
        LOG("Manufacture data save rejected: manufacture date is empty.");
        return false;
    }

    if (!validate_max_length("serial number", serial_number, SERIAL_NUMBER_MAX_CHARS)) {
        return false;
    }
    if (!validate_max_length("board serial", board_serial, BOARD_SERIAL_MAX_CHARS)) {
        return false;
    }
    if (!validate_max_length("board revision", board_revision, BOARD_REVISION_MAX_CHARS)) {
        return false;
    }
    if (!validate_max_length("car model", car_model, CAR_MODEL_MAX_CHARS)) {
        return false;
    }
    if (!validate_max_length("manufacture date", manufacture_date, MANUFACTURE_DATE_MAX_CHARS)) {
        return false;
    }

    if (!validate_numeric_serial_number(serial_number)) {
        return false;
    }
    if (!validate_token_field("board serial", board_serial)) {
        return false;
    }
    if (!validate_token_field("board revision", board_revision)) {
        return false;
    }
    if (!validate_car_model_field(car_model)) {
        return false;
    }

    const char* initial_fw_version =
        (g_current_build_version[0] != '\0') ? g_current_build_version : "unknown";

    manufacturing_preferences_storage.putString(KEY_MFG_SN, serial_number);
    manufacturing_preferences_storage.putString(KEY_MFG_BSN, board_serial);
    manufacturing_preferences_storage.putString(KEY_MFG_REV, board_revision);
    manufacturing_preferences_storage.putString(KEY_MFG_CAR, car_model);
    manufacturing_preferences_storage.putString(KEY_MFG_DATE, manufacture_date);
    manufacturing_preferences_storage.putString(KEY_MFG_FW, initial_fw_version);
    manufacturing_preferences_storage.putBool(KEY_MFG_LOCK, true); // lock set last to avoid partial lock on failed writes

    load_manufacture_data_from_nvs();

    LOG("Manufacture data saved and locked.");
    LOG("SN=%s BoardSN=%s BoardRev=%s CarModel=%s Date=%s InitialFW=%s",
        g_manufacture_data.serial_number,
        g_manufacture_data.board_serial,
        g_manufacture_data.board_revision,
        g_manufacture_data.car_model,
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
    LOG("Board Serial: %s", g_manufacture_data.board_serial[0] ? g_manufacture_data.board_serial : "<empty>");
    LOG("Board Revision: %s", g_manufacture_data.board_revision[0] ? g_manufacture_data.board_revision : "<empty>");
    LOG("Car Model: %s", g_manufacture_data.car_model[0] ? g_manufacture_data.car_model : "<empty>");
    LOG("Manufacture Date: %s", g_manufacture_data.manufacture_date[0] ? g_manufacture_data.manufacture_date : "<empty>");
    LOG("Initial FW Version: %s", g_manufacture_data.initial_fw_version[0] ? g_manufacture_data.initial_fw_version : "<empty>");
    LOG("--------------------------");
}

bool has_error_reported_since_boot()
{
    return g_error_reported_since_boot;
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

    g_error_reported_since_boot = true;
    error_log_manager.add_error_log_entry(entry);

    LOG("Reported error: %s", error_code_to_string(code));
}
