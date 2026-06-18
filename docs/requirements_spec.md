# Requirements Specification & Design Record

This document captures the original design rationale, architectural constraints, hardware/software specifications, functional safety criteria, and scope boundaries for the CH32V003 I2C ADC Module.

---

## 1. Project Genesis (Why the project was built)

Dedicated external Analog-to-Digital Converter (ADC) integrated circuits (such as the ADS1115 or PCF8591) are commonly used in embedded designs to expand analog input count or provide precision readings. However, they present several disadvantages:
* **High Unit Cost:** Quality dedicated ADCs cost between $1.50 and $3.00, which can exceed the cost of the main system microcontroller.
* **Master CPU Overhead:** The Master MCU must continuously poll the ADC registers, fetch raw data, apply digital filtering, and process threshold checks, wasting CPU cycles and bus bandwidth.
* **Reference Drift:** Lower-cost ADCs use the system power supply ($V_{DD}$) as their reference, causing voltage fluctuations on the supply rail to corrupt measurement accuracy.
* **Limited Address Space:** Hardwired address pins usually restrict a system to 2 or 4 identical chips on the same I2C bus.

By programming a $0.15 RISC-V microcontroller (the CH32V003) as a **programmable active co-processor**, we solve these issues. The co-processor offloads high-speed scanning, performs local DSP filtering, dynamically corrects supply drift, and runs an autonomous alarm watchdog, asserting an interrupt only when a value violates configured limits.

---

## 2. Initial Design Constraints

* **Strict BOM Target:** The total Bill of Materials (BOM) for the module must be under $0.25 (comprising the MCU, decoupling capacitors, and standard I2C pull-up resistors).
* **Silicon Resource Limits:** The firmware must operate reliably within the strict bounds of the CH32V003F4P6 package:
  * **Program Memory:** 16 KB Flash.
  * **SRAM:** 2 KB RAM.
* **Backward Compatibility:** Future additions (such as 12-bit oversampling) must be backward compatible with older configurations stored in Flash to prevent breaking deployed hardware.
* **Pin Allocation & Conflicts:** The TSSOP20 package has a limited number of pins. The `ALERT` interrupt pin and analog input `AIN3` must share the same physical pin (`PD2`), requiring dynamic logic override states in the firmware.
* **ADC Reference Constraints:** The CH32V003 lacks a dedicated external $V_{REF}$ pin. It utilizes the system $V_{DD}$ rail as its measurement ceiling. Since $V_{DD}$ is susceptible to drift, the firmware must implement active, real-time drift compensation using the internal 1.2V bandgap reference channel.

---

## 3. Hardware & Software Specifications

### 3.1 Hardware Platform
* **MCU:** CH32V003F4P6 (32-bit RISC-V QingKe V2A core at 48MHz system clock).
* **Communication Interface:** I2C Slave (PC1/SDA, PC2/SCL) supporting Standard-mode (100 kHz) and Fast-mode (400 kHz).
* **Analog Inputs:** 10-bit physical Successive Approximation Register (SAR) ADC, oversampled to emulated 12-bit resolution.
* **Scan Configuration:** Continuous DMA (Direct Memory Access) circular channel transfer cycling through up to 8 external channels and 1 internal reference channel.

### 3.2 Software Framework
* **Development Environment:** PlatformIO with the bare-metal **`ch32v003fun`** framework.
* **Programming Paradigm:** Direct register-level programming without heavy vendor HAL (Hardware Abstraction Layer) libraries, ensuring code size fits within <6 KB of Flash and <400 bytes of RAM.

---

## 4. Functional Safety & Precautions

### 4.1 Flash Memory Wear Mitigation
The co-processor writes settings (I2C address, filter weights, thresholds, resolution) to Program Flash using the `SAVE_SETTINGS` command. The internal flash is rated for approximately 10,000 erase/program cycles.
* **Precaution:** The firmware implements a "dirty check" page-programming check. Before executing an erase-program sequence, the co-processor compares the prepared config buffer with the existing data on the flash page. If the values are identical, the write operation is aborted.

### 4.2 Shared Pin Fault Prevention
When the active channel count is 4 or more, pin `PD2` acts as both the analog input `AIN3` and the open-drain active-low `ALERT` output.
* **Precaution:** To prevent short circuits or signal corruption:
  - The pin is normally configured as an Analog Input.
  - When a threshold alarm is triggered, the pin mode is temporarily overridden to Open-Drain Output and pulled low.
  - While the pin is asserted low, the analog conversions for AIN3 are temporarily ignored/masked in the watchdog thread.
  - Once the Master MCU clears the alarm, the pin mode is restored to Analog Input.

### 4.3 Alarm Interrupt Latching
Microcontrollers interfacing with the co-processor may enter low-power sleep states.
* **Precaution:** Alarms must be sticky and latched. If a channel violates a high/low limit, the alarm flag remains set and the `ALERT` pin remains pulled low until the Master MCU explicitly writes a clear command to the matching bit in the `ALERT_STATUS` register. This guarantees transient faults are never missed.

---

## 5. Scope Boundaries (Out-of-Scope Features)

* **Arbitrary Channel Masking:** Active channels are scanned sequentially starting from `AIN0` up to `num_channels` (e.g., AIN0 to AIN3). Arbitrary scanning (e.g., scanning only AIN1 and AIN5 while skipping others) is explicitly out of scope because dynamic DMA re-mapping and register checks would add significant CPU latency and memory footprint.
* **High-Speed Oscilloscope Streaming:** The co-processor stores only the latest low-pass filtered value for each channel. Buffering high-frequency transient waveforms or streaming raw samples is out of scope.
* **Analog Output (DAC):** The CH32V003 does not contain a Digital-to-Analog Converter (DAC). Any features requiring true analog voltage generation are out of scope.
