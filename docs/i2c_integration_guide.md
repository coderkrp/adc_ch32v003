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

## 2. Arduino C++ Driver Integration

To communicate with the CH32V003 ADC Module from Arduino (or compatible MCUs like ESP32 and STM32), we provide a reusable client driver library:
* Driver Header: [CH32V003_ADC.h](../drivers/arduino/CH32V003_ADC.h)
* Driver Source: [CH32V003_ADC.cpp](../drivers/arduino/CH32V003_ADC.cpp)

### Basic Usage Snippet
Include the header file and instantiate the `CH32V003_ADC` class. This abstracts the low-level register maps and data assembly:

```cpp
#include "CH32V003_ADC.h"

CH32V003_ADC adc; // Uses default I2C Address 0x24

void setup() {
    Serial.begin(115200);
    if (adc.begin()) {
        Serial.println("ADC Module initialized successfully.");
    }
}

void loop() {
    // Read calibrated millivolts directly
    uint16_t vdd_mv = adc.readVDD();
    uint16_t ch0_mv = adc.readMilliVolts(0);
    
    Serial.print("VDD: "); Serial.print(vdd_mv); Serial.print(" mV | ");
    Serial.print("CH0: "); Serial.print(ch0_mv); Serial.println(" mV");
    delay(1000);
}
```

### Reference Examples
See complete, compile-ready sketch directories here:
* [arduino_basic.ino](../examples/arduino_basic/arduino_basic.ino): Demonstrates simple polling of system voltage and channel readings.
* [arduino_latching_alerts.ino](../examples/arduino_latching_alerts/arduino_latching_alerts.ino): Demonstrates setting threshold alerts, handling the ALERT pin interrupt, and clearing latching alarm flags.

---

## 3. Generic Embedded C Integration (e.g., STM32 / ESP-IDF)

For native C SDKs where you do not wish to use Arduino C++, you can map the I2C read and write operations directly to your platform-specific HAL APIs:

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
    
    uint8_t reg_addr = 0x06 + (channel * 4); // Address of REG_MV_CH0 + (channel * 4)
    uint8_t buffer[2];
    
    if (HAL_I2C_Read(SLAVE_ADDR, reg_addr, buffer, 2)) {
        return ((uint16_t)buffer[0] << 8) | buffer[1]; // Assemble MSB + LSB
    }
    return 0;
}
```

---

## 4. Python / Raspberry Pi Integration

For Linux-based single-board computers (like the Raspberry Pi), you can use Python with the standard `smbus2` library. We provide a reusable driver module:
* Python Module: [ch32v003_adc.py](../drivers/python/ch32v003_adc.py)

### Basic Usage Snippet
```python
import time
from ch32v003_adc import CH32V003_ADC

# Initialize connection on default bus 1 (standard on Pi)
adc = CH32V003_ADC(bus_id=1)

print(f"System VDD: {adc.read_vdd()} mV")

try:
    while True:
        print(f"CH0 Voltage: {adc.read_channel_mv(0)} mV")
        time.sleep(1.0)
except KeyboardInterrupt:
    pass
finally:
    adc.close()
```

### Reference Example
* [read_adc.py](../examples/python_raspberry_pi/read_adc.py): A ready-to-run script showing reading calibrated values and scaling them back based on an external resistor divider.


