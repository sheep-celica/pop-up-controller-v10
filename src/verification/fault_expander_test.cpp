#include "verification/fault_expander_test.h"

#include <Arduino.h>
#include <Wire.h>

#include "services/logging/logging.h"

namespace {
    constexpr uint8_t kFaultExpanderAddress = 0x21;
    constexpr uint8_t kInterruptPin = 27;

    constexpr uint8_t kInputPortRegister = 0x00;
    constexpr uint8_t kPolarityInversionRegister = 0x02;
    constexpr uint8_t kConfigurationRegister = 0x03;

    volatile bool s_interrupt_pending = false;

    void IRAM_ATTR on_fault_expander_interrupt()
    {
        s_interrupt_pending = true;
    }

    bool write_register(uint8_t reg, uint8_t value)
    {
        Wire.beginTransmission(kFaultExpanderAddress);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    bool read_register(uint8_t reg, uint8_t& value)
    {
        Wire.beginTransmission(kFaultExpanderAddress);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) {
            return false;
        }

        const uint8_t bytes_read = Wire.requestFrom(kFaultExpanderAddress, static_cast<uint8_t>(1));
        if (bytes_read != 1 || Wire.available() < 1) {
            return false;
        }

        value = Wire.read();
        return true;
    }

    bool is_fault_expander_present()
    {
        Wire.beginTransmission(kFaultExpanderAddress);
        return Wire.endTransmission() == 0;
    }

    void print_pin_states(uint8_t states)
    {
        LOG(
            "TCA6408A pins: P0=%u P1=%u P2=%u P3=%u P4=%u P5=%u P6=%u P7=%u raw=0x%02X.",
            (states >> 0) & 0x01,
            (states >> 1) & 0x01,
            (states >> 2) & 0x01,
            (states >> 3) & 0x01,
            (states >> 4) & 0x01,
            (states >> 5) & 0x01,
            (states >> 6) & 0x01,
            (states >> 7) & 0x01,
            states);
    }

    bool read_and_print_pin_states()
    {
        uint8_t states = 0;
        if (!read_register(kInputPortRegister, states)) {
            LOG("TCA6408A input read failed.");
            return false;
        }

        print_pin_states(states);
        return true;
    }
}

void setup_fault_expander_test()
{
    const bool present = is_fault_expander_present();
    LOG(
        "Fault TCA6408A expander at 0x%02X: %s.",
        kFaultExpanderAddress,
        present ? "present" : "not present");

    if (!present) {
        return;
    }

    if (!write_register(kPolarityInversionRegister, 0x00)) {
        LOG("TCA6408A polarity setup failed.");
        return;
    }

    if (!write_register(kConfigurationRegister, 0xFF)) {
        LOG("TCA6408A input configuration failed.");
        return;
    }

    pinMode(kInterruptPin, INPUT_PULLUP);
    (void)read_and_print_pin_states();
    s_interrupt_pending = false;
    attachInterrupt(digitalPinToInterrupt(kInterruptPin), on_fault_expander_interrupt, FALLING);

    LOG("Fault TCA6408A interrupt attached on GPIO %u, active-low.", kInterruptPin);
}

void update_fault_expander_test()
{
    if (!s_interrupt_pending) {
        return;
    }

    noInterrupts();
    s_interrupt_pending = false;
    interrupts();

    LOG("Fault TCA6408A interrupt triggered.");
    (void)read_and_print_pin_states();
}

bool read_fault_expander_inputs(uint8_t& states)
{
    return read_register(kInputPortRegister, states);
}
