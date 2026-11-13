#include "services/io/remote_module.h"

#include <Arduino.h>
#include <Preferences.h>
#include <RCSwitch.h>
#include <SPI.h>
#include <Wire.h>

#include "config.h"
#include "services/logging/logging.h"

#ifndef POPUP_CONTROLLER_BOARD_USES_EXTERNAL_REMOTE_MODULE

void setup_remote_module() {}
void update_remote_module() {}
bool is_remote_module_supported() { return false; }
bool is_remote_module_io_connected() { return false; }
bool is_remote_module_radio_connected() { return false; }
bool is_remote_module_connected() { return false; }
uint8_t get_remote_module_radio_part_number() { return 0xFF; }
uint8_t get_remote_module_radio_version() { return 0xFF; }
bool read_remote_module_pair_button() { return false; }
bool set_remote_module_pair_led(bool) { return false; }
bool set_remote_module_status_led(bool) { return false; }
bool set_remote_module_debug_led(bool) { return false; }
void log_remote_module_status() { LOG("External remote module: Not supported on this board."); }
bool start_remote_module_pairing() { return false; }
bool is_remote_module_pairing() { return false; }
bool has_paired_remote() { return false; }
uint32_t get_paired_remote_id() { return 0; }
bool test_remote_module_communication_timing(uint16_t) { return false; }
bool test_remote_wake_signal(uint8_t) { return false; }
bool test_remote_carrier_sense(uint8_t) { return false; }
bool test_remote_wor(uint8_t) { return false; }

#else

namespace {
    constexpr uint8_t kPca9536InputRegister = 0x00;
    constexpr uint8_t kPca9536OutputRegister = 0x01;
    constexpr uint8_t kPca9536PolarityRegister = 0x02;
    constexpr uint8_t kPca9536ConfigurationRegister = 0x03;
    constexpr uint8_t kCc1101ReadStatus = 0xC0;
    constexpr uint8_t kCc1101ReadConfig = 0x80;
    constexpr uint8_t kCc1101PartNumberRegister = 0x30;
    constexpr uint8_t kCc1101VersionRegister = 0x31;
    constexpr uint8_t kCc1101RssiRegister = 0x34;
    constexpr uint8_t kCc1101PktstatusRegister = 0x38;
    constexpr uint8_t kCc1101ResetStrobe = 0x30;
    constexpr uint8_t kCc1101RxStrobe = 0x34;
    constexpr uint8_t kCc1101IdleStrobe = 0x36;
    constexpr uint8_t kCc1101WorStrobe = 0x38;
    constexpr uint8_t kCc1101Iocfg2Register = 0x00;
    constexpr uint8_t kCc1101Iocfg1Register = 0x01;
    constexpr uint8_t kCc1101Iocfg0Register = 0x02;
    constexpr uint8_t kCc1101Pktctrl0Register = 0x08;
    constexpr uint8_t kCc1101Fsctrl1Register = 0x0B;
    constexpr uint8_t kCc1101Freq2Register = 0x0D;
    constexpr uint8_t kCc1101Mdmcfg4Register = 0x10;
    constexpr uint8_t kCc1101Mcsm0Register = 0x18;
    constexpr uint8_t kCc1101Mcsm2Register = 0x16;
    constexpr uint8_t kCc1101FoccfgRegister = 0x19;
    constexpr uint8_t kCc1101BscfgRegister = 0x1A;
    constexpr uint8_t kCc1101Agcctrl2Register = 0x1B;
    constexpr uint8_t kCc1101Agcctrl1Register = 0x1C;
    constexpr uint8_t kCc1101Frend1Register = 0x21;
    constexpr uint8_t kCc1101Fscal3Register = 0x23;
    constexpr uint8_t kCc1101Worevt1Register = 0x1E;
    constexpr uint8_t kCc1101Worevt0Register = 0x1F;
    constexpr uint8_t kCc1101WorctrlRegister = 0x20;
    constexpr uint8_t kCc1101Test2Register = 0x2C;
    constexpr uint8_t kCc1101SerialDataAsync = 0x0D;
    // GDO2 drives an LED wired from 3.3 V through a resistor into the pin, so
    // inverted carrier sense makes the LED active-low: off when idle, on for RF.
    constexpr uint8_t kCc1101CarrierSenseInverted = 0x4E;
    constexpr uint8_t kCc1101GdoHighImpedance = 0x2E;
    constexpr uint8_t kCc1101PktstatusCarrierSenseMask = 0x40;
    constexpr uint8_t kCc1101PktstatusCcaMask = 0x10;
    constexpr uint8_t kCc1101PktstatusGdo2Mask = 0x04;
    constexpr uint32_t kCc1101CrystalFrequencyHz = 26'000'000u;
    constexpr uint32_t kEv1527TransmitterIdMask = 0x000FFFFFul;
    constexpr char kRemotePreferencesNamespace[] = "remote_radio";
    constexpr char kPairedRemoteIdKey[] = "ev1527_id";

    enum class PairLedMode : uint8_t { Off, Pairing, Success, Failure };

    bool s_io_connected = false;
    bool s_radio_connected = false;
    uint8_t s_io_output_latch = 0;
    uint8_t s_radio_part_number = 0xFF;
    uint8_t s_radio_version = 0xFF;
    Preferences s_remote_preferences;
    RCSwitch s_remote_receiver;
    bool s_preferences_ready = false;
    uint32_t s_paired_remote_id = 0;
    PairLedMode s_pair_led_mode = PairLedMode::Off;
    uint32_t s_pairing_deadline_ms = 0;
    uint32_t s_pair_led_changed_ms = 0;
    uint8_t s_success_led_transitions_remaining = 0;
    bool s_pair_led_on = false;
    bool s_pair_button_raw = false;
    bool s_pair_button_stable = false;
    uint32_t s_pair_button_changed_ms = 0;
    uint32_t s_pair_button_last_poll_ms = 0;
    bool s_debug_frame_led_active = false;
    bool s_status_frame_led_active = false;
    uint32_t s_debug_frame_led_deadline_ms = 0;
    uint32_t s_status_frame_led_deadline_ms = 0;
    bool s_frame_indication_has_run = false;
    uint32_t s_last_frame_indication_ms = 0;

    uint8_t bit_for_pin(uint8_t pin) { return static_cast<uint8_t>(1u << pin); }

    bool write_pca9536_register(uint8_t reg, uint8_t value)
    {
        Wire.beginTransmission(config::pins::remote_module::IO_EXPANDER_I2C_ADDRESS);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    bool read_pca9536_register(uint8_t reg, uint8_t& value)
    {
        Wire.beginTransmission(config::pins::remote_module::IO_EXPANDER_I2C_ADDRESS);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return false;
        if (Wire.requestFrom(
                config::pins::remote_module::IO_EXPANDER_I2C_ADDRESS,
                static_cast<uint8_t>(1)) != 1 || Wire.available() < 1) {
            return false;
        }
        value = Wire.read();
        return true;
    }

    bool initialize_pca9536()
    {
        // Load inactive LED levels before enabling P1-P3 as outputs, avoiding a flash.
        s_io_output_latch = 0x00;
        if (!write_pca9536_register(kPca9536OutputRegister, s_io_output_latch) ||
            !write_pca9536_register(kPca9536PolarityRegister, 0x00) ||
            !write_pca9536_register(kPca9536ConfigurationRegister, 0x01)) {
            return false;
        }

        uint8_t output = 0xFF;
        uint8_t configuration = 0xFF;
        return read_pca9536_register(kPca9536OutputRegister, output) &&
               read_pca9536_register(kPca9536ConfigurationRegister, configuration) &&
               (output & 0x0F) == 0x00 &&
               (configuration & 0x0F) == 0x01;
    }

    bool set_remote_module_led(uint8_t pin, bool on)
    {
        if (!s_io_connected) return false;
        const uint8_t mask = bit_for_pin(pin);
        if (on == config::pins::remote_module::LED_ACTIVE_HIGH) {
            s_io_output_latch |= mask;
        } else {
            s_io_output_latch &= static_cast<uint8_t>(~mask);
        }
        if (write_pca9536_register(kPca9536OutputRegister, s_io_output_latch)) return true;
        s_io_connected = false;
        s_radio_connected = false;
        return false;
    }

    bool wait_for_cc1101_ready()
    {
        const uint32_t started_us = micros();
        while (digitalRead(config::pins::remote_module::CC1101_MISO) != LOW) {
            if (static_cast<uint32_t>(micros() - started_us) >=
                config::pins::remote_module::CC1101_READY_TIMEOUT_US) {
                return false;
            }
            yield();
        }
        return true;
    }

    bool reset_cc1101()
    {
        // Establish the levels required by the CC1101 manual-reset sequence before
        // handing SCLK and MOSI to the ESP32 SPI peripheral.
        pinMode(config::pins::remote_module::CC1101_CS, OUTPUT);
        pinMode(config::pins::remote_module::CC1101_SCLK, OUTPUT);
        pinMode(config::pins::remote_module::CC1101_MOSI, OUTPUT);
        pinMode(config::pins::remote_module::CC1101_MISO, INPUT);
        digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
        digitalWrite(config::pins::remote_module::CC1101_SCLK, HIGH);
        digitalWrite(config::pins::remote_module::CC1101_MOSI, LOW);
        delayMicroseconds(5);
        digitalWrite(config::pins::remote_module::CC1101_CS, LOW);
        delayMicroseconds(10);
        digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
        delayMicroseconds(45);

        SPI.begin(
            config::pins::remote_module::CC1101_SCLK,
            config::pins::remote_module::CC1101_MISO,
            config::pins::remote_module::CC1101_MOSI,
            config::pins::remote_module::CC1101_CS);
        SPI.beginTransaction(SPISettings(
            config::pins::remote_module::CC1101_SPI_FREQUENCY_HZ,
            MSBFIRST,
            SPI_MODE0));
        digitalWrite(config::pins::remote_module::CC1101_CS, LOW);
        if (!wait_for_cc1101_ready()) {
            digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
            SPI.endTransaction();
            return false;
        }
        SPI.transfer(kCc1101ResetStrobe);
        const bool reset_completed = wait_for_cc1101_ready();
        digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
        SPI.endTransaction();
        return reset_completed;
    }

    bool read_cc1101_status_register(uint8_t address, uint8_t& value)
    {
        SPI.beginTransaction(SPISettings(
            config::pins::remote_module::CC1101_SPI_FREQUENCY_HZ,
            MSBFIRST,
            SPI_MODE0));
        digitalWrite(config::pins::remote_module::CC1101_CS, LOW);
        if (!wait_for_cc1101_ready()) {
            digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
            SPI.endTransaction();
            return false;
        }
        SPI.transfer(static_cast<uint8_t>(address | kCc1101ReadStatus));
        value = SPI.transfer(0x00);
        digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
        SPI.endTransaction();
        return true;
    }

    bool read_cc1101_config_register(uint8_t address, uint8_t& value)
    {
        SPI.beginTransaction(SPISettings(
            config::pins::remote_module::CC1101_SPI_FREQUENCY_HZ,
            MSBFIRST,
            SPI_MODE0));
        digitalWrite(config::pins::remote_module::CC1101_CS, LOW);
        if (!wait_for_cc1101_ready()) {
            digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
            SPI.endTransaction();
            return false;
        }
        SPI.transfer(static_cast<uint8_t>(address | kCc1101ReadConfig));
        value = SPI.transfer(0x00);
        digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
        SPI.endTransaction();
        return true;
    }

    bool write_cc1101_register(uint8_t address, uint8_t value)
    {
        SPI.beginTransaction(SPISettings(
            config::pins::remote_module::CC1101_SPI_FREQUENCY_HZ,
            MSBFIRST,
            SPI_MODE0));
        digitalWrite(config::pins::remote_module::CC1101_CS, LOW);
        if (!wait_for_cc1101_ready()) {
            digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
            SPI.endTransaction();
            return false;
        }
        SPI.transfer(address);
        SPI.transfer(value);
        digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
        SPI.endTransaction();
        return true;
    }

    bool strobe_cc1101(uint8_t command)
    {
        SPI.beginTransaction(SPISettings(
            config::pins::remote_module::CC1101_SPI_FREQUENCY_HZ,
            MSBFIRST,
            SPI_MODE0));
        digitalWrite(config::pins::remote_module::CC1101_CS, LOW);
        if (!wait_for_cc1101_ready()) {
            digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
            SPI.endTransaction();
            return false;
        }
        SPI.transfer(command);
        digitalWrite(config::pins::remote_module::CC1101_CS, HIGH);
        SPI.endTransaction();
        return true;
    }

    bool configure_cc1101_ev1527_receiver()
    {
        // 433.92 MHz, ASK/OOK, wide enough for common EV1527 pulse-rate variants.
        // Asynchronous serial data is exposed on GDO1/SO while CS remains high.
        static const uint8_t registers[][2] = {
            {kCc1101Iocfg2Register, kCc1101CarrierSenseInverted},
            {kCc1101Iocfg1Register, kCc1101SerialDataAsync},
            {kCc1101Iocfg0Register, kCc1101GdoHighImpedance},
            {kCc1101Pktctrl0Register, 0x32},
            {kCc1101Fsctrl1Register, 0x06},
            // 433.92 MHz with the CC1101's 26 MHz reference crystal.
            {kCc1101Freq2Register + 0, 0x10},
            {kCc1101Freq2Register + 1, 0xB0},
            {kCc1101Freq2Register + 2, 0x71},
            // ~203 kHz RX channel bandwidth. Cheap 433.92 MHz SAW keyfobs can
            // have substantially more carrier error than the previous ~58 kHz
            // bandwidth allowed, especially across temperature and battery age.
            {kCc1101Mdmcfg4Register + 0, 0x87},
            {kCc1101Mdmcfg4Register + 1, 0x83},
            {kCc1101Mdmcfg4Register + 2, 0x30},
            {kCc1101Mdmcfg4Register + 3, 0x22},
            {kCc1101Mdmcfg4Register + 4, 0xF8},
            {kCc1101Mcsm0Register, 0x18},
            {kCc1101Mcsm2Register, 0x07},
            {kCc1101FoccfgRegister, 0x16},
            {kCc1101BscfgRegister, 0x6C},
            {kCc1101Agcctrl2Register + 0, 0x03},
            // Preserve LNA priority, require a 10 dB RSSI rise for carrier
            // sense, and disable the absolute threshold. The previous maximum
            // +7 dB absolute adjustment still asserted on the idle noise floor.
            {kCc1101Agcctrl2Register + 1, 0x68},
            {kCc1101Agcctrl2Register + 2, 0x91},
            {kCc1101Frend1Register + 0, 0x56},
            {kCc1101Frend1Register + 1, 0x11},
            {kCc1101Fscal3Register + 0, 0xE9},
            {kCc1101Fscal3Register + 1, 0x2A},
            {kCc1101Fscal3Register + 2, 0x00},
            {kCc1101Fscal3Register + 3, 0x1F},
            {kCc1101Test2Register + 0, 0x81},
            {kCc1101Test2Register + 1, 0x35},
            {kCc1101Test2Register + 2, 0x09},
        };

        if (!strobe_cc1101(kCc1101IdleStrobe)) return false;
        for (const auto& entry : registers) {
            if (!write_cc1101_register(entry[0], entry[1])) return false;
        }
        if (!strobe_cc1101(kCc1101RxStrobe)) return false;
        pinMode(config::pins::remote_module::CC1101_MISO, INPUT);
        s_remote_receiver.enableReceive(
            static_cast<int>(config::pins::remote_module::CC1101_MISO));
        return true;
    }

    bool is_valid_ev1527_frame(uint32_t value, unsigned int bit_length)
    {
        if (bit_length != 24 || value == 0) return false;
        const uint8_t button_code = static_cast<uint8_t>(value & 0x0F);
        return button_code == 0x01 || button_code == 0x02 ||
               button_code == 0x04 || button_code == 0x08;
    }

    void set_pair_led_state(bool on)
    {
        s_pair_led_on = on;
        (void)set_remote_module_pair_led(on);
    }

    void finish_pairing_success(uint32_t transmitter_id, uint8_t button_code)
    {
        s_paired_remote_id = transmitter_id;
        const bool saved = s_preferences_ready &&
            s_remote_preferences.putUInt(kPairedRemoteIdKey, transmitter_id) == sizeof(uint32_t);
        LOG("EV1527 remote paired: id=0x%05lX button=0x%X saved=%s",
            static_cast<unsigned long>(transmitter_id),
            static_cast<unsigned>(button_code),
            saved ? "true" : "false");
        s_pair_led_mode = PairLedMode::Success;
        s_success_led_transitions_remaining =
            static_cast<uint8_t>(config::pins::remote_module::PAIRED_BLINK_COUNT * 2);
        s_pair_led_changed_ms = millis();
        set_pair_led_state(true);
    }

    void finish_pairing_failure()
    {
        LOG("EV1527 remote pairing timed out after 10 seconds.");
        s_pair_led_mode = PairLedMode::Failure;
        s_pair_led_changed_ms = millis();
        set_pair_led_state(true);
    }

    void update_pair_led(uint32_t now_ms)
    {
        if (s_pair_led_mode == PairLedMode::Pairing &&
            static_cast<uint32_t>(now_ms - s_pair_led_changed_ms) >=
                config::pins::remote_module::PAIRING_BLINK_INTERVAL_MS) {
            s_pair_led_changed_ms = now_ms;
            set_pair_led_state(!s_pair_led_on);
        } else if (s_pair_led_mode == PairLedMode::Success &&
                   static_cast<uint32_t>(now_ms - s_pair_led_changed_ms) >=
                       config::pins::remote_module::PAIRED_BLINK_INTERVAL_MS) {
            s_pair_led_changed_ms = now_ms;
            set_pair_led_state(!s_pair_led_on);
            if (s_success_led_transitions_remaining > 0) --s_success_led_transitions_remaining;
            if (s_success_led_transitions_remaining == 0) {
                s_pair_led_mode = PairLedMode::Off;
                set_pair_led_state(false);
            }
        } else if (s_pair_led_mode == PairLedMode::Failure &&
                   static_cast<uint32_t>(now_ms - s_pair_led_changed_ms) >=
                       config::pins::remote_module::PAIRING_FAILED_LED_ON_MS) {
            s_pair_led_mode = PairLedMode::Off;
            set_pair_led_state(false);
        }
    }

    void update_pair_button(uint32_t now_ms)
    {
        constexpr uint32_t kPairButtonPollIntervalMs = 10;
        if (static_cast<uint32_t>(now_ms - s_pair_button_last_poll_ms) <
            kPairButtonPollIntervalMs) {
            return;
        }
        s_pair_button_last_poll_ms = now_ms;
        const bool pressed = read_remote_module_pair_button();
        if (pressed != s_pair_button_raw) {
            s_pair_button_raw = pressed;
            s_pair_button_changed_ms = now_ms;
        }
        if (pressed != s_pair_button_stable &&
            static_cast<uint32_t>(now_ms - s_pair_button_changed_ms) >=
                config::pins::remote_module::PAIR_BUTTON_DEBOUNCE_MS) {
            s_pair_button_stable = pressed;
            if (pressed) (void)start_remote_module_pairing();
        }
    }

    void indicate_received_frame(bool paired, uint32_t now_ms)
    {
        s_debug_frame_led_active = true;
        s_debug_frame_led_deadline_ms =
            now_ms + config::pins::remote_module::RECEIVED_FRAME_LED_PULSE_MS;
        (void)set_remote_module_debug_led(true);

        if (paired) {
            // The module-connected status indication is normally solid on, so
            // briefly turn it off to make a matched frame visible.
            s_status_frame_led_active = true;
            s_status_frame_led_deadline_ms =
                now_ms + config::pins::remote_module::RECEIVED_FRAME_LED_PULSE_MS;
            (void)set_remote_module_status_led(false);
        }
    }

    void update_received_frame_leds(uint32_t now_ms)
    {
        if (s_debug_frame_led_active &&
            static_cast<int32_t>(now_ms - s_debug_frame_led_deadline_ms) >= 0) {
            s_debug_frame_led_active = false;
            (void)set_remote_module_debug_led(false);
        }
        if (s_status_frame_led_active &&
            static_cast<int32_t>(now_ms - s_status_frame_led_deadline_ms) >= 0) {
            s_status_frame_led_active = false;
            (void)set_remote_module_status_led(s_radio_connected);
        }
    }

    bool initialize_cc1101()
    {
        if (!reset_cc1101()) return false;
        if (!read_cc1101_status_register(kCc1101PartNumberRegister, s_radio_part_number) ||
            !read_cc1101_status_register(kCc1101VersionRegister, s_radio_version)) {
            return false;
        }
        return s_radio_part_number == config::pins::remote_module::CC1101_EXPECTED_PART_NUMBER &&
               s_radio_version != 0x00 && s_radio_version != 0xFF;
    }
}

void setup_remote_module()
{
    s_io_connected = initialize_pca9536();
    s_radio_connected = initialize_cc1101();
    if (s_radio_connected) {
        s_radio_connected = configure_cc1101_ev1527_receiver();
    }
    s_preferences_ready = s_remote_preferences.begin(kRemotePreferencesNamespace, false);
    if (s_preferences_ready) {
        s_paired_remote_id =
            s_remote_preferences.getUInt(kPairedRemoteIdKey, 0) & kEv1527TransmitterIdMask;
    }
    if (s_io_connected) (void)set_remote_module_status_led(s_radio_connected);
    log_remote_module_status();
}

void update_remote_module()
{
    if (!is_remote_module_connected()) return;
    const uint32_t now_ms = millis();
    update_pair_button(now_ms);

    if (s_pair_led_mode == PairLedMode::Pairing &&
        static_cast<int32_t>(now_ms - s_pairing_deadline_ms) >= 0) {
        finish_pairing_failure();
    }

    if (s_remote_receiver.available()) {
        const uint32_t value = s_remote_receiver.getReceivedValue();
        const unsigned int bit_length = s_remote_receiver.getReceivedBitlength();
        const unsigned int protocol = s_remote_receiver.getReceivedProtocol();
        const unsigned int pulse_length = s_remote_receiver.getReceivedDelay();
        s_remote_receiver.resetAvailable();

        if (is_valid_ev1527_frame(value, bit_length)) {
            const uint32_t transmitter_id = (value >> 4) & kEv1527TransmitterIdMask;
            const uint8_t button_code = static_cast<uint8_t>(value & 0x0F);
            const bool paired = transmitter_id == s_paired_remote_id &&
                s_paired_remote_id != 0;
            const bool indication_due = !s_frame_indication_has_run ||
                static_cast<uint32_t>(now_ms - s_last_frame_indication_ms) >=
                    config::pins::remote_module::RECEIVED_FRAME_LED_INTERVAL_MS;
            if (indication_due) {
                s_frame_indication_has_run = true;
                s_last_frame_indication_ms = now_ms;
                indicate_received_frame(paired, now_ms);
            }
            LOG("EV1527 received: id=0x%05lX button=0x%X protocol=%u pulse_us=%u paired=%s",
                static_cast<unsigned long>(transmitter_id),
                static_cast<unsigned>(button_code), protocol, pulse_length,
                paired ? "true" : "false");
            if (s_pair_led_mode == PairLedMode::Pairing) {
                finish_pairing_success(transmitter_id, button_code);
            }
        }
    }
    update_pair_led(now_ms);
    update_received_frame_leds(now_ms);
}

bool is_remote_module_supported() { return true; }
bool is_remote_module_io_connected() { return s_io_connected; }
bool is_remote_module_radio_connected() { return s_radio_connected; }
bool is_remote_module_connected() { return s_io_connected && s_radio_connected; }
uint8_t get_remote_module_radio_part_number() { return s_radio_part_number; }
uint8_t get_remote_module_radio_version() { return s_radio_version; }

bool read_remote_module_pair_button()
{
    if (!s_io_connected) return false;
    uint8_t inputs = 0xFF;
    if (!read_pca9536_register(kPca9536InputRegister, inputs)) return false;
    const bool high = (inputs & bit_for_pin(config::pins::remote_module::PAIR_BUTTON_PIN)) != 0;
    return config::pins::remote_module::PAIR_BUTTON_ACTIVE_LOW ? !high : high;
}

bool set_remote_module_pair_led(bool on)
{
    return set_remote_module_led(config::pins::remote_module::PAIR_LED_PIN, on);
}
bool set_remote_module_status_led(bool on)
{
    return set_remote_module_led(config::pins::remote_module::STATUS_LED_PIN, on);
}
bool set_remote_module_debug_led(bool on)
{
    return set_remote_module_led(config::pins::remote_module::DEBUG_LED_PIN, on);
}

bool start_remote_module_pairing()
{
    if (!is_remote_module_connected()) return false;
    s_pair_led_mode = PairLedMode::Pairing;
    s_pairing_deadline_ms = millis() + config::pins::remote_module::PAIRING_TIMEOUT_MS;
    s_pair_led_changed_ms = millis();
    set_pair_led_state(true);
    LOG("EV1527 pairing started; press and hold any remote button (10 second timeout).");
    return true;
}

bool is_remote_module_pairing()
{
    return s_pair_led_mode == PairLedMode::Pairing;
}

bool has_paired_remote()
{
    return s_paired_remote_id != 0;
}

uint32_t get_paired_remote_id()
{
    return s_paired_remote_id;
}

bool test_remote_module_communication_timing(uint16_t samples)
{
    if (!is_remote_module_connected() || samples == 0 || is_remote_module_pairing()) {
        return false;
    }

    uint32_t i2c_total_us = 0;
    uint32_t i2c_min_us = UINT32_MAX;
    uint32_t i2c_max_us = 0;
    uint16_t i2c_successes = 0;
    for (uint16_t i = 0; i < samples; ++i) {
        uint8_t inputs = 0;
        const uint32_t started_us = micros();
        const bool success = read_pca9536_register(kPca9536InputRegister, inputs);
        const uint32_t elapsed_us = static_cast<uint32_t>(micros() - started_us);
        if (!success) continue;
        ++i2c_successes;
        i2c_total_us += elapsed_us;
        if (elapsed_us < i2c_min_us) i2c_min_us = elapsed_us;
        if (elapsed_us > i2c_max_us) i2c_max_us = elapsed_us;
    }

    // GDO1 shares the SPI MISO pin. Detach the edge-capture interrupt while CS
    // is low so SPI response transitions cannot enter the EV1527 decoder.
    s_remote_receiver.disableReceive();
    uint32_t spi_total_us = 0;
    uint32_t spi_min_us = UINT32_MAX;
    uint32_t spi_max_us = 0;
    uint16_t spi_successes = 0;
    for (uint16_t i = 0; i < samples; ++i) {
        uint8_t part_number = 0;
        const uint32_t started_us = micros();
        const bool success =
            read_cc1101_status_register(kCc1101PartNumberRegister, part_number);
        const uint32_t elapsed_us = static_cast<uint32_t>(micros() - started_us);
        if (!success) continue;
        ++spi_successes;
        spi_total_us += elapsed_us;
        if (elapsed_us < spi_min_us) spi_min_us = elapsed_us;
        if (elapsed_us > spi_max_us) spi_max_us = elapsed_us;
    }
    pinMode(config::pins::remote_module::CC1101_MISO, INPUT);
    s_remote_receiver.enableReceive(
        static_cast<int>(config::pins::remote_module::CC1101_MISO));

    if (i2c_successes == 0 || spi_successes == 0) {
        LOG("REMOTE_MODULE_TIMING samples=%u i2c_success=%u spi_success=%u",
            static_cast<unsigned>(samples),
            static_cast<unsigned>(i2c_successes),
            static_cast<unsigned>(spi_successes));
        return false;
    }

    LOG("REMOTE_MODULE_TIMING bus=I2C device=PCA9536 samples=%u success=%u avg_us=%lu min_us=%lu max_us=%lu",
        static_cast<unsigned>(samples), static_cast<unsigned>(i2c_successes),
        static_cast<unsigned long>(i2c_total_us / i2c_successes),
        static_cast<unsigned long>(i2c_min_us),
        static_cast<unsigned long>(i2c_max_us));
    LOG("REMOTE_MODULE_TIMING bus=SPI device=CC1101 samples=%u success=%u avg_us=%lu min_us=%lu max_us=%lu",
        static_cast<unsigned>(samples), static_cast<unsigned>(spi_successes),
        static_cast<unsigned long>(spi_total_us / spi_successes),
        static_cast<unsigned long>(spi_min_us),
        static_cast<unsigned long>(spi_max_us));
    return true;
}

bool test_remote_wake_signal(uint8_t duration_seconds)
{
    if (!is_remote_module_connected() || duration_seconds == 0 ||
        is_remote_module_pairing() ||
        config::pins::power::DEEP_SLEEP_WAKE_PIN == GPIO_NUM_NC) {
        return false;
    }

    const gpio_num_t wake_pin = config::pins::power::DEEP_SLEEP_WAKE_PIN;

    // GDO1 is also SPI MISO. Ignore SPI transitions while changing GDO0.
    s_remote_receiver.disableReceive();
    const bool configured = write_cc1101_register(
        kCc1101Iocfg0Register, kCc1101CarrierSenseInverted);
    pinMode(config::pins::remote_module::CC1101_MISO, INPUT);
    s_remote_receiver.enableReceive(
        static_cast<int>(config::pins::remote_module::CC1101_MISO));
    if (!configured) return false;

    pinMode(wake_pin, INPUT);
    LOG("REMOTE_WAKE_TEST_BEGIN duration_s=%u wake_gpio=%u idle_level=%s",
        static_cast<unsigned>(duration_seconds),
        static_cast<unsigned>(wake_pin),
        digitalRead(wake_pin) == LOW ? "LOW" : "HIGH");
    LOG("Press and hold the remote during the measurement window.");
    Serial.flush();

    const uint32_t test_started_us = micros();
    const uint32_t test_duration_us =
        static_cast<uint32_t>(duration_seconds) * 1'000'000u;
    bool previous_low = digitalRead(wake_pin) == LOW;
    uint32_t low_started_us = previous_low ? test_started_us : 0;
    uint32_t low_total_us = 0;
    uint32_t low_min_us = UINT32_MAX;
    uint32_t low_max_us = 0;
    uint32_t low_pulse_count = previous_low ? 1u : 0u;
    while (static_cast<uint32_t>(micros() - test_started_us) < test_duration_us) {
        const uint32_t now_us = micros();
        const bool low = digitalRead(wake_pin) == LOW;
        if (low && !previous_low) {
            low_started_us = now_us;
            ++low_pulse_count;
        } else if (!low && previous_low) {
            const uint32_t pulse_us = static_cast<uint32_t>(now_us - low_started_us);
            low_total_us += pulse_us;
            if (pulse_us < low_min_us) low_min_us = pulse_us;
            if (pulse_us > low_max_us) low_max_us = pulse_us;
        }
        previous_low = low;
        yield();
    }

    if (previous_low) {
        const uint32_t pulse_us = static_cast<uint32_t>(micros() - low_started_us);
        low_total_us += pulse_us;
        if (pulse_us < low_min_us) low_min_us = pulse_us;
        if (pulse_us > low_max_us) low_max_us = pulse_us;
    }

    s_remote_receiver.disableReceive();
    const bool restored = write_cc1101_register(
        kCc1101Iocfg0Register, kCc1101GdoHighImpedance);
    pinMode(config::pins::remote_module::CC1101_MISO, INPUT);
    s_remote_receiver.enableReceive(
        static_cast<int>(config::pins::remote_module::CC1101_MISO));

    if (low_pulse_count == 0) {
        LOG("REMOTE_WAKE_TEST_END pulses=0 total_low_us=0 final_level=%s restored=%s",
            digitalRead(wake_pin) == LOW ? "LOW" : "HIGH",
            restored ? "true" : "false");
        return restored;
    }

    LOG("REMOTE_WAKE_TEST_END pulses=%lu total_low_us=%lu avg_low_us=%lu min_low_us=%lu max_low_us=%lu final_level=%s restored=%s",
        static_cast<unsigned long>(low_pulse_count),
        static_cast<unsigned long>(low_total_us),
        static_cast<unsigned long>(low_total_us / low_pulse_count),
        static_cast<unsigned long>(low_min_us),
        static_cast<unsigned long>(low_max_us),
        digitalRead(wake_pin) == LOW ? "LOW" : "HIGH",
        restored ? "true" : "false");
    return restored;
}

bool test_remote_carrier_sense(uint8_t duration_seconds)
{
    if (!is_remote_module_connected() || duration_seconds == 0 ||
        is_remote_module_pairing()) {
        return false;
    }

    constexpr uint32_t kSampleIntervalMs = 10;
    constexpr uint32_t kReportIntervalMs = 250;
    constexpr float kRssiOffsetDb = 74.0f;

    // Status reads use GDO1 as SPI MISO. Pause its radio-edge interrupt so the
    // SPI response bits cannot be interpreted as asynchronous EV1527 data.
    s_remote_receiver.disableReceive();
    LOG("REMOTE_CS_TEST_BEGIN duration_s=%u sample_ms=%lu report_ms=%lu",
        static_cast<unsigned>(duration_seconds),
        static_cast<unsigned long>(kSampleIntervalMs),
        static_cast<unsigned long>(kReportIntervalMs));
    LOG("Leave the remote idle briefly, then hold one button to create a visible RSSI step.");
    Serial.flush();

    const uint32_t started_ms = millis();
    uint32_t report_started_ms = started_ms;
    uint32_t successful_samples = 0;
    uint32_t failed_samples = 0;
    uint32_t carrier_sense_samples = 0;
    uint32_t cca_samples = 0;
    uint32_t gdo2_high_samples = 0;
    int32_t rssi_raw_total = 0;
    int8_t rssi_raw_min = INT8_MAX;
    int8_t rssi_raw_max = INT8_MIN;

    uint8_t iocfg2 = 0xFF;
    uint8_t agcctrl1 = 0xFF;
    const bool config_read =
        read_cc1101_config_register(kCc1101Iocfg2Register, iocfg2) &&
        read_cc1101_config_register(kCc1101Agcctrl2Register + 1, agcctrl1);
    LOG("REMOTE_CS_CONFIG read=%s iocfg2=0x%02X agcctrl1=0x%02X expected_iocfg2=0x%02X expected_agcctrl1=0x68",
        config_read ? "true" : "false", iocfg2, agcctrl1,
        kCc1101CarrierSenseInverted);

    while (static_cast<uint32_t>(millis() - started_ms) <
           static_cast<uint32_t>(duration_seconds) * 1000u) {
        uint8_t raw_rssi_byte = 0;
        uint8_t packet_status = 0;
        const bool success =
            read_cc1101_status_register(kCc1101RssiRegister, raw_rssi_byte) &&
            read_cc1101_status_register(kCc1101PktstatusRegister, packet_status);
        if (success) {
            const int8_t raw_rssi = static_cast<int8_t>(raw_rssi_byte);
            ++successful_samples;
            rssi_raw_total += raw_rssi;
            if (raw_rssi < rssi_raw_min) rssi_raw_min = raw_rssi;
            if (raw_rssi > rssi_raw_max) rssi_raw_max = raw_rssi;
            if ((packet_status & kCc1101PktstatusCarrierSenseMask) != 0) {
                ++carrier_sense_samples;
            }
            if ((packet_status & kCc1101PktstatusCcaMask) != 0) {
                ++cca_samples;
            }
            if ((packet_status & kCc1101PktstatusGdo2Mask) != 0) {
                ++gdo2_high_samples;
            }
        } else {
            ++failed_samples;
        }

        const uint32_t now_ms = millis();
        if (static_cast<uint32_t>(now_ms - report_started_ms) >= kReportIntervalMs) {
            if (successful_samples > 0) {
                const float average_dbm =
                    (static_cast<float>(rssi_raw_total) / successful_samples) / 2.0f -
                    kRssiOffsetDb;
                const float minimum_dbm =
                    static_cast<float>(rssi_raw_min) / 2.0f - kRssiOffsetDb;
                const float maximum_dbm =
                    static_cast<float>(rssi_raw_max) / 2.0f - kRssiOffsetDb;
                LOG("REMOTE_CS t_ms=%lu samples=%lu rssi_avg_dbm=%.1f rssi_min_dbm=%.1f rssi_max_dbm=%.1f cs_percent=%lu cca_percent=%lu gdo2_high_percent=%lu failures=%lu",
                    static_cast<unsigned long>(now_ms - started_ms),
                    static_cast<unsigned long>(successful_samples),
                    average_dbm, minimum_dbm, maximum_dbm,
                    static_cast<unsigned long>(carrier_sense_samples * 100u / successful_samples),
                    static_cast<unsigned long>(cca_samples * 100u / successful_samples),
                    static_cast<unsigned long>(gdo2_high_samples * 100u / successful_samples),
                    static_cast<unsigned long>(failed_samples));
            } else {
                LOG("REMOTE_CS t_ms=%lu samples=0 failures=%lu",
                    static_cast<unsigned long>(now_ms - started_ms),
                    static_cast<unsigned long>(failed_samples));
            }

            report_started_ms = now_ms;
            successful_samples = 0;
            failed_samples = 0;
            carrier_sense_samples = 0;
            cca_samples = 0;
            gdo2_high_samples = 0;
            rssi_raw_total = 0;
            rssi_raw_min = INT8_MAX;
            rssi_raw_max = INT8_MIN;
        }
        delay(kSampleIntervalMs);
    }

    pinMode(config::pins::remote_module::CC1101_MISO, INPUT);
    s_remote_receiver.enableReceive(
        static_cast<int>(config::pins::remote_module::CC1101_MISO));
    LOG("REMOTE_CS_TEST_END reception_restored=true");
    return true;
}

bool test_remote_wor(uint8_t duration_seconds)
{
    if (!is_remote_module_connected() || duration_seconds == 0 ||
        is_remote_module_pairing() ||
        config::pins::power::DEEP_SLEEP_WAKE_PIN == GPIO_NUM_NC) {
        return false;
    }

    // tEvent0 = (750 / fXOSC) * EVENT0 for WOR_RES=0. Round to the
    // nearest supported EVENT0 count and report the resulting real interval.
    const uint64_t numerator =
        static_cast<uint64_t>(config::pins::remote_module::WOR_SAMPLE_INTERVAL_MS) *
        kCc1101CrystalFrequencyHz;
    const uint32_t event0 = static_cast<uint32_t>(
        (numerator + 375'000u) / 750'000u);
    if (event0 == 0 || event0 > 0xFFFFu) return false;
    const uint32_t actual_interval_us = static_cast<uint32_t>(
        (static_cast<uint64_t>(750u) * event0 * 1'000'000u) /
        kCc1101CrystalFrequencyHz);

    // Terminate an idle ASK/OOK receive window after the first valid no-carrier
    // result. A detected carrier keeps RX active for the bounded RX_TIME=0
    // window, reducing both idle power and noise-triggered wake pulses.
    constexpr uint8_t kWorMcsm2 = 0x10;
    // RC oscillator enabled, longest Event1/XOSC-start allowance, automatic
    // RC calibration enabled, WOR_RES=0.
    constexpr uint8_t kWorControl = 0x78;

    s_remote_receiver.disableReceive();
    bool configured = strobe_cc1101(kCc1101IdleStrobe);
    configured = configured && write_cc1101_register(
        kCc1101Mcsm2Register, kWorMcsm2);
    configured = configured && write_cc1101_register(
        kCc1101Worevt1Register, static_cast<uint8_t>(event0 >> 8));
    configured = configured && write_cc1101_register(
        kCc1101Worevt0Register, static_cast<uint8_t>(event0));
    configured = configured && write_cc1101_register(
        kCc1101WorctrlRegister, kWorControl);
    // Keep the normal 10 dB relative carrier threshold for the GDO2 bodge-wire
    // wake test.
    configured = configured && write_cc1101_register(
        kCc1101Agcctrl1Register, 0x68);
    // GDO2 now reaches the active-low wake net through the prototype bodge
    // wire. Leave the original GDO0 wake output high impedance so the two
    // push-pull outputs cannot contend on that shared net.
    configured = configured && write_cc1101_register(
        kCc1101Iocfg0Register, kCc1101GdoHighImpedance);
    uint8_t iocfg0_readback = 0xFF;
    uint8_t iocfg2_readback = 0xFF;
    uint8_t mcsm2_readback = 0xFF;
    uint8_t worctrl_readback = 0xFF;
    uint8_t agcctrl1_readback = 0xFF;
    configured = configured &&
        read_cc1101_config_register(kCc1101Iocfg0Register, iocfg0_readback) &&
        read_cc1101_config_register(kCc1101Iocfg2Register, iocfg2_readback) &&
        read_cc1101_config_register(kCc1101Mcsm2Register, mcsm2_readback) &&
        read_cc1101_config_register(kCc1101WorctrlRegister, worctrl_readback) &&
        read_cc1101_config_register(kCc1101Agcctrl1Register, agcctrl1_readback);
    configured = configured &&
        iocfg0_readback == kCc1101GdoHighImpedance &&
        iocfg2_readback == kCc1101CarrierSenseInverted &&
        mcsm2_readback == kWorMcsm2 &&
        worctrl_readback == kWorControl &&
        agcctrl1_readback == 0x68;
    configured = configured && strobe_cc1101(kCc1101WorStrobe);

    const gpio_num_t wake_pin = config::pins::power::DEEP_SLEEP_WAKE_PIN;
    pinMode(wake_pin, INPUT);
    if (!configured) {
        (void)configure_cc1101_ev1527_receiver();
        return false;
    }

    LOG("REMOTE_WOR_CONFIG mode=gdo2_bodge_10db_early_rssi max_rx_window_us=12500 iocfg0=0x%02X iocfg2=0x%02X mcsm2=0x%02X worctrl=0x%02X agcctrl1=0x%02X",
        iocfg0_readback, iocfg2_readback, mcsm2_readback, worctrl_readback,
        agcctrl1_readback);
    LOG("REMOTE_WOR_TEST_BEGIN duration_s=%u target_interval_ms=%lu event0=0x%04lX actual_interval_us=%lu wake_gpio=%u initial_level=%s",
        static_cast<unsigned>(duration_seconds),
        static_cast<unsigned long>(config::pins::remote_module::WOR_SAMPLE_INTERVAL_MS),
        static_cast<unsigned long>(event0),
        static_cast<unsigned long>(actual_interval_us),
        static_cast<unsigned>(wake_pin),
        digitalRead(wake_pin) == LOW ? "LOW" : "HIGH");
    LOG("Leave the remote idle for 5 seconds, hold a button for 5 seconds, then release it.");
    Serial.flush();

    const uint32_t test_started_us = micros();
    const uint32_t test_duration_us =
        static_cast<uint32_t>(duration_seconds) * 1'000'000u;
    bool previous_low = digitalRead(wake_pin) == LOW;
    uint32_t low_started_us = previous_low ? test_started_us : 0;
    uint32_t low_total_us = 0;
    uint32_t low_min_us = UINT32_MAX;
    uint32_t low_max_us = 0;
    uint32_t low_pulse_count = previous_low ? 1u : 0u;
    uint32_t pulses_per_second[30] = {};
    uint32_t low_us_per_second[30] = {};
    uint8_t low_started_second = 0;
    if (previous_low) pulses_per_second[0] = 1;

    while (static_cast<uint32_t>(micros() - test_started_us) < test_duration_us) {
        const uint32_t now_us = micros();
        const bool low = digitalRead(wake_pin) == LOW;
        if (low && !previous_low) {
            low_started_us = now_us;
            low_started_second = static_cast<uint8_t>(
                (now_us - test_started_us) / 1'000'000u);
            if (low_started_second >= duration_seconds) {
                low_started_second = static_cast<uint8_t>(duration_seconds - 1u);
            }
            ++pulses_per_second[low_started_second];
            ++low_pulse_count;
        } else if (!low && previous_low) {
            const uint32_t pulse_us = static_cast<uint32_t>(now_us - low_started_us);
            low_total_us += pulse_us;
            low_us_per_second[low_started_second] += pulse_us;
            if (pulse_us < low_min_us) low_min_us = pulse_us;
            if (pulse_us > low_max_us) low_max_us = pulse_us;
        }
        previous_low = low;
        yield();
    }
    if (previous_low) {
        const uint32_t pulse_us = static_cast<uint32_t>(micros() - low_started_us);
        low_total_us += pulse_us;
        low_us_per_second[low_started_second] += pulse_us;
        if (pulse_us < low_min_us) low_min_us = pulse_us;
        if (pulse_us > low_max_us) low_max_us = pulse_us;
    }

    const bool restored = configure_cc1101_ev1527_receiver();
    if (restored) {
        pinMode(config::pins::remote_module::CC1101_MISO, INPUT);
        s_remote_receiver.enableReceive(
            static_cast<int>(config::pins::remote_module::CC1101_MISO));
    }

    for (uint8_t second = 0; second < duration_seconds; ++second) {
        LOG("REMOTE_WOR_SECOND second=%u pulses=%lu total_low_us=%lu",
            static_cast<unsigned>(second + 1u),
            static_cast<unsigned long>(pulses_per_second[second]),
            static_cast<unsigned long>(low_us_per_second[second]));
    }

    if (low_pulse_count == 0) {
        LOG("REMOTE_WOR_TEST_END pulses=0 total_low_us=0 final_level=%s restored=%s",
            digitalRead(wake_pin) == LOW ? "LOW" : "HIGH",
            restored ? "true" : "false");
        return restored;
    }
    LOG("REMOTE_WOR_TEST_END pulses=%lu total_low_us=%lu avg_low_us=%lu min_low_us=%lu max_low_us=%lu final_level=%s restored=%s",
        static_cast<unsigned long>(low_pulse_count),
        static_cast<unsigned long>(low_total_us),
        static_cast<unsigned long>(low_total_us / low_pulse_count),
        static_cast<unsigned long>(low_min_us),
        static_cast<unsigned long>(low_max_us),
        digitalRead(wake_pin) == LOW ? "LOW" : "HIGH",
        restored ? "true" : "false");
    return restored;
}

void log_remote_module_status()
{
    LOG("External remote module: %s", is_remote_module_connected() ? "Connected" : "Not Connected");
    LOG("PCA9536 at 0x%02X: %s",
        config::pins::remote_module::IO_EXPANDER_I2C_ADDRESS,
        s_io_connected ? "Connected" : "Not Connected");
    if (s_radio_part_number == 0xFF && s_radio_version == 0xFF) {
        LOG("CC1101: Not Connected");
        return;
    }
    LOG("CC1101: %s, PARTNUM=0x%02X, VERSION=0x%02X",
        s_radio_connected ? "Connected" : "Identification Failed",
        s_radio_part_number,
        s_radio_version);
    if (s_radio_connected) {
        if (has_paired_remote()) {
            LOG("EV1527 remote: Paired, ID=0x%05lX",
                static_cast<unsigned long>(get_paired_remote_id()));
        } else {
            LOG("EV1527 remote: Not Paired");
        }
    }
}

#endif
