---
name: Bug report
about: Create a report to help us improve the firmware or drivers
title: '[BUG] '
labels: bug
assignees: ''

---

**Describe the Bug**
A clear and concise description of what the bug is.

**To Reproduce**
Steps to reproduce the behavior:
1. Initialize the module with configuration '...'
2. Perform I2C read/write on register '...'
3. See error

**Hardware Setup (Important)**
*   **CH32V003 Package:** TSSOP20 / QFN20 / SOP8
*   **Master MCU:** (e.g. ESP32, STM32, Arduino Uno, Raspberry Pi)
*   **Logic Level Voltage:** 3.3V or 5.0V
*   **I2C Bus Pull-Ups:** (e.g. 4.7k ohms to VDD)
*   **Analog Input configuration:** (resistor dividers, filter capacitors used - see docs/adc_linearity_guide.md)

**Firmware Configuration (CONFIG Register)**
*   Filter strength (alpha):
*   Number of channels scanned:

**Expected Behavior**
A clear and concise description of what you expected to happen.

**Screenshots/Serial Logs**
If applicable, add serial monitor output logs (from minichlink debug printfs) or oscilloscope screenshots of I2C bus traffic to help explain your problem.

**Additional Context**
Add any other context about the problem here.
