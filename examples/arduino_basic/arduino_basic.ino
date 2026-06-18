/*
  CH32V003 I2C ADC Module - Basic Polling Example
  
  This example demonstrates how to interface an Arduino with the CH32V003 ADC Module
  using the reusable CH32V003_ADC driver class.
  It reads the system VDD rail voltage (corrected for drift) and reads Channel 0,
  demonstrating how to scale the millivolt reading back to a high-voltage battery reading.

  Wiring:
    Arduino 5V/3.3V  ->  Module VDD
    Arduino GND     ->  Module GND
    Arduino SDA     ->  Module SDA (Requires external 4.7k pull-up to VDD)
    Arduino SCL     ->  Module SCL (Requires external 4.7k pull-up to VDD)

  Linearity Warning:
    The CH32V003 ADC is non-linear below ~300mV and above VDD - 300mV.
    Ensure your input voltages fall within the 10% to 90% range of VDD.
    See docs/adc_linearity_guide.md for more details.
*/

#include "CH32V003_ADC.h"

// Resistor divider values from docs/adc_linearity_guide.md for 12V battery scaling
// R1 = 39k, R2 = 10k => Ratio = R2 / (R1 + R2) = 10 / 49 = 0.2041
const float R1_OHMS = 39000.0;
const float R2_OHMS = 10000.0;
const float DIVIDER_RATIO = R2_OHMS / (R1_OHMS + R2_OHMS);

// Instantiate the driver class
CH32V003_ADC adc;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor to open (USB boards)

  Serial.println(F("=== CH32V003 ADC Module basic_polling Example ==="));

  // Initialize the driver and communication
  if (adc.begin()) {
    Serial.println(F("Communication with CH32V003 ADC Module established successfully."));
  } else {
    Serial.println(F("Error: Could not communicate with CH32V003 ADC Module. Check wiring!"));
  }

  Wire.setClock(400000); // 400kHz fast mode
}

void loop() {
  uint16_t vdd_mv = adc.readVDD();
  uint16_t raw_ch0 = adc.readRawChannel(0);
  uint16_t mv_ch0 = adc.readMilliVolts(0);

  // Convert raw value back to millivolts locally (optional check)
  float local_mv = (raw_ch0 * (float)vdd_mv) / 1023.0;

  // Scale back to real battery voltage (using divider ratio)
  // Real Volts = (Measured pin mV / Divider Ratio) / 1000
  float battery_volts = ((float)mv_ch0 / DIVIDER_RATIO) / 1000.0;

  Serial.print(F("System VDD: "));
  Serial.print(vdd_mv);
  Serial.print(F(" mV | CH0 Raw: "));
  Serial.print(raw_ch0);
  Serial.print(F(" | CH0 Pin: "));
  Serial.print(mv_ch0);
  Serial.print(F(" mV (calculated: "));
  Serial.print(local_mv, 1);
  Serial.print(F(" mV) | Scaled Battery: "));
  Serial.print(battery_volts, 2);
  Serial.println(F(" V"));

  delay(1000); // Poll once per second
}
