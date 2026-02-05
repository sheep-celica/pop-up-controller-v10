#include "ADS7138.h"

ADS7138::ADS7138(uint8_t i2cAddr, float avddVolts)
: _addr(i2cAddr), _avdd(avddVolts) {}

bool ADS7138::begin(TwoWire& wire) {
  _wire = &wire;

  // Put device into a simple, known configuration:
  // - GENERAL_CFG = 0x00 (CRC off, stats off, no reset, etc.)
  if (!writeReg(REG_GENERAL_CFG, 0x00)) return false;

  // - DATA_CFG = 0x00 (no appended status/channel id; simplest 2-byte frames)
  if (!writeReg(REG_DATA_CFG, 0x00)) return false;

  // - Manual conversion mode (OPMODE_CFG default 0x00 is fine)
  if (!writeReg(REG_OPMODE_CFG, 0x00)) return false;

  // - Manual sequencing mode (SEQ_MODE=00, SEQ_START=0)
  if (!writeReg(REG_SEQUENCE_CFG, 0x00)) return false;

  // Apply oversampling (OSR_CFG[2:0])
  if (!writeReg(REG_OSR_CFG, (uint8_t)_osr & 0x07)) return false;

  // Apply cached pin configuration (defaults = all analog inputs).
  if (!applyPinConfig()) return false;

  return true;
}

void ADS7138::setAnalogInput(uint8_t ch) {
  if (ch > 7) return;

  // Analog input => PIN_CFG bit = 0 (not GPIO)
  _pinCfg &= ~(1u << ch);

  // Clear direction/drive bits for cleanliness
  _gpioCfg &= ~(1u << ch);
  _gpoDriveCfg &= ~(1u << ch);
}

void ADS7138::setDigitalOutput(uint8_t ch, bool pushPull) {
  if (ch > 7) return;

  // GPIO output:
  _pinCfg |= (1u << ch);   // GPIO
  _gpioCfg |= (1u << ch);  // output
  if (pushPull) _gpoDriveCfg |= (1u << ch);
  else          _gpoDriveCfg &= ~(1u << ch);
}

void ADS7138::setDigitalInput(uint8_t ch) {
  if (ch > 7) return;

  _pinCfg |= (1u << ch);    // GPIO
  _gpioCfg &= ~(1u << ch);  // input
  _gpoDriveCfg &= ~(1u << ch);
}

bool ADS7138::applyPinConfig() {
  if (!_wire) return false;

  if (!writeReg(REG_PIN_CFG, _pinCfg)) return false;
  if (!writeReg(REG_GPIO_CFG, _gpioCfg)) return false;
  if (!writeReg(REG_GPO_DRIVE_CFG, _gpoDriveCfg)) return false;

  // Push cached output levels
  if (!writeReg(REG_GPO_VALUE, _gpoValue)) return false;

  return true;
}

bool ADS7138::setOversampling(Oversampling osr) {
  _osr = osr;
  if (!_wire) return true; // cache; begin() will apply later
  return writeReg(REG_OSR_CFG, (uint8_t)_osr & 0x07);
}

bool ADS7138::digitalWrite(uint8_t ch, bool level) {
  if (ch > 7) return false;
  if (!_wire) return false;

  // Must be GPIO + output
  if (((_pinCfg >> ch) & 0x01) == 0) return false;
  if (((_gpioCfg >> ch) & 0x01) == 0) return false;

  if (level) _gpoValue |= (1u << ch);
  else       _gpoValue &= ~(1u << ch);

  return writeReg(REG_GPO_VALUE, _gpoValue);
}

bool ADS7138::digitalRead(uint8_t ch) {
  if (ch > 7) return false;
  if (!_wire) return false;

  uint8_t v = readReg(REG_GPI_VALUE);
  return ((v >> ch) & 0x01) != 0;
}

uint16_t ADS7138::readAnalogRaw(uint8_t ch, bool discardFirstAfterMuxChange) {
  if (ch > 7) return 0;
  if (!_wire) return 0;

  // Only valid as ADC if not configured as GPIO.
  if (((_pinCfg >> ch) & 0x01) != 0) return 0;

  if (!setManualChannel(ch)) return 0;

  // Optional: discard first sample after mux switch to allow settling
  if (discardFirstAfterMuxChange && _lastAnalogCh != (int8_t)ch) {
    if ((uint8_t)_osr == 0) (void)readFrameA12();
    else                   (void)readFrameB16();
  }
  _lastAnalogCh = (int8_t)ch;

  // OSR_1 -> Frame A 12-bit, else Frame B 16-bit averaged
  if ((uint8_t)_osr == 0) return readFrameA12();
  return readFrameB16();
}

float ADS7138::readAnalogVolts(uint8_t ch, bool discardFirstAfterMuxChange) {
  uint16_t code = readAnalogRaw(ch, discardFirstAfterMuxChange);

  if ((uint8_t)_osr == 0) {
    // 12-bit (0..4095)
    return (float)code * _avdd / 4095.0f;
  } else {
    // 16-bit averaged (0..65535)
    return (float)code * _avdd / 65535.0f;
  }
}

bool ADS7138::setManualChannel(uint8_t ch) {
  return writeReg(REG_MANUAL_CH_SEL, (ch & 0x0F));
}

uint16_t ADS7138::readFrameA12() {
  // Request 2 bytes from device; conversion happens and device may stretch SCL until ready.
  _wire->requestFrom(_addr, (uint8_t)2);
  if (_wire->available() < 2) return 0;

  uint8_t b1 = _wire->read();
  uint8_t b2 = _wire->read();

  // Frame A packing: [D11..D4] then [D3..D0 xxxx]
  return ((uint16_t)b1 << 4) | (b2 >> 4);
}

uint16_t ADS7138::readFrameB16() {
  // Averaging enabled: Frame B returns 16-bit average as two bytes.
  _wire->requestFrom(_addr, (uint8_t)2);
  if (_wire->available() < 2) return 0;

  uint8_t hi = _wire->read();
  uint8_t lo = _wire->read();
  return ((uint16_t)hi << 8) | lo;
}

bool ADS7138::writeReg(uint8_t reg, uint8_t val) {
  _wire->beginTransmission(_addr);
  _wire->write(OP_WR1);
  _wire->write(reg);
  _wire->write(val);
  return (_wire->endTransmission(true) == 0);
}

uint8_t ADS7138::readReg(uint8_t reg) {
  _wire->beginTransmission(_addr);
  _wire->write(OP_RD1);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return 0;

  _wire->requestFrom(_addr, (uint8_t)1);
  if (_wire->available() < 1) return 0;
  return _wire->read();
}
