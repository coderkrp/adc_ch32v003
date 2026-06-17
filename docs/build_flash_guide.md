# Build and Flash Guide (PlatformIO)
## CH32V003 I2C ADC Module

This guide details how to set up the compile toolchain, configure VSCode/PlatformIO, configure Linux `udev` USB permissions, and build, flash, and monitor the firmware.

---

## 1. Prerequisites & PlatformIO Setup

PlatformIO is the recommended environment for compiling, flashing, and debugging this project. It manages toolchains and frameworks automatically.

### 1.1 Installation
1. Install **Visual Studio Code (VSCode)**.
2. Open VSCode, navigate to the Extensions tab (`Ctrl+Shift+X`), and search for and install **PlatformIO IDE**.
3. (Optional) For command-line integration, ensure the `pio` command-line tools are installed in your system path.

---

## 2. WCH-LinkE USB Permission Setup (udev rules)

On Linux systems, the kernel restricts raw USB communication with the WCH-LinkE programmer to the `root` user by default. To flash and debug as a normal user, you must set up udev rules.

1. Create a new rules file:
   ```bash
   sudo nano /etc/udev/rules.d/99-wch-link.rules
   ```
2. Paste the following configuration lines:
   ```udev
   # WCH-LinkE in RV (RISC-V) Mode
   SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8010", MODE="0666", GROUP="plugdev"
   SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="8012", MODE="0666", GROUP="plugdev"
   ```
3. Save the file and reload the udev daemon:
   ```bash
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```
4. **Re-plug** your WCH-LinkE programmer into your PC for the rules to take effect.

---

## 3. Project Configuration (`platformio.ini`)

Create a `platformio.ini` file in the root directory of your project. This configuration uses the community-maintained WCH RISC-V platform and the lightweight `ch32v003fun` framework.

```ini
[env:genericCH32V003F4P6]
platform = https://github.com/Community-PIO-CH32V/platform-ch32v.git
board = genericCH32V003F4P6
framework = ch32v003fun

; Monitor configuration for ch32fun high-speed debug printfs
monitor_speed = 115200

; Upload and Debug protocols (uses WCH-LinkE)
upload_protocol = minichlink
```

> [!NOTE]
> The `ch32v003fun` framework automatically downloads and links the minimal startup headers and files, eliminating the need to compile the vendor's heavy HAL libraries.

---

## 4. Project Directory Structure

Your PlatformIO project directory should be structured as follows:

```text
adc_ch32v003/
├── .pio/               # PlatformIO build artifacts (auto-generated)
├── docs/               # Reference design documentation
│   ├── architecture_design.md
│   ├── build_flash_guide.md
│   ├── hardware_design.md
│   ├── i2c_integration_guide.md
│   └── requirements_spec.md
├── include/
│   └── funconfig.h     # ch32fun configuration overrides
├── src/
│   └── main.c          # Core firmware source code
└── platformio.ini      # PlatformIO configuration file
```

---

## 5. Build, Flash, and Monitor Commands

### 5.1 Using VSCode IDE (Graphical Interface)
*   **Compile Code:** Click the **Checkmark** button ($\checkmark$) in the blue bottom status bar, or press `Ctrl+Alt+B`.
*   **Flash Firmware:** Click the **Right Arrow** button ($\rightarrow$) in the status bar to upload code to the chip over the WCH-LinkE SWIO connection.
*   **Debug Console (Printf monitor):** Click the **Plug** icon in the status bar to open the serial monitor and capture high-speed `ch32fun` printfs.

### 5.2 Using PlatformIO Core CLI (Terminal)
If you prefer building from the terminal, use the following command-line shortcuts:

#### Build Project
```bash
pio run
```

#### Flash/Upload to Chip
```bash
pio run --target upload
```

#### View Debug Prints (Monitor)
```bash
pio device monitor
```
*(Press `Ctrl+C` to exit the monitor).*

#### Clean Build Files
```bash
pio run --target clean
```

---

## 6. Legacy Command-Line Makefile (Alternative)

If you prefer building without PlatformIO using raw GCC and `make`, the project supports the traditional `ch32fun` Make-based build toolchain:

1. Install dependencies:
   ```bash
   sudo apt install -y build-essential gcc-riscv64-unknown-elf libusb-1.0-0-dev
   ```
2. Build code:
   ```bash
   make
   ```
3. Flash code:
   ```bash
   make flash
   ```
4. Monitor debug outputs:
   ```bash
   make monitor
   ```
