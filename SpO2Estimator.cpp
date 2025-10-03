// SpO2Estimator.cpp - minimal estimator using ratio of ratios method
#include "SpO2Estimator.h"
#include <algorithm>
#include <numeric>

SpO2Estimator::SpO2Estimator(size_t ws): windowSize(ws), idx(0), full(false), lpAlpha(0.2f), hpAlpha(0.995f), lastFilteredRed(0), lastFilteredIr(0), lastDcRed(0), lastDcIr(0), prevFilteredIr(0), prevSlope(0), inBeat(false), beatPeakIr(0), beatTroughIr(0), beatPeakRed(0), beatTroughRed(0), samplesSinceBeat(0), spo2HistorySize(8), lastQuality(0.0f) {
  redBuf.assign(windowSize, 0.0f);
  irBuf.assign(windowSize, 0.0f);
  spo2History.reserve(spo2HistorySize);
}

void SpO2Estimator::addSample(float redRaw, float irRaw) {
  // store raw in circular buffer
  redBuf[idx] = redRaw;
  irBuf[idx] = irRaw;
  idx = (idx + 1) % windowSize;
  if (idx == 0) full = true;

  // streaming DC estimate (simple IIR) and HP output
  lastDcRed = hpAlpha * lastDcRed + (1.0f - hpAlpha) * redRaw;
  lastDcIr = hpAlpha * lastDcIr + (1.0f - hpAlpha) * irRaw;
  float hpRed = redRaw - lastDcRed;
  float hpIr = irRaw - lastDcIr;

  // low-pass smoothing of the HP signal
  lastFilteredRed = lpAlpha * lastFilteredRed + (1.0f - lpAlpha) * hpRed;
  lastFilteredIr = lpAlpha * lastFilteredIr + (1.0f - lpAlpha) * hpIr;

  // simple beat detection on filtered IR using slope/peak detection
  float slope = lastFilteredIr - prevFilteredIr;
  samplesSinceBeat++;

  // detect rising slope then a falling slope => peak
  if (!inBeat) {
    // look for start of beat (positive slope above threshold)
    if (slope > (0.02f * fabsf(prevFilteredIr) + 1e-6f)) {
      inBeat = true;
      beatPeakIr = lastFilteredIr;
      beatPeakRed = lastFilteredRed;
      beatTroughIr = lastFilteredIr;
      beatTroughRed = lastFilteredRed;
      samplesSinceBeat = 0;
    }
  } else {
    // update peak/trough
    if (lastFilteredIr > beatPeakIr) beatPeakIr = lastFilteredIr;
    if (lastFilteredIr < beatTroughIr) beatTroughIr = lastFilteredIr;
    if (lastFilteredRed > beatPeakRed) beatPeakRed = lastFilteredRed;
    if (lastFilteredRed < beatTroughRed) beatTroughRed = lastFilteredRed;

    // falling slope indicates beat peak passed
    if (slope < -0.01f || samplesSinceBeat > 200) {
      // compute AC amplitudes per-beat (peak-trough)/2
      float acIr = (beatPeakIr - beatTroughIr) / 2.0f;
      float acRed = (beatPeakRed - beatTroughRed) / 2.0f;
      float dcIr = lastDcIr > 1e-6f ? lastDcIr : 1.0f;
      float dcRed = lastDcRed > 1e-6f ? lastDcRed : 1.0f;

      if (acIr > 0.0f) {
        float R = (acRed / dcRed) / (acIr / dcIr);
        float spo2 = 110.0f - 25.0f * R;
        if (spo2 >= 0.0f && spo2 <= 100.0f) {
          pushSpo2(spo2);
          lastQuality = std::min(1.0f, (acIr/dcIr + acRed/dcRed) * 5.0f);
        }
      }

      // reset beat state
      inBeat = false;
      beatPeakIr = beatTroughIr = beatPeakRed = beatTroughRed = 0.0f;
      samplesSinceBeat = 0;
    }
  }

  prevFilteredIr = lastFilteredIr;
  prevSlope = slope;
}

void SpO2Estimator::computeACDC(float &acRed, float &dcRed, float &acIr, float &dcIr) {
  // fallback to windowed computation (original method)
  size_t count = full ? windowSize : idx;
  if (count < 3) {
    acRed = acIr = 0; dcRed = dcIr = 0;
    return;
  }

  float meanRed = std::accumulate(redBuf.begin(), redBuf.begin() + count, 0.0f) / count;
  float meanIr = std::accumulate(irBuf.begin(), irBuf.begin() + count, 0.0f) / count;
  float minR = redBuf[0], maxR = redBuf[0];
  float minI = irBuf[0], maxI = irBuf[0];
  for (size_t i = 1; i < count; ++i) {
    float r = redBuf[i];
    float ir = irBuf[i];
    if (r < minR) minR = r;
    if (r > maxR) maxR = r;
    if (ir < minI) minI = ir;
    if (ir > maxI) maxI = ir;
  }
  acRed = (maxR - minR) / 2.0f;
  acIr = (maxI - minI) / 2.0f;
  dcRed = meanRed;
  dcIr = meanIr;
}

void SpO2Estimator::pushSpo2(float v) {
  if (spo2History.size() >= spo2HistorySize) {
    // pop front
    spo2History.erase(spo2History.begin());
  }
  spo2History.push_back(v);
}

float SpO2Estimator::estimateSpO2() {
  if (spo2History.empty()) return -1.0f;
  float sum = 0.0f;
  for (float v : spo2History) sum += v;
  return sum / spo2History.size();
}

float SpO2Estimator::quality() {
  return lastQuality;
}
