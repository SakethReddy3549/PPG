// SpO2Estimator.h - simple on-device SpO2 estimator
#ifndef SPO2_ESTIMATOR_H
#define SPO2_ESTIMATOR_H

#include <Arduino.h>
#include <vector>

class SpO2Estimator {
public:
  SpO2Estimator(size_t windowSize = 100);
  void addSample(float red, float ir);
  // Returns averaged SpO2 across recent beats, or -1 if not available
  float estimateSpO2();
  // Quality in range 0..1
  float quality();

  // tuning parameters (run-time adjustable)
  void setLowpassAlpha(float a) { lpAlpha = a; }
  void setHighpassAlpha(float a) { hpAlpha = a; }

private:
  // raw circular buffer (used for fallback computations)
  size_t windowSize;
  std::vector<float> redBuf, irBuf;
  size_t idx;
  bool full;

  // streaming filter state
  float lpAlpha; // low-pass smoothing (0..1, higher = smoother)
  float hpAlpha; // high-pass DC estimation coefficient (close to 1.0)
  float lastFilteredRed;
  float lastFilteredIr;
  float lastDcRed;
  float lastDcIr;

  // beat detection state (on IR)
  float prevFilteredIr;
  float prevSlope;
  bool inBeat;
  float beatPeakIr;
  float beatTroughIr;
  float beatPeakRed;
  float beatTroughRed;
  unsigned long samplesSinceBeat;

  // per-beat SpO2 history for smoothing
  std::vector<float> spo2History;
  size_t spo2HistorySize;
  float lastQuality;

  // helpers
  void computeACDC(float &acRed, float &dcRed, float &acIr, float &dcIr);
  void pushSpo2(float v);
};

#endif
