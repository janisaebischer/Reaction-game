#ifndef I2C_GPIO_H
#define I2C_GPIO_H

#include <Arduino.h>
#include <Wire.h>
#include <array>

#include "settings.h"

class I2C_GPIO {
 public:
  bool begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_HZ);

    ready_ = false;
    chipPresent_.fill(false);
    outputState_ = 0;
    inputState_ = 0;

    for (uint8_t chip = 0; chip < I2C_MCP23017_DEVICE_COUNT; ++chip) {
      const uint8_t address = I2C_MCP23017_BASE_ADDR + chip;
      if (!probeChip(address)) {
        continue;
      }

      if (!configureChip(address, chip == 0)) {
        continue;
      }

      chipPresent_[chip] = true;
      ready_ = true;
    }

    refreshInputs();
    return ready_;
  }

  bool isReady() const {
    return ready_;
  }

  void refreshInputs() {
    if (!chipPresent_[1]) return;

    inputState_ = readChipGpio(1);
  }

  bool buttonPressed(uint8_t index) const {
    if (index >= GAME_BUTTON_COUNT) return false;

    const bool raw = (inputState_ >> index) & 0x01;

    return GAME_BUTTON_ACTIVE_LOW ? !raw : raw;
  }

  bool powerSwitchOn() const {
    const bool raw = digitalRead(GAME_POWER_SWITCH_PIN);
    return GAME_CONTROL_ACTIVE_LOW ? !raw : raw;
  }

  bool setLed(uint8_t index, bool enabled) {
    if (index >= GAME_LED_COUNT || !chipPresent_[0]) {
      return false;
    }

    if (enabled) {
      outputState_ |= (1u << index);
    } else {
      outputState_ &= ~(1u << index);
    }

    return writeOutputs();
  }

  bool setOutputChannel(uint8_t index, bool enabled, bool activeLow) {
    if (index >= 16 || !chipPresent_[0]) {
      return false;
    }

    const bool level = activeLow ? !enabled : enabled;
    if (level) {
      outputState_ |= (1u << index);
    } else {
      outputState_ &= ~(1u << index);
    }

    return writeOutputs();
  }

  void allOff() {
    outputState_ = 0;
    if (chipPresent_[0]) {
      writeOutputs();
    }
  }

 private:
  static constexpr uint8_t MCP23017_IODIRA = 0x00;
  static constexpr uint8_t MCP23017_IODIRB = 0x01;
  static constexpr uint8_t MCP23017_GPPUA = 0x0C;
  static constexpr uint8_t MCP23017_GPPUB = 0x0D;
  static constexpr uint8_t MCP23017_GPIOA = 0x12;
  static constexpr uint8_t MCP23017_OLATA = 0x14;
  static constexpr uint8_t MCP23017_OLATB = 0x15;

  bool ready_ = false;
  std::array<bool, I2C_MCP23017_DEVICE_COUNT> chipPresent_{};
  uint16_t outputState_ = 0;
  uint16_t inputState_ = 0;

  bool probeChip(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
  }

  bool configureChip(uint8_t address, bool outputs) {
    if (outputs) {
      return writeRegister(address, MCP23017_IODIRA, 0x00) &&
             writeRegister(address, MCP23017_IODIRB, 0x00) &&
             writeRegister(address, MCP23017_GPPUA, 0x00) &&
             writeRegister(address, MCP23017_GPPUB, 0x00) &&
             writeRegister(address, MCP23017_OLATA, 0x00) &&
             writeRegister(address, MCP23017_OLATB, 0x00);
    }

    return writeRegister(address, MCP23017_IODIRA, 0xFF) &&
           writeRegister(address, MCP23017_IODIRB, 0xFF) &&
           writeRegister(address, MCP23017_GPPUA, 0xFF) &&
           writeRegister(address, MCP23017_GPPUB, 0xFF);
  }

  bool writeOutputs() {
    const uint8_t address = I2C_MCP23017_BASE_ADDR + 0;

    return writeRegister(address, MCP23017_OLATA, outputState_ & 0xFF) &&
           writeRegister(address, MCP23017_OLATB, (outputState_ >> 8) & 0xFF);
  }

  uint16_t readChipGpio(uint8_t chip) {
    const uint8_t address = I2C_MCP23017_BASE_ADDR + chip;
    Wire.beginTransmission(address);
    Wire.write(MCP23017_GPIOA);
    if (Wire.endTransmission(false) != 0) {
      return 0xFFFF;
    }

    const uint8_t bytesRequested = Wire.requestFrom(static_cast<int>(address), 2);
    if (bytesRequested < 2) {
      return 0xFFFF;
    }

    const uint8_t gpioa = Wire.read();
    const uint8_t gpiob = Wire.read();
    return static_cast<uint16_t>(gpioa) | (static_cast<uint16_t>(gpiob) << 8);
  }

  bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
  }
};

extern I2C_GPIO i2c_GPIO;

#endif // I2C_GPIO_H