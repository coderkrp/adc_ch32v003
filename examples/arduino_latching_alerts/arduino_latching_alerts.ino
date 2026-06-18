/*
  CH32V003 I2C ADC Module - Latching Alerts Example
  
  This example demonstrates how to configure upper and lower window limits on
  the co-processor, monitor the physical ALERT pin using an interrupt service routine
  (ISR), and clear the latching (sticky) alarm flags once handled.

  Wiring:
    Arduino 5V/3.3V  ->  Module VDD
    Arduino GND     ->  Module GND
    Arduino SDA     ->  Module SDA (Requires external 4.7k pull-up to VDD)
    Arduino SCL     ->  Module SCL (Requires external 4.7k pull-up to VDD)
    Arduino D2      ->  Module PD2 (ALERT / Open-Drain Interrupt Line)
                        (Requires pull-up to VDD, either Arduino internal or external)

  Latching Alerts & ALERT Pin (PD2) Behavior:
    * The ALERT pin pulls low when limits are violated, triggering the Arduino interrupt.
    * Alarms are sticky and latching; they remain active until explicitly cleared
      by writing a '1' to the matching bit in the ALERT_STATUS register.
    * PD2 is shared with AIN3. If scanning 4 or more channels, PD2 functions as AIN3
      and is briefly forced open-drain LOW during alerts. If scanning 3 or fewer,
      PD2 is a dedicated open-drain interrupt output.
      See docs/window_limit_alerts.md for more details.
*/

#include <Wire.h>

const uint8_t ADC_MODULE_I2C_ADDR = 0x24;

// Register Address Definitions
const uint8_t REG_LIMIT_HIGH_CH0 = 0x24;
const uint8_t REG_LIMIT_LOW_CH0  = 0x26;
const uint8_t REG_ALERT_STATUS   = 0x44;

// Arduino Pin mapping for D2 (external interrupt 0 on Uno/Nano)
const uint8_t INTERRUPT_PIN = 2;

// Volatile flag set by the ISR
volatile bool alertTriggered = false;

// ISR (Interrupt Service Routine)
void handleAlertInterrupt() {
  alertTriggered = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("=== CH32V003 ADC Module latching_alerts Example ==="));

  // Initialize I2C Master
  Wire.begin();
  Wire.setClock(400000);

  // 1. Configure the ALERT pin on the Arduino (needs a pull-up because module is Open-Drain)
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleAlertInterrupt, FALLING);

  // 2. Configure Limits on Channel 0
  // Safe window: High = 2500mV, Low = 1000mV
  // If CH0 voltage exceeds 2.5V or falls below 1.0V, the alert will trigger
  Serial.println(F("Setting CH0 limits: Low = 1000 mV, High = 2500 mV..."));
  writeRegister16(REG_LIMIT_HIGH_CH0, 2500);
  writeRegister16(REG_LIMIT_LOW_CH0, 1000);

  // 3. Clear any existing alerts
  // Write 0xFF to clear all bits (write '1' to clear)
  writeRegister8(REG_ALERT_STATUS, 0xFF);
  alertTriggered = false;

  Serial.println(F("Setup complete. Monitoring..."));
}

void loop() {
  // Check if ISR flagged an interrupt
  if (alertTriggered) {
    alertTriggered = false; // Reset local flag
    Serial.println(F("\n>>> ALERT PIN PULLED LOW! <<<"));

    // Read ALERT_STATUS register to see which channel violated limits
    uint8_t status = readRegister8(REG_ALERT_STATUS);
    Serial.print(F("ALERT_STATUS Bitmask: 0x"));
    Serial.println(status, HEX);

    for (int ch = 0; ch < 8; ch++) {
      if (status & (1 << ch)) {
        Serial.print(F("-> Violation detected on Channel "));
        Serial.println(ch);

        // [Take safety actions here, e.g. turn off relays, print warnings]

        // Clear the sticky alarm for this channel by writing a '1' back to its bit
        Serial.print(F("Clearing alarm for Channel "));
        Serial.println(ch);
        writeRegister8(REG_ALERT_STATUS, (1 << ch));
      }
    }
    
    Serial.println(F("Alarm cleared. Returning to monitoring...\n"));
  }

  // Debug indicator to show the loop is running
  static uint32_t lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 5000) {
    lastHeartbeat = millis();
    Serial.println(F("Heartbeat: Monitoring background sequences..."));
  }
}

// Helper: Write a 16-bit register (Big-Endian format)
void writeRegister16(uint8_t regAddress, uint16_t value) {
  Wire.beginTransmission(ADC_MODULE_I2C_ADDR);
  Wire.write(regAddress);
  Wire.write((uint8_t)(value >> 8));   // MSB
  Wire.write((uint8_t)(value & 0xFF)); // LSB
  if (Wire.endTransmission() != 0) {
    Serial.println(F("Error: I2C write16 failed."));
  }
}

// Helper: Write a 8-bit register
void writeRegister8(uint8_t regAddress, uint8_t value) {
  Wire.beginTransmission(ADC_MODULE_I2C_ADDR);
  Wire.write(regAddress);
  Wire.write(value);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("Error: I2C write8 failed."));
  }
}

// Helper: Read a 8-bit register
uint8_t readRegister8(uint8_t regAddress) {
  Wire.beginTransmission(ADC_MODULE_I2C_ADDR);
  Wire.write(regAddress);
  if (Wire.endTransmission(false) != 0) {
    Serial.println(F("Error: I2C read8 request failed."));
    return 0;
  }
  
  uint8_t bytesReceived = Wire.requestFrom(ADC_MODULE_I2C_ADDR, (uint8_t)1);
  if (bytesReceived == 1) {
    return Wire.read();
  }
  return 0;
}
