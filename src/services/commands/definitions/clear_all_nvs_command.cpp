#include "services/commands/command_definitions.h"

#include <Arduino.h>
#include <cstring>
#include <nvs_flash.h>

#include "services/commands/command_passwords.h"
#include "services/commands/commands.h"
#include "services/logging/logging.h"

namespace {
    constexpr uint32_t kConfirmationTimeoutMs = 5000u;

    bool is_space_char(char c)
    {
        return c == ' ' || c == '\t';
    }

    char* next_token(char*& cursor)
    {
        if (!cursor) return nullptr;

        while (*cursor != '\0' && is_space_char(*cursor)) {
            ++cursor;
        }

        if (*cursor == '\0') {
            return nullptr;
        }

        char* token = cursor;
        while (*cursor != '\0' && !is_space_char(*cursor)) {
            ++cursor;
        }

        if (*cursor != '\0') {
            *cursor = '\0';
            ++cursor;
        }

        return token;
    }

    void log_clear_all_nvs_usage()
    {
        LOG("Usage: clearAllNvs <password>");
    }

    void handle_clear_all_nvs_confirmation(char* line, bool timed_out)
    {
        if (timed_out) {
            LOG("clearAllNvs cancelled: confirmation timed out.");
            return;
        }

        if (!line || strcmp(line, "yes") != 0) {
            LOG("clearAllNvs cancelled: confirmation response was not 'yes'.");
            return;
        }

        LOG("clearAllNvs confirmation accepted. Erasing all NVS data.");

        const esp_err_t deinit_result = nvs_flash_deinit();
        if (deinit_result != ESP_OK && deinit_result != ESP_ERR_NVS_NOT_INITIALIZED) {
            LOG("clearAllNvs failed: nvs_flash_deinit returned %ld.", static_cast<long>(deinit_result));
            return;
        }

        const esp_err_t erase_result = nvs_flash_erase();
        if (erase_result != ESP_OK) {
            LOG("clearAllNvs failed: nvs_flash_erase returned %ld.", static_cast<long>(erase_result));
            return;
        }

        // Do not use reboot_controller() here because the normal power-off path saves state back into NVS.
        LOG("clearAllNvs succeeded: all NVS data erased. Rebooting now.");
        Serial.flush();
        delay(100);
        ESP.restart();
    }

    void handle_clear_all_nvs(char* remaining_args)
    {
        char* cursor = remaining_args;
        char* password = next_token(cursor);
        char* extra_token = next_token(cursor);

        if (!password) {
            LOG("clearAllNvs rejected: missing password argument.");
            log_clear_all_nvs_usage();
            return;
        }

        if (extra_token) {
            LOG("clearAllNvs rejected: unexpected extra argument.");
            log_clear_all_nvs_usage();
            return;
        }

        if (strcmp(password, command_passwords::kProtectedClearPassword) != 0) {
            LOG("clearAllNvs rejected: incorrect password.");
            return;
        }

        if (!request_next_command_line(handle_clear_all_nvs_confirmation, kConfirmationTimeoutMs)) {
            LOG("clearAllNvs rejected: another confirmation is already pending.");
            return;
        }

        LOG("clearAllNvs confirmation required.");
        LOG("Reply with 'yes' within 5 seconds to erase all NVS data and reboot.");
    }
}

extern const CommandDefinition kClearAllNvsCommandDefinition = {
    "clearAllNvs",
    "clearAllNvs <password>",
    handle_clear_all_nvs,
    false
};
