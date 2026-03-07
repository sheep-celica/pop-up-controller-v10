#include "services/inputs/remote_input_pins.h"

#include <Preferences.h>

#include "config.h"
#include "services/inputs/logic/remote_input_1.h"
#include "services/inputs/logic/remote_input_2.h"
#include "services/inputs/logic/remote_input_3.h"
#include "services/inputs/logic/remote_input_4.h"
#include "services/io/io_expanders.h"
#include "services/logging/logging.h"

namespace {
    constexpr const char* kRemoteInputPinNamespace = "remote_in";
    constexpr const char* kRemoteInput1PinKey = "r1";
    constexpr const char* kRemoteInput2PinKey = "r2";
    constexpr const char* kRemoteInput3PinKey = "r3";
    constexpr const char* kRemoteInput4PinKey = "r4";

    Preferences s_remote_input_pin_preferences;
    bool s_remote_input_pin_preferences_initialized = false;
    bool s_remote_input_pin_mapping_loaded = false;
    RemoteInputPinMapping s_remote_input_pin_mapping = {1, 2, 3, 4};

    bool is_valid_pin_number(uint8_t pin_number_1_to_4)
    {
        return pin_number_1_to_4 >= 1u && pin_number_1_to_4 <= 4u;
    }

    bool is_unique_mapping(const RemoteInputPinMapping& mapping)
    {
        bool seen[5] = {false, false, false, false, false};
        const uint8_t pins[] = {
            mapping.remote_input_1,
            mapping.remote_input_2,
            mapping.remote_input_3,
            mapping.remote_input_4
        };

        for (const uint8_t pin : pins)
        {
            if (seen[pin]) {
                return false;
            }

            seen[pin] = true;
        }

        return true;
    }

    bool is_valid_mapping_impl(const RemoteInputPinMapping& mapping)
    {
        if (!is_valid_pin_number(mapping.remote_input_1) ||
            !is_valid_pin_number(mapping.remote_input_2) ||
            !is_valid_pin_number(mapping.remote_input_3) ||
            !is_valid_pin_number(mapping.remote_input_4))
        {
            return false;
        }

        return is_unique_mapping(mapping);
    }

    void ensure_remote_input_pin_preferences()
    {
        if (!s_remote_input_pin_preferences_initialized)
        {
            s_remote_input_pin_preferences.begin(kRemoteInputPinNamespace, false);
            s_remote_input_pin_preferences_initialized = true;
        }

        if (!s_remote_input_pin_mapping_loaded)
        {
            RemoteInputPinMapping loaded_mapping = s_remote_input_pin_mapping;

            const bool has_full_mapping =
                s_remote_input_pin_preferences.isKey(kRemoteInput1PinKey) &&
                s_remote_input_pin_preferences.isKey(kRemoteInput2PinKey) &&
                s_remote_input_pin_preferences.isKey(kRemoteInput3PinKey) &&
                s_remote_input_pin_preferences.isKey(kRemoteInput4PinKey);

            if (has_full_mapping)
            {
                loaded_mapping.remote_input_1 = s_remote_input_pin_preferences.getUChar(
                    kRemoteInput1PinKey,
                    s_remote_input_pin_mapping.remote_input_1);
                loaded_mapping.remote_input_2 = s_remote_input_pin_preferences.getUChar(
                    kRemoteInput2PinKey,
                    s_remote_input_pin_mapping.remote_input_2);
                loaded_mapping.remote_input_3 = s_remote_input_pin_preferences.getUChar(
                    kRemoteInput3PinKey,
                    s_remote_input_pin_mapping.remote_input_3);
                loaded_mapping.remote_input_4 = s_remote_input_pin_preferences.getUChar(
                    kRemoteInput4PinKey,
                    s_remote_input_pin_mapping.remote_input_4);
            }

            if (!is_valid_mapping_impl(loaded_mapping))
            {
                LOG("Stored remote input mapping is invalid; falling back to default 1 2 3 4.");
                loaded_mapping = {1, 2, 3, 4};
            }

            s_remote_input_pin_mapping = loaded_mapping;
            s_remote_input_pin_mapping_loaded = true;
        }
    }

    IoExpanderPin to_external_expander_pin(uint8_t pin_number_1_to_4)
    {
        switch (pin_number_1_to_4)
        {
            case 1: return config::pins::external_expander::REMOTE_INPUT_0;
            case 2: return config::pins::external_expander::REMOTE_INPUT_1;
            case 3: return config::pins::external_expander::REMOTE_INPUT_2;
            case 4: return config::pins::external_expander::REMOTE_INPUT_3;
            default: return config::pins::external_expander::REMOTE_INPUT_0;
        }
    }

    InputPin make_remote_input_pin(uint8_t pin_number_1_to_4)
    {
        return InputPin {
            .backend = InputBackend::EXTERNAL_EXPANDER,
            .expander_pin = to_external_expander_pin(pin_number_1_to_4)
        };
    }

    void apply_mapping_to_live_inputs(const RemoteInputPinMapping& mapping)
    {
        remote_input_1.set_pin(make_remote_input_pin(mapping.remote_input_1));
        remote_input_2.set_pin(make_remote_input_pin(mapping.remote_input_2));
        remote_input_3.set_pin(make_remote_input_pin(mapping.remote_input_3));
        remote_input_4.set_pin(make_remote_input_pin(mapping.remote_input_4));
    }

    bool persist_mapping(const RemoteInputPinMapping& mapping)
    {
        const size_t bytes_written_r1 = s_remote_input_pin_preferences.putUChar(
            kRemoteInput1PinKey,
            mapping.remote_input_1);
        const size_t bytes_written_r2 = s_remote_input_pin_preferences.putUChar(
            kRemoteInput2PinKey,
            mapping.remote_input_2);
        const size_t bytes_written_r3 = s_remote_input_pin_preferences.putUChar(
            kRemoteInput3PinKey,
            mapping.remote_input_3);
        const size_t bytes_written_r4 = s_remote_input_pin_preferences.putUChar(
            kRemoteInput4PinKey,
            mapping.remote_input_4);

        return bytes_written_r1 == sizeof(uint8_t) &&
               bytes_written_r2 == sizeof(uint8_t) &&
               bytes_written_r3 == sizeof(uint8_t) &&
               bytes_written_r4 == sizeof(uint8_t);
    }
}

bool is_valid_remote_input_pin_mapping(const RemoteInputPinMapping& mapping)
{
    return is_valid_mapping_impl(mapping);
}

RemoteInputPinMapping get_remote_input_pin_mapping()
{
    ensure_remote_input_pin_preferences();
    return s_remote_input_pin_mapping;
}

bool set_remote_input_pin_mapping(const RemoteInputPinMapping& mapping)
{
    if (!is_valid_mapping_impl(mapping)) {
        return false;
    }

    ensure_remote_input_pin_preferences();

    if (!persist_mapping(mapping)) {
        return false;
    }

    s_remote_input_pin_mapping = mapping;
    s_remote_input_pin_mapping_loaded = true;

    if (remote_pcf.isConnected())
    {
        apply_mapping_to_live_inputs(s_remote_input_pin_mapping);
    }

    return true;
}

void setup_remote_input_pin_mapping()
{
    ensure_remote_input_pin_preferences();

    if (remote_pcf.isConnected())
    {
        apply_mapping_to_live_inputs(s_remote_input_pin_mapping);
    }

    LOG(
        "Remote input mapping active: %u %u %u %u.",
        static_cast<unsigned>(s_remote_input_pin_mapping.remote_input_1),
        static_cast<unsigned>(s_remote_input_pin_mapping.remote_input_2),
        static_cast<unsigned>(s_remote_input_pin_mapping.remote_input_3),
        static_cast<unsigned>(s_remote_input_pin_mapping.remote_input_4));
}
