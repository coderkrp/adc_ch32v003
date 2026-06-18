#include "CH32V003_ADC.h"

CH32V003_ADC::CH32V003_ADC(TwoWire& wire, uint8_t address) : _wire(wire), _address(address) {}

bool CH32V003_ADC::begin() {
    _wire.begin();
    // Test communication by reading system VDD register (which should be non-zero)
    return readVDD() > 0;
}

uint16_t CH32V003_ADC::readVDD() {
    return readReg16(REG_VDD_MV);
}

uint16_t CH32V003_ADC::readRawChannel(uint8_t channel) {
    if (channel > 7) return 0;
    // Each channel has a 4-byte block (2 bytes raw, 2 bytes mv)
    return readReg16(REG_RAW_CH0 + (channel * 4));
}

uint16_t CH32V003_ADC::readMilliVolts(uint8_t channel) {
    if (channel > 7) return 0;
    // Each channel has a 4-byte block (2 bytes raw, 2 bytes mv)
    return readReg16(REG_MV_CH0 + (channel * 4));
}

bool CH32V003_ADC::setFilterAndMask(uint8_t filter_strength, uint8_t channel_mask) {
    // Pack filter strength (4 bits) and channel mask (4 bits)
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
    // Writing a '1' to the matching bit in the alert register clears that sticky alert
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
    _wire.write((uint8_t)(value >> 8));   // MSB
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
