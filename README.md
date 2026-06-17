# CH32V003 I2C ADC Module

A programmable, high-performance co-processor firmware for the **CH32V003F4P6 (TSSOP20)** RISC-V microcontroller. It offloads continuous multi-channel analog-to-digital conversion, digital low-pass filtering, supply voltage drift correction, and threshold window limit checks from a Master MCU (such as an ESP32, STM32, Arduino, or Raspberry Pi) over the I2C bus.

```
+------------------+                   +-------------------------+
|                  |                   |  CH32V003 ADC Module    |
|                  | <==== I2C Bus ===>| (Slave Address: 0x24)   |
|                  |                   |                         |
|    Master MCU    | <--- ALERT Pin ---| PD2 (Active Low)        |
|                  |                   +-------------------------+
|                  |                     | AIN0..AIN7   | VREF
|                  |                     v              v
+------------------+                [Analog Ins]    [1.2V Bandgap]
```

---

## Features

*   **Multi-Channel background scanning:** Automatically cycles through up to 8 external channels (`AIN0`–`AIN7`) and 1 internal reference channel (`AIN8`, 1.2V Bandgap) using continuous DMA transfers. Zero Master CPU overhead.
*   **EMA Digital Filtering:** Real-time Exponential Moving Average (EMA) low-pass smoothing filter on all active channels with configurable weights ($\alpha$) from 0 (disabled) to 15 (maximum filtering).
*   **Voltage Drift Correction:** Live measurement of the internal 1.2V bandgap reference is used to calculate the real $V_{DD}$ supply voltage rail. Output data is automatically corrected and available in millivolts ($mV$) directly.
*   **Window Limit Monitoring & Latching Alerts:** Configurable high and low limit registers ($mV$) for each channel. An active-low hardware interrupt pin (`PD2`) pulls low if limits are violated. Alarms are sticky and latching until cleared by the Master.
*   **Dynamic Reconfiguration:** Seamless updates to active channel counts (`CH_MASK`) and filter strengths over I2C, which dynamically reconfigure the hardware sequencer and DMA buffers on the fly.
*   **Non-Volatile Storage:** Commits configured settings (I2C address, filter weights, thresholds) to the internal Program Flash.
*   **Flash Wear-Out Protection:** Implements a dirty-flag check before writing settings to Flash, preventing premature wearing of the flash memory (rated for ~10,000 cycles).
*   **Super-Lightweight Footprint:** Highly optimized code written directly on registers using the bare-metal **ch32v003fun** framework. Takes only **~4.9 KB of Flash** (30.0%) and **356 bytes of SRAM** (17.4%).

---

## Hardware Reference & Pinout

Designed to operate on either a 3.3V or 5.0V power rail, matching the logic levels of your Master MCU.

| Pin | Name | Type | Connection | Description |
| :--- | :--- | :--- | :--- | :--- |
| **1** | PD4 | AI | **AIN7** | Analog input channel 7 |
| **2** | PD5 | AI | **AIN5** | Analog input channel 5 |
| **3** | PD6 | AI | **AIN6** | Analog input channel 6 |
| **7** | PA1 | AI | **AIN1** | Analog input channel 1 |
| **8** | PA2 | AI | **AIN0** | Analog input channel 0 |
| **9** | VDD | Power | **VDD** | Input voltage (2.7V - 5.5V). Decouple with 100nF + 10uF capacitors. |
| **10** | GND | Power | **GND** | System Ground |
| **11** | PC1 | I/O | **SDA** | I2C Data Line (Requires external 4.7kΩ pull-up to VDD) |
| **12** | PC2 | I/O | **SCL** | I2C Clock Line (Requires external 4.7kΩ pull-up to VDD) |
| **13** | PC4 | AI | **AIN2** | Analog input channel 2 |
| **15** | PD1 | I/O | **SWIO** | Programming Clock/Data line (WCH-LinkE) |
| **16** | PD7 | I/O | **NRST** | Optional Reset line (WCH-LinkE NRST) |
| **18** | PD2 | I/O | **ALERT / AIN3** | Active-Low Interrupt Pin (Wired-AND config) OR Analog Channel AIN3 |
| **19** | PD3 | AI | **AIN4** | Analog input channel 4 |

> [!IMPORTANT]
> **PD2 Shared Functionality:** `PD2` acts as both the active-low hardware `ALERT` interrupt pin and the analog input channel `AIN3`. 
> * If the scan channel count is configured to 3 or fewer (does not scan AIN3), the pin is dedicated to `ALERT` (Output Open-Drain when alarm active, Floating Input when idle).
> * If the scan channel count is 4 or more, the pin functions as `AIN3` in the ADC sequence. When an alarm is triggered, the firmware temporarily overrides it to Output Open-Drain (pulling it low to assert the interrupt), and reverts it back to an analog input once the alarm status is cleared.

---

## I2C Communication & Register Map

All register parameters are **addressed in bytes**. Multi-byte registers (such as raw readings or calibrated millivolts) are 16-bit values stored and transmitted in **Big-Endian** format (MSB first, LSB second). During multi-byte reads/writes, the register pointer automatically increments.

The default I2C address is **`0x24`** (7-bit address).

| Address | Type | Name | Description | Reset Value |
| :--- | :--- | :--- | :--- | :--- |
| **0x00** | R/W | `CONFIG` | Filter coefficient `[7:4]` and active channel scan count `[3:0]` | `0x4F` |
| **0x01** | R/W | `DEVICE_ADDR` | 7-bit active I2C Slave Address (default `0x24`) | `0x24` |
| **0x02 - 0x03** | R | `VDD_MV` | Drift-corrected supply rail voltage in $mV$ | - |
| **0x04 - 0x05** | R | `RAW_CH0` | Filtered 10-bit raw ADC reading for Channel 0 (0 to 1023) | - |
| **0x06 - 0x07** | R | `MV_CH0` | Compensated voltage reading for Channel 0 in $mV$ | - |
| *0x08 - 0x23* | *R* | *CH1 - CH7* | *Alternate Raw and mV reading registers for channels 1 to 7* | - |
| **0x24 - 0x25** | R/W | `LIMIT_HIGH_CH0` | Upper limit threshold for Channel 0 ($mV$) | `0xFFFF` |
| **0x26 - 0x27** | R/W | `LIMIT_LOW_CH0` | Lower limit threshold for Channel 0 ($mV$) | `0x0000` |
| *0x28 - 0x43* | *R/W* | *CH1 - CH7* | *Alternate High and Low threshold limit registers for channels 1 to 7* | - |
| **0x44** | R/W | `ALERT_STATUS` | Sticky bitmask of channel limit violations. Write `1` to a bit to clear it. | `0x00` |
| **0xFE** | W | `SAVE_SETTINGS` | Write `0xAA` to commit `CONFIG`, `DEVICE_ADDR`, and limits to Flash | `0x00` |
| **0xFF** | W | `RESET` | Write `0x77` to trigger a software system reboot | `0x00` |

### CONFIG Register (0x00)
```
 Bit   [7]     [6]     [5]     [4]     [3]     [2]     [1]     [0]
     +---------------+---------------+---------------+---------------+
Name |            FILTER             |            CH_MASK            |
     +---------------+---------------+---------------+---------------+
```
*   **FILTER[7:4] - Filter Weight Selection ($\alpha$):**
    *   `0x0`: Filter disabled (fastest step response, raw readings outputted directly).
    *   `0x1` to `0xF`: EMA smoothing coefficient (where `0x4` is default, giving moderate filtering and low noise).
*   **CH_MASK[3:0] - Pin Scanning Channel Count:**
    *   Defines the number of active channels scanned sequentially starting from `AIN0`.
    *   `0x1`: Scan `AIN0` only (1 channel active).
    *   `0x2`: Scan `AIN0` - `AIN1` (2 channels active).
    *   ...
    *   `0x8` to `0xF` (Default `0xF`): Scan all channels `AIN0` - `AIN7` (8 channels active).

### ALERT_STATUS Register (0x44)
```
 Bit   [7]     [6]     [5]     [4]     [3]     [2]     [1]     [0]
     +-------+-------+-------+-------+-------+-------+-------+-------+
Name |  CH7  |  CH6  |  CH5  |  CH4  |  CH3  |  CH2  |  CH1  |  CH0  |
     +-------+-------+-------+-------+-------+-------+-------+-------+
```
*   **0:** Voltage is within configured limits.
*   **1:** Voltage has violated either `LIMIT_HIGH` or `LIMIT_LOW` thresholds.
*   *Latching Clears:* Alarms are sticky and latching. The Master MCU must write a `1` to the matching channel bit to clear the interrupt (e.g., write `0x02` to clear `CH1`).

---

## Safety & Safety Warnings

### Flash Memory Wear
The `SAVE_SETTINGS` command triggers page erasure and programming in the microcontroller's Program Flash. The internal flash has a limit of **~10,000 erase/write cycles**. 
*   **Do not** automate setting saves in recurring runtime code loops.
*   Only call `SAVE_SETTINGS` when settings are explicitly changed by a user.
*   The firmware incorporates a dirty-flag check: writing `0xAA` will read the current flash content first and abort the write sequence if the values are unchanged.

---

## Setup & Flashing Instructions

PlatformIO is the recommended environment for compiling, flashing, and debugging.

### 1. PlatformIO Setup
1. Install **VSCode** and the **PlatformIO IDE** extension.
2. Open this project directory in VSCode.

### 2. Linux USB Permissions (udev rules)
To flash and debug as a non-root user under Linux, set up the WCH-LinkE USB permissions:
1. Create a rules file:
   ```bash
   sudo nano /etc/udev/rules.d/99-wch-link.rules
   ```
2. Paste the following configuration lines:
   ```udev
   # WCH-LinkE in RV (RISC-V) Mode
   SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8010", MODE="0666", GROUP="plugdev"
   SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8012", MODE="0666", GROUP="plugdev"
   ```
3. Save, reload the daemon, and re-plug your programmer:
   ```bash
   sudo udevadm control --reload-rules && sudo udevadm trigger
   ```

### 3. Build & Flash
Connect your WCH-LinkE programmer to the MCU:
*   `VCC` -> `VDD`
*   `GND` -> `GND`
*   `SWIO` -> `PD1`

Use the following commands (or equivalent graphical buttons in the VSCode status bar):

*   **Build Project:**
    ```bash
    pio run
    ```
*   **Flash Firmware:**
    ```bash
    pio run --target upload
    ```
*   **View Serial Logs (SWIO printfs):**
    ```bash
    pio device monitor
    ```
    *(Press `Ctrl+C` to exit the monitor).*

---

## Directory Structure

```text
adc_ch32v003/
├── docs/                     # Design and Integration documentation
│   ├── requirements_spec.md  # Product specifications
│   ├── architecture_design.md# Firmware architecture and loops
│   ├── hardware_design.md    # Schematic recommendations and BOM
│   ├── i2c_integration_guide.md # Multi-platform Master driver examples
│   └── window_limit_alerts.md# Window limits & latching alerts guide
├── include/
│   ├── funconfig.h           # ch32fun framework clock/debug configuration
│   └── i2c_slave.h           # Modified single-file I2C slave library
├── src/
│   └── main.c                # Main application entry point & interrupts
├── platformio.ini            # PlatformIO project configuration
├── LICENSE                   # MIT License file
└── README.md                 # This overview document
```

---

## Documentation & Integration Guide

*   **[Window Limits & Latching Alerts Guide](docs/window_limit_alerts.md)**: Detailed explanation of window limit monitoring, latching alarms, PD2 hardware interrupts, and real-world system safety use cases.
*   **[I2C Master Driver Guide](docs/i2c_integration_guide.md)**: Fully detailed, boilerplate-ready master drivers and examples for:
    *   **Arduino (C++ `<Wire.h>`)**
    *   **Generic Embedded C (HAL/ESP-IDF)**
    *   **Raspberry Pi / Linux Single-Board Computers (Python `smbus2`)**

---

## License

This project is open-source software licensed under the [MIT License](LICENSE).
