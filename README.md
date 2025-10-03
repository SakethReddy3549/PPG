# On-device SpO2 (Seeed XIAO-nRF52840 + GY-MAX3010X)

## Summary

I implemented a lightweight, on-device pipeline to compute SpO2 using a GY-MAX3010X PPG sensor connected to a Seeed XIAO-nRF52840. The MCU reads raw red/IR samples over I2C, applies streaming filters, detects beats on the IR channel, extracts per-beat AC amplitudes, computes the ratio-of-ratios (R), and converts R to SpO2 using a simple calibration formula. A Python simulator is included for validation and parameter tuning.

## Repository layout

- `firmware/SpO2.ino` — Arduino-compatible sketch: initializes the sensor, reads FIFO samples, runs the estimator, and prints results to serial.
- `firmware/MAX3010x.h/.cpp` — Minimal I2C driver with a `readPartId()` helper (MAX30xxx variants differ in register maps).
- `firmware/SpO2Estimator.h/.cpp` — Streaming estimator: DC removal, low-pass smoothing, beat detection, per-beat AC extraction, ratio calculation, per-beat SpO2, and averaging.
- `test/simulate_spo2.py` — Python simulator that generates synthetic red/IR PPG signals with noise for offline tuning.

## How the estimator works

- **DC removal**: running IIR estimate of DC is subtracted from raw sample to isolate AC.
- **Low-pass smoothing**: the AC signal is lightly low-pass filtered to stabilize peak detection.
- **Beat detection**: detect rises and falls on filtered IR to capture peak and trough per beat.
- **Per-beat AC/DC**: AC ≈ (peak − trough)/2; DC ≈ current DC estimate. Compute R = (AC_red/DC_red) / (AC_ir/DC_ir).
- **Calibration**: SpO2 = A − B·R. Defaults: A = 110, B = 25 (requires calibration against a reference).
- **Output**: averages recent per-beat SpO2 values and reports a simple 0..1 quality metric.

## Why this approach

- Per-beat amplitude extraction isolates single cardiac pulses and reduces contamination from motion or noise.
- Streaming DC removal prevents baseline shifts from biasing AC/DC ratios.
- Lightweight implementation suitable for the XIAO-nRF52840 with room for optimization.

## Quick wiring & run steps

1. Wire the sensor to the XIAO-nRF52840 I2C pins:
   - SDA → SDA
   - SCL → SCL
   - VIN → 3.3V (confirm breakout voltage)
   - GND → GND
2. Open the `firmware` folder in Arduino IDE or PlatformIO, select the XIAO-nRF52840 board, compile and flash `SpO2.ino`.
3. Open the Serial Monitor at **115200** baud. On startup the sketch prints the sensor part ID (hex). Paste that here so I can update `MAX3010x.cpp` for the exact variant. The sketch then prints averaged SpO2 and a quality metric.

## Tuning suggestions (before calibration)

- If beats are missed or the estimator is unstable:
  - Print `lastFilteredIr` (temporary debug) to confirm pulsatile peaks are visible; if not, check wiring and LED current.
  - Increase `lpAlpha` (more smoothing) to reduce spike noise; decrease it for faster response.
  - Adjust `hpAlpha` to change DC tracking speed (closer to 1.0 tracks DC more slowly).
  - Scale beat detection thresholds to match your signal magnitude (thresholds are absolute and depend on ADC range and LED settings).

## Calibration & next steps

1. Run the sketch and share the printed sensor part ID. I'll update `MAX3010x.cpp` registers and FIFO order for that MAX30xxx variant.
2. Collect paired measurements (this device's R and a reference SpO2). I will compute the best-fit calibration parameters A and B for SpO2 = A − B·R.

## Possible improvements (future work)

- Bandpass filtering (0.5–5 Hz) implemented in fixed-point for MCU performance.
- Adaptive peak detection with dynamic thresholds and motion rejection.
- Accelerometer-based motion detection to ignore contaminated beats.
- Convert math to fixed-point to reduce CPU and memory usage (nRF52840 has an FPU, but fixed-point can be smaller/faster).

---
