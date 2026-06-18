# Changelog

All notable changes for the CH32V003 I2C ADC Module are documented here.

## [v1.0.0] — 2026-06-18

### Added

- **10-bit ADC with oversampling support** — Core firmware for the CH32V003F4P6 performing continuous multi-channel analog scanning with configurable oversampling (up to 12-bit effective resolution via EMA low-pass filtering).
- **I2C slave interface** — Configurable slave address (default `0x24`) with register-based read access for all analog channel readings and device status.
- **Supply voltage drift compensation** — Internal 1.2V bandgap reference used to correct for VREF drift in hardware-calibrated mode.
- **Autonomous window limit checks** — Configurable per-channel high/low thresholds with an ALERT pin (PD2, active-low) for out-of-range notifications.
- **PlatformIO compatible build** — Full PlatformIO project configuration (`platformio.ini`) for the ch32v003fun framework.

### Documentation

- Comprehensive README with architecture diagram, feature overview, and comparison vs. dedicated ADC chips.
- Design guides: ADC linearity, voltage divider design, oversampling techniques, and troubleshooting.
- Arduino and Python integration examples for quick start.
- Contributing guide and Code of Conduct.
- GitHub Actions CI workflow for build verification.
- Issue templates for bug reports and feature requests.
