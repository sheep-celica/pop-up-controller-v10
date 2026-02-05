#pragma once

#include <vector>
#include <Arduino.h>
#include "services/inputs/input.h"
#include <Wire.h>
#include "PCF8574.h"
#include "helpers/ADS7138.h"


class InputManager
{
public:
    void add(Input& input);
    void update();

private:
    std::vector<Input*> inputs;
};


void setup_io_expanders();


extern InputManager inputs_manager;
extern PCF8574 remote_pcf;
extern ADS7138 internal_ads;