/*
  CH32V003 I2C ADC Module - Basic Polling Example
  
  This example demonstrates how to interface an Arduino with the CH32V003 ADC Module.
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

#include <Wire.h>

// Default I2C Address of the CH32V003 ADC Module
const uint8_t ADC_MODULE_I2C_ADDR = 0x24;

// Register Address Definitions
const uint8_t REG_VDD_MV = 0x02;
const uint8_t REG_RAW_CH0 = 0x04;
const uint8_t REG_MV_CH0 = 0x06;

// Resistor divider values from docs/adc_linearity_guide.md for 12V battery scaling
// R1 = 39k, R2 = 10k => Ratio = R2 / (R1 + R2) = 10 / 49 = 0.2041
const float R1_OHMS = 39000.0;
const float R2_OHMS = 10000.0;
const float DIVIDER_RATIO = R2_OHMS / (R1_OHMS + R2_OHMS);

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor to open (USB boards)

  Serial.println(F("=== CH32V003 ADC Module basic_polling Example ==="));

  // Initialize I2C bus as Master
  Wire.begin();
  Wire.setClock(400000); // 400kHz fast mode
}

void loop() {
  uint16_t vdd_mv = readRegister16(REG_VDD_MV);
  uint16_t raw_ch0 = readRegister16(REG_RAW_CH0);
  uint16_t mv_ch0 = readRegister16(REG_MV_CH0);

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

// Read a 16-bit register from the I2C Slave (Big-Endian format)
uint16_t readRegister16(uint8_t regAddress) {
  // 1. Point to the desired register
  Wire.beginTransmission(ADC_MODULE_I2C_ADDR);
  Wire.write(regAddress);
  if (Wire.endTransmission(false) != 0) {
    Serial.println(F("Error: I2C write failed."));
    return 0;
  }

  // 2. Request 2 bytes of data
  uint8_t bytesReceived = Wire.requestFrom(ADC_MODULE_I2C_ADDR, (uint8_t)2);
  if (bytesReceived == 2) {
    uint8_t msb = Wire.read(); // Big-endian: MSB first
    uint8_t lsb = Wire.read(); // LSB second
    return ((uint16_t)msb << 8) | lsb;
  }

  Serial.println(F("Error: RequestFrom failed."));
  return 0;
}
