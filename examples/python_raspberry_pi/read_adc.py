#!/usr/bin/env python3
"""
CH32V003 I2C ADC Module - Raspberry Pi Python Example

This script demonstrates how to interface a Raspberry Pi (or any Linux SBC)
with the CH32V003 ADC Module using Python and the reusable ch32v003_adc driver class.
It reads the drift-corrected system VDD and calculates the physical external
voltages on Channel 0 based on a scaling resistor divider network.

Requirements:
  pip install smbus2

Wiring (Raspberry Pi Pinout):
  VDD   -> Pin 1 (3.3V) or Pin 2 (5.0V) (Match logic levels!)
  GND   -> Pin 6 (GND)
  SDA   -> Pin 3 (GPIO 2 / I2C SDA)
  SCL   -> Pin 5 (GPIO 3 / I2C SCL)

Linearity Warning:
  The CH32V003 ADC is non-linear below ~300mV and above VDD - 300mV.
  Ensure your input voltages fall within the 10% to 90% range of VDD.
  See docs/adc_linearity_guide.md for more details.
"""

import time
import sys
from ch32v003_adc import CH32V003_ADC

# Resistor divider values from docs/adc_linearity_guide.md for 12V battery scaling
# R1 = 39k, R2 = 10k => Ratio = R2 / (R1 + R2) = 10 / 49 = 0.2041
R1_OHMS = 39000.0
R2_OHMS = 10000.0
DIVIDER_RATIO = R2_OHMS / (R1_OHMS + R2_OHMS)

def main():
    print("=== CH32V003 ADC Module Raspberry Pi Python Example ===")
    
    # Initialize the driver on standard I2C bus 1
    try:
        adc = CH32V003_ADC(bus_id=1)
    except FileNotFoundError:
        print("Error: I2C bus 1 not found. Please enable I2C via raspi-config.")
        sys.exit(1)
    except Exception as e:
        print(f"Error initializing I2C connection: {e}")
        sys.exit(1)

    print("I2C Bus initialized. Reading data (Ctrl+C to exit)...")
    print("-" * 60)

    try:
        while True:
            # 1. Read Drift-Corrected VDD rail
            vdd_mv = adc.read_vdd()
            
            # 2. Read Voltage on CH0 (Pin input voltage in mV)
            ch0_mv = adc.read_channel_mv(0)
            
            if vdd_mv is not None and ch0_mv is not None:
                # 3. Scale back to real battery voltage (using divider ratio)
                # Real Volts = (Measured pin mV / Divider Ratio) / 1000
                battery_volts = (ch0_mv / DIVIDER_RATIO) / 1000.0
                
                print(f"System VDD: {vdd_mv} mV | CH0 Pin: {ch0_mv} mV | Scaled Battery: {battery_volts:.2f} V")
            
            time.sleep(1.0) # Poll once per second
            
    except KeyboardInterrupt:
        print("\nExiting program.")
    finally:
        adc.close()

if __name__ == "__main__":
    main()
