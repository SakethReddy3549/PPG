#include "MAX3010X.h"

// register addresses are illustrative; adjust to datasheet for exact part
static const uint8_t REG_PART_ID = 0xFF;
static const uint8_t REG_FIFO_DATA = 0x07;
static const uint8_t REG_FIFO_WR_PTR = 0x04;
static const uint8_t REG_FIFO_OVF_CNT = 0x05;
static const uint8_t REG_FIFO_RD_PTR = 0x06;

MAX3010x::MAX3010x(TwoWire &wire, uint8_t address): wire(wire), addr(address), hasData(false) {}

bool MAX3010x::begin() {
  wire.beginTransmission(addr);
  if (wire.endTransmission() != 0) return false;
  // try read part id
  uint8_t id = readPartId();
  (void)id;
  return true;
}

uint8_t MAX3010x::readPartId() {
  // Attempt to read a single-part ID register. The register address may differ; we try REG_PART_ID.
  wire.beginTransmission(addr);
  wire.write(REG_PART_ID);
  if (wire.endTransmission(false) != 0) return 0xFF;
  wire.requestFrom((int)addr, 1);
  if (!wire.available()) return 0xFF;
  return wire.read();
}

void MAX3010x::reset() {
  // soft reset command placeholder - consult datasheet
  writeRegister(0x09, 0x40);
}

void MAX3010x::setup(uint8_t ledPower, uint8_t sampleRate, uint8_t pulseWidth) {
  // write LED power and adc settings - registers differ per variant; these are placeholders
  writeRegister(0x0A, ledPower);
  writeRegister(0x0B, (sampleRate << 2) | (pulseWidth & 0x03));
}

void MAX3010x::enableFifoInterrupt(bool en) {
  writeRegister(0x02, en ? 0x80 : 0x00);
}

bool MAX3010x::available() {
  // read fifo pointers to see if data available
  uint32_t wr = readRegister24(REG_FIFO_WR_PTR);
  uint32_t rd = readRegister24(REG_FIFO_RD_PTR);
  hasData = (wr != rd);
  return hasData;
}

bool MAX3010x::readSample(uint32_t &red, uint32_t &ir) {
  // read 6 bytes (3 bytes IR, 3 bytes Red) or variant order; this is a minimal example
  wire.beginTransmission(addr);
  wire.write(REG_FIFO_DATA);
  if (wire.endTransmission(false) != 0) return false;
  // request 6 bytes
  wire.requestFrom((int)addr, 6);
  if (wire.available() < 6) return false;
  uint8_t b[6];
  for (int i = 0; i < 6; ++i) b[i] = wire.read();
  ir = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
  red = ((uint32_t)b[3] << 16) | ((uint32_t)b[4] << 8) | b[5];
  return true;
}

uint32_t MAX3010x::readRegister24(uint8_t reg) {
  wire.beginTransmission(addr);
  wire.write(reg);
  if (wire.endTransmission(false) != 0) return 0;
  wire.requestFrom((int)addr, 1);
  if (!wire.available()) return 0;
  return (uint32_t)wire.read();
}

void MAX3010x::writeRegister(uint8_t reg, uint8_t val) {
  wire.beginTransmission(addr);
  wire.write(reg);
  wire.write(val);
  wire.endTransmission();
}
