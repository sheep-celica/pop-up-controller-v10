#pragma once

#include <cstdint>

struct RemoteInputPinMapping
{
    uint8_t remote_input_1;
    uint8_t remote_input_2;
    uint8_t remote_input_3;
    uint8_t remote_input_4;
};

bool is_valid_remote_input_pin_mapping(const RemoteInputPinMapping& mapping);
RemoteInputPinMapping get_remote_input_pin_mapping();
bool set_remote_input_pin_mapping(const RemoteInputPinMapping& mapping);
void setup_remote_input_pin_mapping();
