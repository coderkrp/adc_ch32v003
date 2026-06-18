#ifndef CH32V003_ADC_H
#define CH32V003_ADC_H

#include <Arduino.h>
#include <Wire.h>

/**
 * Driver class for interfacing with the CH32V003 I2C ADC Module.
 */
class CH32V003_ADC {
public:
    // Default I2C address of the module.
    static const uint8_t DEFAULT_ADDR = 0x24;

    // Register Address Map
    enum Register : uint8_t {
        REG_CONFIG          = 0x00, // Filter strength [7:4] and Channel scan mask [3:0]
        REG_DEVICE_ADDR     = 0x01, // 7-bit dynamic I2C address
        REG_VDD_MV          = 0x02, // Drift-corrected system VDD in mV (16-bit)
        REG_RAW_CH0         = 0x04, // Start of raw 10-bit ADC channel registers (8x 32-bit blocks)
        REG_MV_CH0          = 0x06, // Start of calibrated mV channel registers (8x 32-bit blocks)
        REG_LIMIT_HIGH_CH0  = 0x24, // Start of high-threshold alert limit registers
        REG_LIMIT_LOW_CH0   = 0x26, // Start of low-threshold alert limit registers
        REG_ALERT_STATUS    = 0x44, // Sticky latching alarm flags for each channel
        REG_SAVE_SETTINGS   = 0xFE, // Command register to flash current settings (0xAA)
        REG_RESET           = 0xFF  // Command register to soft-reset co-processor (0x77)
    };

    /**
     * Constructor.
     * @param wire Reference to the TwoWire I2C instance (default Wire).
     * @param address I2C address of the ADC module (default 0x24).
     */
    CH32V003_ADC(TwoWire& wire = Wire, uint8_t address = DEFAULT_ADDR);

    /**
     * Initializes the I2C interface and checks connectivity to the module.
     * @return true if communication succeeded, false otherwise.
     */
    bool begin();
    
    /**
     * Reads the drift-corrected rail voltage of the module in millivolts.
     * @return Rail voltage in mV.
     */
    uint16_t readVDD();

    /**
     * Reads the raw 10-bit value for a specific ADC channel.
     * @param channel Channel index (0 to 7).
     * @return Raw 10-bit ADC value (0-1023).
     */
    uint16_t readRawChannel(uint8_t channel);

    /**
     * Reads the calibrated voltage in millivolts for a specific ADC channel.
     * @param channel Channel index (0 to 7).
     * @return Channel voltage in mV.
     */
    uint16_t readMilliVolts(uint8_t channel);
    
    /**
     * Configures filter strength and active channel mask (analog scan count).
     * @param filter_strength EMA filter alpha value (0-15, where 0 is disabled, 15 is heaviest filtering).
     * @param channel_mask Number of scanned channels (1-8, where 8 scans all AIN0-7).
     * @return true on success, false on I2C failure.
     */
    bool setFilterAndMask(uint8_t filter_strength, uint8_t channel_mask);

    /**
     * Configures the high and low voltage limits for threshold alerts.
     * @param channel Channel index (0 to 7).
     * @param high_mv Upper limit in mV.
     * @param low_mv Lower limit in mV.
     * @return true on success, false on I2C failure or invalid channel.
     */
    bool setChannelThresholds(uint8_t channel, uint16_t high_mv, uint16_t low_mv);

    /**
     * Reads the active alert bitmask.
     * @return Bitmask where bit 'ch' represents an alert on channel 'ch'.
     */
    uint8_t readAlerts();

    /**
     * Clears the active alarm flag for a specific channel.
     * @param channel Channel index (0 to 7).
     */
    void clearAlert(uint8_t channel);
    
    /**
     * Saves current register settings to non-volatile flash memory on the module.
     */
    void saveSettings();

    /**
     * Resets the CH32V003 co-processor via soft reset.
     */
    void softReset();

private:
    TwoWire& _wire;
    uint8_t _address;
    
    bool writeReg8(uint8_t reg, uint8_t value);
    bool writeReg16(uint8_t reg, uint16_t value);
    uint16_t readReg16(uint8_t reg);
};

#endif // CH32V003_ADC_H
