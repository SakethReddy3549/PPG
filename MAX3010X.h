// MAX3010x.h - minimal I2C driver for MAX3010x family
#ifndef MAX3010X_H
#define MAX3010X_H

#include <Arduino.h>
#include <Wire.h>

class MAX3010x {
public:
  MAX3010x(TwoWire &wire, uint8_t address = 0x57);
  bool begin();
  void reset();
  void setup(uint8_t ledPower, uint8_t sampleRate, uint8_t pulseWidth);
  bool available();
  bool readSample(uint32_t &red, uint32_t &ir);
  uint8_t readPartId();
  void enableFifoInterrupt(bool en);

private:
  TwoWire &wire;
  uint8_t addr;
  bool hasData;
  uint32_t readRegister24(uint8_t reg);
  void writeRegister(uint8_t reg, uint8_t val);
};

#endif
