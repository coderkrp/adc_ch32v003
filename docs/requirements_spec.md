# Product Requirements and Register Specification (PRD)
## CH32V003 I2C ADC Module

This document defines the functional requirements, specifications, and register interface of the CH32V003-based multi-channel I2C ADC module.

---

## 1. Product Overview

The **CH32V003 I2C ADC Module** is a programmable co-processor designed to offload analog-to-digital conversions, filtering, voltage calibration, and limit monitoring from a Master MCU.

```
+------------------+                   +-------------------------+
|                  |                   |  CH32V003 ADC Module    |
|                  | <==== I2C Bus ===>| (Slave: 0x24)           |
|                  |                   |                         |
|    Master MCU    | <--- ALERT Pin ---| PD2 (Active Low)        |
|                  |                   +-------------------------+
|                  |                     | AIN0..AIN7   | VREF
|                  |                     v              v
+------------------+                [Analog Ins]    [1.2V Bandgap]
```

---

## 2. Functional Requirements

### 2.1 Analog Inputs (ADC)
* **Resolution:** 10-bit Successive Approximation Register (SAR) ADC.
* **Channels:** Up to 8 external channels (AIN0 to AIN7) and 1 internal reference channel.
  > [!NOTE]
  > The internal 1.2V bandgap reference is scanned by the ADC to compute system $V_{DD}$ rail fluctuations. Its raw conversion value is used solely for internal voltage calibration and compensation and is not directly exposed as an I2C register.
* **Sampling Rate:** Continuous scanning in the background with zero Master CPU polling overhead.
* **Calibration & Compensation:**
  * **Startup Calibration:** Automatic offset self-calibration.
  * **Supply Voltage Drift Correction:** Live measurement of the internal 1.2V reference to calculate actual $V_{DD}$ rail fluctuations. Output data is corrected and available as millivolts ($mV$) directly.

### 2.2 DSP & Digital Filtering
* **EMA Low-Pass Filter:** Single-pole Exponential Moving Average filter on all raw channels.
* **Selectable Filter Coefficient:** Configurable filtering strength ($\alpha$) from 0 (disabled) to 15 (maximum filtering).

### 2.3 Limits & Interrupt Notification
* **Window Thresholds:** High and low limit registers ($mV$) for each channel.
* **ALERT Interrupt Pin:** Active-low hardware interrupt pin (`PD2`). Pulls low if any enabled channel violates high/low limits.
* **Fault Hysteresis:** Latched alarm flags that must be cleared by the master.

### 2.4 Device Control
* **Slave Address Configuration:** Ability to reprogram the 7-bit I2C slave address.
* **Persistent Settings:** Save configurations (I2C address, filter strength, thresholds) to internal Flash.
* **Soft Reboot:** Support software reboot command via register write.

---

## 3. Communication Protocol

### 3.1 Physical Interface
* **Interface:** I2C Slave.
* **Standard Speeds:** 100 kHz (Standard Mode) and 400 kHz (Fast Mode).
* **Pins:** PC1 (SDA) and PC2 (SCL).
* **Pull-Up Resistors:** Externally pulled up to VDD (typically 4.7kΩ).

### 3.2 Address Pointer Rules
* All read and write operations are register-addressed.
* The master writes a 1-byte register address offset, which remains cached in the slave.
* **Auto-Increment:** During multi-byte reads or writes, the register address pointer automatically increments by 1 for each transmitted byte.

### 3.3 Transaction Formats

#### 3.3.1 Register Write
```
Start | Slave Address + W | ACK | Register Offset | ACK | Data Byte 0 | ACK | [Data Byte 1 | ACK] ... | Stop
```

#### 3.3.2 Register Read (Combined Format)
```
Start | Slave Address + W | ACK | Register Offset | ACK |
Restart | Slave Address + R | ACK | Data Byte 0 | ACK | Data Byte 1 | ACK | ... | Data Byte N | NACK | Stop
```

---

## 4. Register Map Reference

All 16-bit registers are stored and transmitted in **Big-Endian** format (MSB first, LSB second).

| Address | Type | Name | Description | Reset Value |
| :--- | :--- | :--- | :--- | :--- |
| **0x00** | R/W | `CONFIG` | Configures filter strength [7:4] and channel scan count [3:0] | `0x4F` |
| **0x01** | R/W | `DEVICE_ADDR` | 7-bit active I2C Slave Address (default `0x24`) | `0x24` |
| **0x02 - 0x03** | R | `VDD_MV` | Compensated $V_{DD}$ rail voltage in $mV$ | - |
| **0x04 - 0x05** | R | `RAW_CH0` | Raw ADC conversion for Channel 0 (0 to 1023) | - |
| **0x06 - 0x07** | R | `MV_CH0` | Calibrated voltage reading for Channel 0 in $mV$ | - |
| **0x08 - 0x09** | R | `RAW_CH1` | Raw ADC conversion for Channel 1 (0 to 1023) | - |
| **0x0A - 0x0B** | R | `MV_CH1` | Calibrated voltage reading for Channel 1 in $mV$ | - |
| **0x0C - 0x0D** | R | `RAW_CH2` | Raw ADC conversion for Channel 2 (0 to 1023) | - |
| **0x0E - 0x0F** | R | `MV_CH2` | Calibrated voltage reading for Channel 2 in $mV$ | - |
| **0x10 - 0x11** | R | `RAW_CH3` | Raw ADC conversion for Channel 3 (0 to 1023) | - |
| **0x12 - 0x13** | R | `MV_CH3` | Calibrated voltage reading for Channel 3 in $mV$ | - |
| **0x14 - 0x15** | R | `RAW_CH4` | Raw ADC conversion for Channel 4 (0 to 1023) | - |
| **0x16 - 0x17** | R | `MV_CH4` | Calibrated voltage reading for Channel 4 in $mV$ | - |
| **0x18 - 0x19** | R | `RAW_CH5` | Raw ADC conversion for Channel 5 (0 to 1023) | - |
| **0x1A - 0x1B** | R | `MV_CH5` | Calibrated voltage reading for Channel 5 in $mV$ | - |
| **0x1C - 0x1D** | R | `RAW_CH6` | Raw ADC conversion for Channel 6 (0 to 1023) | - |
| **0x1E - 0x1F** | R | `MV_CH6` | Calibrated voltage reading for Channel 6 in $mV$ | - |
| **0x20 - 0x21** | R | `RAW_CH7` | Raw ADC conversion for Channel 7 (0 to 1023) | - |
| **0x22 - 0x23** | R | `MV_CH7` | Calibrated voltage reading for Channel 7 in $mV$ | - |
| **0x24 - 0x25** | R/W | `LIMIT_HIGH_CH0` | Upper limit threshold for Channel 0 ($mV$) | `0xFFFF` |
| **0x26 - 0x27** | R/W | `LIMIT_LOW_CH0` | Lower limit threshold for Channel 0 ($mV$) | `0x0000` |
| **0x28 - 0x29** | R/W | `LIMIT_HIGH_CH1` | Upper limit threshold for Channel 1 ($mV$) | `0xFFFF` |
| **0x2A - 0x2B** | R/W | `LIMIT_LOW_CH1` | Lower limit threshold for Channel 1 ($mV$) | `0x0000` |
| **0x2C - 0x2D** | R/W | `LIMIT_HIGH_CH2` | Upper limit threshold for Channel 2 ($mV$) | `0xFFFF` |
| **0x2E - 0x2F** | R/W | `LIMIT_LOW_CH2` | Lower limit threshold for Channel 2 ($mV$) | `0x0000` |
| **0x30 - 0x31** | R/W | `LIMIT_HIGH_CH3` | Upper limit threshold for Channel 3 ($mV$) | `0xFFFF` |
| **0x32 - 0x33** | R/W | `LIMIT_LOW_CH3` | Lower limit threshold for Channel 3 ($mV$) | `0x0000` |
| **0x34 - 0x35** | R/W | `LIMIT_HIGH_CH4` | Upper limit threshold for Channel 4 ($mV$) | `0xFFFF` |
| **0x36 - 0x37** | R/W | `LIMIT_LOW_CH4` | Lower limit threshold for Channel 4 ($mV$) | `0x0000` |
| **0x38 - 0x39** | R/W | `LIMIT_HIGH_CH5` | Upper limit threshold for Channel 5 ($mV$) | `0xFFFF` |
| **0x3A - 0x3B** | R/W | `LIMIT_LOW_CH5` | Lower limit threshold for Channel 5 ($mV$) | `0x0000` |
| **0x3C - 0x3D** | R/W | `LIMIT_HIGH_CH6` | Upper limit threshold for Channel 6 ($mV$) | `0xFFFF` |
| **0x3E - 0x3F** | R/W | `LIMIT_LOW_CH6` | Lower limit threshold for Channel 6 ($mV$) | `0x0000` |
| **0x40 - 0x41** | R/W | `LIMIT_HIGH_CH7` | Upper limit threshold for Channel 7 ($mV$) | `0xFFFF` |
| **0x42 - 0x43** | R/W | `LIMIT_LOW_CH7` | Lower limit threshold for Channel 7 ($mV$) | `0x0000` |
| **0x44** | R/W | `ALERT_STATUS` | Bitmask of channels violating limits. Write 1 to clear alarm | `0x00` |
| **0xFE** | W | `SAVE_SETTINGS` | Write `0xAA` to commit CONFIG & DEVICE_ADDR to Flash | `0x00` |
| **0xFF** | W | `RESET` | Write `0x77` to trigger a software reboot | `0x00` |

---

## 5. Register Bitfields

### 5.1 `CONFIG` Register (0x00)
```
 Bit   [7]     [6]     [5]     [4]     [3]     [2]     [1]     [0]
     +---------------+---------------+---------------+---------------+
Name |            FILTER             |            CH_MASK            |
     +---------------+---------------+---------------+---------------+
```
* **FILTER[7:4] - Filter Weight Selection ($\alpha$):**
  * `0x0`: Filter disabled (fastest step response, raw readings outputted directly).
  * `0x1` to `0xF`: EMA smoothing coefficient. A value of `0x4` is standard (moderate filtering, low noise).
* **CH_MASK[3:0] - Pin Scanning Channel Count:**
  * Defines the number of active channels scanned sequentially starting from AIN0. Despite the name "MASK", this field operates as a count/limit.
  * `0x1`: Scan AIN0 only (1 channel active).
  * `0x2`: Scan AIN0 - AIN1 (2 channels active).
  * `0x3`: Scan AIN0 - AIN2 (3 channels active).
  * `0x4`: Scan AIN0 - AIN3 (4 channels active).
  * `0x5`: Scan AIN0 - AIN4 (5 channels active).
  * `0x6`: Scan AIN0 - AIN5 (6 channels active).
  * `0x7`: Scan AIN0 - AIN6 (7 channels active).
  * `0x8` to `0xF` (Default `0xF`): Scan all channels AIN0 - AIN7 (8 channels active).
  * 
    > [!NOTE]
    > **Design Decision (Sequential Count vs. Arbitrary Mask):**
    > Although named `CH_MASK`, this field acts as a sequential count rather than an arbitrary bitmask. An arbitrary bitmask would require 8 bits (one per channel), which exceeds the 4 bits available in the `CONFIG` register. Splitting the register or implementing complex dynamic DMA buffer mapping in the firmware was skipped to keep the core execution loop simple and avoid I2C timing overhead.

### 5.2 `ALERT_STATUS` Register (0x44)
```
 Bit   [7]     [6]     [5]     [4]     [3]     [2]     [1]     [0]
     +-------+-------+-------+-------+-------+-------+-------+-------+
Name |  CH7  |  CH6  |  CH5  |  CH4  |  CH3  |  CH2  |  CH1  |  CH0  |
     +-------+-------+-------+-------+-------+-------+-------+-------+
```
* Each bit represents a threshold alarm state for that channel.
* **0:** Voltage is within limits.
* **1:** Voltage is outside configured `LIMIT_HIGH` or `LIMIT_LOW` thresholds.
* **Clearing:** Alarms are sticky and latching. The Master MCU must write a `1` to the matching bit address to clear the interrupt.

---

## 6. System Integration Guidelines & Safety Warnings

### 6.1 Flash Write Persistence Limits
> [!WARNING]
> The `SAVE_SETTINGS` command triggers an erase and write sequence on the microcontroller's program flash memory. The program flash has a write endurance limit of approximately 10,000 cycles.
> To prevent premature flash wear-out:
> - Only write to `SAVE_SETTINGS` (`0xFE`) when settings (such as `DEVICE_ADDR` or `CONFIG`) are explicitly changed by a user.
> - Do not automate setting saves in recurring runtime code loops or at every boot cycle.
> - The firmware should ideally perform a dirty-flag check, comparing new settings to the values currently stored in flash, and skip the write if they are identical.

### 6.2 I2C Address Conflicts
> [!IMPORTANT]
> The default I2C address of the module is `0x24`. This address lies within a range frequently utilized by other microchips, such as some external I2C ADCs (e.g., PCF8591 or ADS1115 variants) and sensors.
> - Always perform an I2C bus scan when integrating this module with other devices.
> - If an address conflict is detected, change the module's I2C address by writing the desired new address to `DEVICE_ADDR` (`0x01`) and saving it via `SAVE_SETTINGS` (`0xFE`).
