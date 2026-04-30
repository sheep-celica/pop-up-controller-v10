#include "verification/deep_sleep_test.h"

#include <Arduino.h>
#include <esp_sleep.h>

#include "config.h"
#include "services/logging/logging.h"
#include "verification/io_expander_test.h"

namespace {
    constexpr gpio_num_t kWakePin = GPIO_NUM_13;
    constexpr uint8_t kSharedSleepPin = 14;
    constexpr uint8_t kCountdownStartSeconds = 5;
}

void run_deep_sleep_test()
{
    LOG("Deep-sleep verification countdown started. Wake source: GPIO %d LOW.", static_cast<int>(kWakePin));
    write_internal_expander_led(config::pins::internal_expander::INPUT_LED_PIN, true);

    for (int8_t seconds = kCountdownStartSeconds; seconds >= 0; --seconds) {
        LOG("Entering deep sleep in %d.", seconds);
        delay(1000);
    }

    write_internal_expander_led(config::pins::internal_expander::INPUT_LED_PIN, false);
    LOG("Entering deep sleep now. Pull GPIO %d LOW to wake.", static_cast<int>(kWakePin));
    Serial.flush();

    digitalWrite(kSharedSleepPin, LOW);
    pinMode(kSharedSleepPin, OUTPUT);

    esp_sleep_enable_ext0_wakeup(kWakePin, 0);
    esp_deep_sleep_start();
}
