# Hardware Reference Design Guide
## CH32V003 I2C ADC Module

This document defines the hardware reference design, schematic concepts, component selection, programming headers, and wiring requirements for the CH32V003 TSSOP20 ADC module.

---

## 1. Schematic Design Concept

The module is designed to operate on either a 3.3V or 5.0V power rail, matching the power domain of the Master MCU.

```
                  +--------------------------------+
                  |         CH32V003 TSSOP20       |
                  |                                |
  [VDD] ----------| 9 (VDD)               (SWIO) 15|---------- [WCH-Link SWIO]
  [GND] ----------| 10 (GND)             (NRST) 16 |---------- [WCH-Link NRST]
                  |                                |
  [SDA] <===+=====| 11 (PC1)              (PD2) 18 |---------- [ALERT / AIN3]
            |     |                       (PA2) 8  |---------- [Analog In 0]
  [SCL] <=+=|=====| 12 (PC2)              (PA1) 7  |---------- [Analog In 1]
          | |     |                       (PC4) 13 |---------- [Analog In 2]
         [R][R]   |                       (PD3) 19 |---------- [Analog In 4]
          | |     |                       (PD5) 2  |---------- [Analog In 5]
  [VDD] --+-+     |                       (PD6) 3  |---------- [Analog In 6]
                  |                       (PD4) 1  |---------- [Analog In 7]
                  +--------------------------------+
```

---

## 2. Pin Assignments (TSSOP20)

| Pin | Name | Type | Connection | Description |
| :--- | :--- | :--- | :--- | :--- |
| **1** | PD4 | AI | **AIN7** | Analog input channel 7 |
| **2** | PD5 | AI | **AIN5** | Analog input channel 5 |
| **3** | PD6 | AI | **AIN6** | Analog input channel 6 |
| **7** | PA1 | AI | **AIN1** | Analog input channel 1 |
| **8** | PA2 | AI | **AIN0** | Analog input channel 0 |
| **9** | VDD | Power | **VDD** | Input voltage (2.7V - 5.5V) |
| **10** | GND | Power | **GND** | System Ground |
| **11** | PC1 | I/O | **SDA** | I2C Data Line (External pull-up required) |
| **12** | PC2 | I/O | **SCL** | I2C Clock Line (External pull-up required) |
| **13** | PC4 | AI | **AIN2** | Analog input channel 2 |
| **15** | PD1 | I/O | **SWIO** | Programming Clock/Data line (WCH-LinkE) |
| **16** | PD7 | I/O | **NRST** | Optional Reset line (WCH-LinkE NRST) |
| **18** | PD2 | I/O | **ALERT / AIN3** | Configurable: Alarm Interrupt Pin or AIN3 |
| **19** | PD3 | AI | **AIN4** | Analog input channel 4 |

---

## 3. Passive Components and Filters

### 3.1 Power Supply Decoupling
To achieve accurate ADC readings, the MCU power domain must be filtered from high-frequency switching noise on the supply lines.
* **Decoupling Capacitors:** Place one **100nF ceramic capacitor** and one **10uF tantalum or ceramic capacitor** directly adjacent to Pin 9 (VDD) and Pin 10 (GND). Keep trace lengths to a minimum.

### 3.2 I2C Pull-Up Resistors
The I2C bus lines (SDA and SCL) are open-drain and require pull-up resistors to $V_{DD}$.
* **Value:** **$4.7\text{ k}\Omega$** is standard for 100 kHz/400 kHz buses. 
* **Placement:** These can be located on the slave board or on the master board (do not double-install them).

### 3.3 Analog Input Low-Pass Filters (Anti-Aliasing)
To prevent high-frequency noise from corrupting ADC results, each analog input should have a simple RC low-pass filter:
```
Analog Sensor Out -----[ 100 Ohm ]-----+----- MCU AINx Pin
                                       |
                                    [ 10nF ]
                                       |
                                      GND
```
* **Cutoff Frequency:** $f_c = \frac{1}{2\pi R C} \approx 159\text{ kHz}$. This is fast enough to sample dynamic sensors while cutting out high-frequency EMF.

---

## 4. Programming Header Interface

The module exposes a 4-pin programming header compatible with the standard **WCH-LinkE** programmer:

```
    [ WCH-LinkE ]                         [ Module Header ]
    Pin 1: VCC    <=====================> Pin 1: VDD (3.3V or 5V)
    Pin 2: GND    <=====================> Pin 2: GND
    Pin 3: SWIO   <=====================> Pin 3: SWIO (PD1)
    Pin 4: NRST   <=====================> Pin 4: NRST (PD7) (Optional)
```

> [!IMPORTANT]
> The programming SWIO line on PD1 is dedicated. Ensure no analog circuits or low-impedance loads are wired to PD1, as it will disrupt programming and real-time debugging.

---

## 5. Bill of Materials (BOM) Recommendation

| Designator | Qty | Value / Part | Package | Description |
| :--- | :--- | :--- | :--- | :--- |
| **U1** | 1 | CH32V003F4P6 | TSSOP20 | 32-bit RISC-V Microcontroller |
| **C1** | 1 | 100nF 16V | 0603 | Ceramic Decoupling Capacitor |
| **C2** | 1 | 10uF 10V | 0805 | Ceramic Decoupling Capacitor |
| **R1, R2** | 2 | 4.7kΩ | 0603 | I2C SCL & SDA Pull-up Resistors |
| **R_filter[0:7]**| 8 | 100Ω | 0603 | Optional Analog Filter Resistors |
| **C_filter[0:7]**| 8 | 10nF | 0603 | Optional Analog Filter Capacitors |
| **J1** | 1 | 4-Pin Male Header | 2.54mm Pitch | Programming / Debug Port (SWD) |
| **J2** | 1 | 5-Pin Male Header | 2.54mm Pitch | Main Interface (VDD, GND, SDA, SCL, ALERT) |
