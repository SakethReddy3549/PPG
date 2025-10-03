// Basic sketch to read GY-MAX3010X and compute SpO2 on-board

#include <Wire.h>
#include "MAX3010x.h"
#include "SpO2Estimator.h"

// I2C address for MAX3010x typical
#define MAX3010X_ADDR 0x57

MAX3010x sensor(Wire, MAX3010X_ADDR);
SpO2Estimator estimator(100); // window size in samples

unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("SpO2 on-device demo");

  Wire.begin();

  if (!sensor.begin()) {
    Serial.println("MAX3010x not found, check wiring");
    while (1) { delay(1000); }
  }
  uint8_t part = sensor.readPartId();
  Serial.print("MAX3010x part id: 0x");
  Serial.println(part, HEX);

  // Basic configuration - adjust for your board and sensor
  sensor.reset();
  delay(100);
  sensor.setup(0x1F, 0x03, 0x03); // led power, sample rate, pulse width (example values)
  sensor.enableFifoInterrupt(false);

  Serial.println("Sensor initialized");
}

void loop() {
  // Read FIFO until empty and feed estimator
  while (sensor.available()) {
    uint32_t red = 0, ir = 0;
    if (sensor.readSample(red, ir)) {
      estimator.addSample((float)red, (float)ir);
    }
  }

  // Print an estimate every 1000 ms
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    float spo2 = estimator.estimateSpO2();
    float confidence = estimator.quality();
    Serial.print("SpO2: ");
    if (spo2 < 0 || spo2 > 100) Serial.print("--   "); else Serial.print(spo2, 1);
    Serial.print(" %, quality: ");
    Serial.println(confidence, 2);
  }

  delay(10);
}
