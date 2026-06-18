import time
from smbus2 import SMBus

class CH32V003_ADC:
    """
    Python driver for the CH32V003 I2C ADC Module.
    Uses standard Linux SMBus interface (via smbus2).
    """
    DEFAULT_ADDR = 0x24

    # Register Address Map
    REG_CONFIG          = 0x00
    REG_DEVICE_ADDR     = 0x01
    REG_VDD_MV          = 0x02
    REG_RAW_CH0         = 0x04
    REG_MV_CH0          = 0x06
    REG_LIMIT_HIGH_CH0  = 0x24
    REG_LIMIT_LOW_CH0   = 0x26
    REG_ALERT_STATUS    = 0x44
    REG_SAVE_SETTINGS   = 0xFE
    REG_RESET           = 0xFF

    def __init__(self, bus_id=1, address=DEFAULT_ADDR):
        """
        Initializes the SMBus connection to the co-processor.
        :param bus_id: Standard I2C bus ID (e.g., 1 on Raspberry Pi).
        :param address: 7-bit I2C slave address of the module.
        """
        self.bus = SMBus(bus_id)
        self.address = address

    def read_vdd(self):
        """Reads the drift-corrected system rail voltage (VDD) in millivolts."""
        return self._read_reg16(self.REG_VDD_MV)

    def read_raw_channel(self, channel):
        """
        Reads the raw 10-bit value for a specific ADC channel.
        :param channel: Channel index (0 to 7).
        :return: Raw 10-bit integer (0-1023).
        """
        if not (0 <= channel <= 7):
            raise ValueError("Channel must be between 0 and 7")
        return self._read_reg16(self.REG_RAW_CH0 + (channel * 4))

    def read_channel_mv(self, channel):
        """
        Reads the calibrated voltage in millivolts for a specific channel.
        :param channel: Channel index (0 to 7).
        :return: Channel voltage in mV.
        """
        if not (0 <= channel <= 7):
            raise ValueError("Channel must be between 0 and 7")
        return self._read_reg16(self.REG_MV_CH0 + (channel * 4))

    def set_filter_and_mask(self, filter_strength, channel_mask):
        """
        Configures the EMA filter weight and active analog scan channel count.
        :param filter_strength: Filter weight (0 to 15, 0 = bypassed, 15 = maximum filter).
        :param channel_mask: Active scan count (1 to 8, 8 = scan all AIN0-7).
        :return: True on success, False on error.
        """
        config = ((filter_strength & 0x0F) << 4) | (channel_mask & 0x0F)
        return self._write_reg8(self.REG_CONFIG, config)

    def set_channel_thresholds(self, channel, high_mv, low_mv):
        """
        Configures high and low voltage limit thresholds for alerting.
        :param channel: Channel index (0 to 7).
        :param high_mv: High threshold in mV.
        :param low_mv: Low threshold in mV.
        :return: True on success, False on error.
        """
        if not (0 <= channel <= 7):
            raise ValueError("Channel must be between 0 and 7")
        reg_high = self.REG_LIMIT_HIGH_CH0 + (channel * 4)
        reg_low = self.REG_LIMIT_LOW_CH0 + (channel * 4)
        
        success = self._write_reg16(reg_high, high_mv)
        success = success and self._write_reg16(reg_low, low_mv)
        return success

    def read_alerts(self):
        """
        Reads the active latching alarm flags for all channels.
        :return: Alert bitmask (bit 'ch' set indicates limit violation on that channel).
        """
        return self._read_reg8(self.REG_ALERT_STATUS)

    def clear_alert(self, channel):
        """
        Clears the sticky/latching alarm status for a specific channel.
        :param channel: Channel index (0 to 7).
        :return: True on success, False on error.
        """
        if not (0 <= channel <= 7):
            raise ValueError("Channel must be between 0 and 7")
        return self._write_reg8(self.REG_ALERT_STATUS, 1 << channel)

    def save_settings(self):
        """
        Saves current configuration limits and register states to persistent Flash memory.
        :return: True on success, False on error.
        """
        return self._write_reg8(self.REG_SAVE_SETTINGS, 0xAA)

    def soft_reset(self):
        """
        Commands the co-processor to perform a software reset.
        :return: True on success, False on error.
        """
        return self._write_reg8(self.REG_RESET, 0x77)

    def close(self):
        """Closes the SMBus I2C connection interface."""
        self.bus.close()

    # Private Low-Level Helper Methods

    def _read_reg8(self, reg):
        try:
            return self.bus.read_byte_data(self.address, reg)
        except IOError as e:
            print(f"Error reading byte from register {hex(reg)}: {e}")
            return None

    def _write_reg8(self, reg, val):
        try:
            self.bus.write_byte_data(self.address, reg, val)
            return True
        except IOError as e:
            print(f"Error writing byte {hex(val)} to register {hex(reg)}: {e}")
            return False

    def _read_reg16(self, reg):
        try:
            # Reads 2 bytes starting at register offset (Big-Endian assembly)
            data = self.bus.read_i2c_block_data(self.address, reg, 2)
            return (data[0] << 8) | data[1]
        except IOError as e:
            print(f"Error reading word from register {hex(reg)}: {e}")
            return None

    def _write_reg16(self, reg, val):
        try:
            # Writes 2 bytes (Big-Endian format)
            data = [(val >> 8) & 0xFF, val & 0xFF]
            self.bus.write_i2c_block_data(self.address, reg, data)
            return True
        except IOError as e:
            print(f"Error writing word {hex(val)} to register {hex(reg)}: {e}")
            return False
