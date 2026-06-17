# I2C Integration and Master Driver Guide
## CH32V003 I2C ADC Module

This guide details how a Master MCU (e.g., ESP32, STM32, Arduino, Raspberry Pi) communicates with the CH32V003 ADC Module, handles data formatting, and provides software integration examples.

---

## 1. Data Representation and Byte Ordering

All multi-byte data values (such as raw 10-bit values and calibrated millivolts) are stored and transmitted as **Big-Endian** (Most Significant Byte first).

### Byte Assembly Example
When reading a 16-bit register (e.g., `MV_CH0` starting at address `0x06`), the Master receives two bytes:
1. `High Byte` (MSB)
2. `Low Byte` (LSB)

To convert these bytes into a standard 16-bit unsigned integer in your code:
```c
uint8_t msb; // Read from I2C (Register Offset + 0)
uint8_t lsb; // Read from I2C (Register Offset + 1)

uint16_t voltage_mv = ((uint16_t)msb << 8) | lsb;
```

---

## 2. Arduino C++ Driver Example

*Arduino C++ is used here as a reference because it is highly readable and its `<Wire.h>` API maps cleanly to other platforms like ESP32, STM32Duino, and RP2040.*

### 2.1 Driver Header (`CH32V003_ADC.h`)
```cpp
#ifndef CH32V003_ADC_H
#define CH32V003_ADC_H

#include <Arduino.h>
#include <Wire.h>

class CH32V003_ADC {
public:
    // Default I2C address. Warning: change this if it conflicts with other devices on the bus
    static const uint8_t DEFAULT_ADDR = 0x24;

    // Registers
    enum Register : uint8_t {
        REG_CONFIG          = 0x00,
        REG_DEVICE_ADDR     = 0x01,
        REG_VDD_MV          = 0x02,
        REG_RAW_CH0         = 0x04,
        REG_MV_CH0          = 0x06,
        REG_LIMIT_HIGH_CH0  = 0x24,
        REG_LIMIT_LOW_CH0   = 0x26,
        REG_ALERT_STATUS    = 0x44,
        REG_SAVE_SETTINGS   = 0xFE,
        REG_RESET           = 0xFF
    };

    CH32V003_ADC(TwoWire& wire = Wire, uint8_t address = DEFAULT_ADDR);
    bool begin();
    
    uint16_t readVDD();
    uint16_t readRawChannel(uint8_t channel);
    uint16_t readMilliVolts(uint8_t channel);
    
    // Sets filter alpha (0-15) and active channel count (1-8 channels, default 8/0xF)
    bool setFilterAndMask(uint8_t filter_strength, uint8_t channel_mask);
    bool setChannelThresholds(uint8_t channel, uint16_t high_mv, uint16_t low_mv);
    uint8_t readAlerts();
    void clearAlert(uint8_t channel);
    
    void saveSettings();
    void softReset();

private:
    TwoWire& _wire;
    uint8_t _address;
    
    bool writeReg8(uint8_t reg, uint8_t value);
    bool writeReg16(uint8_t reg, uint16_t value);
    uint16_t readReg16(uint8_t reg);
};

#endif // CH32V003_ADC_H
```

### 2.2 Driver Source (`CH32V003_ADC.cpp`)
```cpp
#include "CH32V003_ADC.h"

CH32V003_ADC::CH32V003_ADC(TwoWire& wire, uint8_t address) : _wire(wire), _address(address) {}

bool CH32V003_ADC::begin() {
    _wire.begin();
    // Test connection by reading the VDD register
    return readVDD() > 0;
}

uint16_t CH32V003_ADC::readVDD() {
    return readReg16(REG_VDD_MV);
}

uint16_t CH32V003_ADC::readRawChannel(uint8_t channel) {
    if (channel > 7) return 0;
    return readReg16(REG_RAW_CH0 + (channel * 4));
}

uint16_t CH32V003_ADC::readMilliVolts(uint8_t channel) {
    if (channel > 7) return 0;
    return readReg16(REG_MV_CH0 + (channel * 4));
}

// Sets filter weight (0-15) and active channel count (1-8 channels, default 8/0xF)
bool CH32V003_ADC::setFilterAndMask(uint8_t filter_strength, uint8_t channel_mask) {
    uint8_t config = ((filter_strength & 0x0F) << 4) | (channel_mask & 0x0F);
    return writeReg8(REG_CONFIG, config);
}

bool CH32V003_ADC::setChannelThresholds(uint8_t channel, uint16_t high_mv, uint16_t low_mv) {
    if (channel > 7) return false;
    uint8_t reg_high = REG_LIMIT_HIGH_CH0 + (channel * 4);
    uint8_t reg_low  = REG_LIMIT_LOW_CH0 + (channel * 4);
    
    bool success = writeReg16(reg_high, high_mv);
    success &= writeReg16(reg_low, low_mv);
    return success;
}

uint8_t CH32V003_ADC::readAlerts() {
    _wire.beginTransmission(_address);
    _wire.write(REG_ALERT_STATUS);
    if (_wire.endTransmission() != 0) return 0;
    
    _wire.requestFrom(_address, (uint8_t)1);
    if (_wire.available()) {
        return _wire.read();
    }
    return 0;
}

void CH32V003_ADC::clearAlert(uint8_t channel) {
    if (channel > 7) return;
    writeReg8(REG_ALERT_STATUS, (1 << channel));
}

void CH32V003_ADC::saveSettings() {
    writeReg8(REG_SAVE_SETTINGS, 0xAA);
}

void CH32V003_ADC::softReset() {
    writeReg8(REG_RESET, 0x77);
}

// Private Helper Functions
bool CH32V003_ADC::writeReg8(uint8_t reg, uint8_t value) {
    _wire.beginTransmission(_address);
    _wire.write(reg);
    _wire.write(value);
    return _wire.endTransmission() == 0;
}

bool CH32V003_ADC::writeReg16(uint8_t reg, uint16_t value) {
    _wire.beginTransmission(_address);
    _wire.write(reg);
    _wire.write((uint8_t)(value >> 8)); // MSB
    _wire.write((uint8_t)(value & 0xFF)); // LSB
    return _wire.endTransmission() == 0;
}

uint16_t CH32V003_ADC::readReg16(uint8_t reg) {
    _wire.beginTransmission(_address);
    _wire.write(reg);
    if (_wire.endTransmission() != 0) return 0;

    _wire.requestFrom(_address, (uint8_t)2);
    if (_wire.available() >= 2) {
        uint8_t msb = _wire.read();
        uint8_t lsb = _wire.read();
        return ((uint16_t)msb << 8) | lsb;
    }
    return 0;
}
```

---

## 3. Generic Embedded C Integration (e.g., STM32 / ESP-IDF)

For native C SDKs, you can map the I2C read and write operations to your platform-specific HAL APIs:

```c
#include <stdint.h>
#include <stdbool.h>

// User must implement platform-specific I2C read/write calls:
extern bool HAL_I2C_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
extern bool HAL_I2C_Read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

#define SLAVE_ADDR 0x24

// High-level API for reading calibrated mV values
uint16_t get_channel_mv(uint8_t channel) {
    if (channel > 7) return 0;
    
    uint8_t reg_addr = 0x06 + (channel * 4); // Address of MV_CHx
    uint8_t buffer[2];
    
    if (HAL_I2C_Read(SLAVE_ADDR, reg_addr, buffer, 2)) {
        return ((uint16_t)buffer[0] << 8) | buffer[1]; // Assemble MSB + LSB
    }
    return 0;
}
```

---

## 4. Python / Raspberry Pi Integration

For Linux-based single-board computers (like the Raspberry Pi), you can use Python with the standard `smbus2` library.

```python
import time
from smbus2 import SMBus

class CH32V003_ADC:
    def __init__(self, bus_id=1, address=0x24):
        self.bus = SMBus(bus_id)
        self.address = address

    def read_vdd(self):
        # Read two bytes starting from VDD register address 0x02
        data = self.bus.read_i2c_block_data(self.address, 0x02, 2)
        return (data[0] << 8) | data[1]

    def read_channel_mv(self, channel):
        if channel < 0 or channel > 7:
            raise ValueError("Channel must be 0-7")
        reg_addr = 0x06 + (channel * 4)
        data = self.bus.read_i2c_block_data(self.address, reg_addr, 2)
        return (data[0] << 8) | data[1]

# Example Usage:
if __name__ == "__main__":
    adc = CH32V003_ADC()
    print(f"System VDD: {adc.read_vdd()} mV")
    while True:
        print(f"CH0 Voltage: {adc.read_channel_mv(0)} mV")
        time.sleep(1.0)
```
