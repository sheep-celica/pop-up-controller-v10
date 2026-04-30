#include <Arduino.h>
#include <esp_sleep.h>

#include "config.h"
#include "services/logging/logging.h"
#include "verification/deep_sleep_test.h"
#include "verification/fault_expander_test.h"
#include "verification/i2c_test.h"
#include "verification/io_expander_test.h"
#include "verification/motor_current_test.h"
#include "verification/temp_sensor_test.h"

#ifndef BUILD_VERSION
#define BUILD_VERSION "dev"
#endif

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP "unknown"
#endif

namespace {
    constexpr uint8_t kSharedSleepPin = 14;

    void setup_shared_sleep_pin()
    {
        digitalWrite(kSharedSleepPin, HIGH);
        pinMode(kSharedSleepPin, OUTPUT);
        LOG("Shared SLEEP/nSLEEP GPIO %u set HIGH.", kSharedSleepPin);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(300);

    initialize_logging(BUILD_VERSION, BUILD_TIMESTAMP);
    const esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    LOG("Running %s firmware (%s).", config::board::HARDWARE_REVISION, config::board::ID);
    LOG("Wakeup cause: %d.", static_cast<int>(wakeup_cause));
    setup_shared_sleep_pin();
    setup_i2c_verification_bus();
    print_i2c_scan();
    setup_io_expander_test();
    setup_temp_sensor_test();
    setup_fault_expander_test();
    run_motor_current_test();
    run_deep_sleep_test();
}

void loop()
{
    update_fault_expander_test();
}
