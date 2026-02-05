#pragma once
#include <Arduino.h>
#include <Wire.h>

class ADS7138 {
public:
  enum class PinMode : uint8_t {
    AnalogIn = 0,
    DigitalOutOpenDrain,
    DigitalOutPushPull,
    DigitalIn
  };

  // OSR settings: 0 = no averaging (Frame A 12-bit), 1..7 enable averaging (Frame B 16-bit)
  enum class Oversampling : uint8_t {
    OSR_1   = 0, // no averaging
    OSR_2   = 1,
    OSR_4   = 2,
    OSR_8   = 3,
    OSR_16  = 4,
    OSR_32  = 5,
    OSR_64  = 6,
    OSR_128 = 7
  };

  explicit ADS7138(uint8_t i2cAddr = 0x10, float avddVolts = 3.3f);

  // Pass in the TwoWire instance you use (default Wire). You should call Wire.begin() in setup().
  bool begin(TwoWire& wire = Wire);

  // Pin configuration (cached until applyPinConfig() / begin()).
  void setAnalogInput(uint8_t ch);
  void setDigitalOutput(uint8_t ch, bool pushPull = true);
  void setDigitalInput(uint8_t ch);

  // Writes cached pin config to device.
  bool applyPinConfig();

  // Oversampling (cached if begin() not called yet).
  bool setOversampling(Oversampling osr);
  Oversampling getOversampling() const { return _osr; }

  // Analog
  // Returns:
  //  - OSR_1:  12-bit code (0..4095) decoded from Frame A
  //  - OSR>1:  16-bit averaged code (0..65535) decoded from Frame B
  uint16_t readAnalogRaw(uint8_t ch, bool discardFirstAfterMuxChange = true);

  // Returns voltage at the ADS7138 pin.
  float readAnalogVolts(uint8_t ch, bool discardFirstAfterMuxChange = true);

  // Digital
  bool digitalWrite(uint8_t ch, bool level);
  bool digitalRead(uint8_t ch);

  // Optional debug: read back cached config bytes
  uint8_t getPinCfgCached() const { return _pinCfg; }
  uint8_t getGpioCfgCached() const { return _gpioCfg; }
  uint8_t getGpoDriveCfgCached() const { return _gpoDriveCfg; }

private:
  // ---- ADS7138 opcodes ----
  static constexpr uint8_t OP_RD1 = 0x10; // single register read
  static constexpr uint8_t OP_WR1 = 0x08; // single register write

  // ---- ADS7138 registers ----
  static constexpr uint8_t REG_SYSTEM_STATUS   = 0x00;
  static constexpr uint8_t REG_GENERAL_CFG     = 0x01;
  static constexpr uint8_t REG_DATA_CFG        = 0x02;
  static constexpr uint8_t REG_OSR_CFG         = 0x03;
  static constexpr uint8_t REG_OPMODE_CFG      = 0x04;
  static constexpr uint8_t REG_PIN_CFG         = 0x05;
  static constexpr uint8_t REG_GPIO_CFG        = 0x07;
  static constexpr uint8_t REG_GPO_DRIVE_CFG   = 0x09;
  static constexpr uint8_t REG_GPO_VALUE       = 0x0B;
  static constexpr uint8_t REG_GPI_VALUE       = 0x0D;
  static constexpr uint8_t REG_SEQUENCE_CFG    = 0x10;
  static constexpr uint8_t REG_MANUAL_CH_SEL   = 0x11;

  bool writeReg(uint8_t reg, uint8_t val);
  uint8_t readReg(uint8_t reg);

  bool setManualChannel(uint8_t ch);

  // Data frame readers:
  uint16_t readFrameA12(); // 12-bit result packed into 2 bytes (Frame A)
  uint16_t readFrameB16(); // 16-bit averaged result in 2 bytes (Frame B)

private:
  TwoWire* _wire = nullptr;
  uint8_t  _addr = 0x10;
  float    _avdd = 3.3f;

  // Cached config registers
  uint8_t _pinCfg = 0x00;       // 0 = analog input, 1 = GPIO
  uint8_t _gpioCfg = 0x00;      // 0 = input, 1 = output
  uint8_t _gpoDriveCfg = 0x00;  // 0 = open-drain, 1 = push-pull
  uint8_t _gpoValue = 0x00;     // output levels

  Oversampling _osr = Oversampling::OSR_1;

  int8_t _lastAnalogCh = -1;
};
