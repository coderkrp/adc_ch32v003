# Contributing to CH32V003 I2C ADC Module

Thank you for your interest in improving this project! We welcome code contributions, documentation enhancements, issue reports, and feedback.

---

## How to Contribute

### 1. Reporting Bugs
Before opening a new issue, please search the existing issues to ensure it hasn't been reported already. When filing a bug report, please include:
*   **Target Hardware Setup:** Schematic or wiring diagram (resistors, capacitor values).
*   **Power Domain:** Voltage level (3.3V or 5.0V) and source (e.g. Master VDD, USB debugger).
*   **Master MCU:** Board model (e.g., ESP32-WROOM, Arduino Uno, RPi 4) and software SDK/library (e.g., Wire.h, ESP-IDF HAL).
*   **I2C Bus speed:** 100kHz or 400kHz.
*   **Detailed Description:** Steps to reproduce, expected behavior, and actual results.
*   **Logs:** Serial monitor logs from the CH32V003 debug SWIO printfs (if active).

### 2. Submitting Pull Requests
1.  **Fork the Repository:** Create your own copy of the repo on GitHub.
2.  **Create a Feature Branch:** Choose a descriptive name (e.g., `feature/oversampling-support` or `fix/i2c-write-boundary`).
3.  **Code Styling guidelines:**
    *   Indent with 4 spaces (no tabs).
    *   Maintain comments for registers and calculations.
    *   Ensure the code compiles without warnings in PlatformIO using the `pio run` task.
    *   Verify taking up minimal flash and RAM footprints (ensure it fits the 16KB flash / 2KB RAM boundary of the CH32V003F4P6).
4.  **Reference Guides:** Verify any additions align with [ADC Linearity & Voltage Divider Design Guide](docs/adc_linearity_guide.md) and [Firmware & Hardware Architecture Design](docs/architecture_design.md).
5.  **Submit the PR:** Describe your changes clearly in the PR template.

### 3. Suggestions and Feature Requests
If you have suggestions (e.g., adding dynamic calibration, advanced math functions like NTC linearization, oversampling, etc.), please open a feature request issue. Keep in mind:
*   **CH32V003 constraints:** The chip only has 16KB Flash and 2KB RAM. Features must be highly optimized and lightweight.
*   **I2C constraints:** Command execution must be fast to avoid blocking the I2C interrupt sequence.
