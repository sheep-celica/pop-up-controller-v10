#pragma once

#include "helpers/DRV8243.h"

// DRV8873H-Q1 in independent half-bridge mode has the same firmware-facing
// run/brake/coast truth table as the existing modern driver path. Keeping a
// distinct type prevents its current-sense and protection configuration from
// being confused with the DRV8243 hardware.
class DRV8873 : public DRV8243 {
public:
    using Config = DRV8243::Config;
    explicit DRV8873(const Config& config) : DRV8243(config) {}
};
