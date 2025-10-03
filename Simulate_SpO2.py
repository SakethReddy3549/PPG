"""
Simple simulator to generate synthetic PPG red/IR signals
and run the same SpO2 estimator logic to help tune parameters.
"""
import math
import random
from collections import deque


class SimpleEstimator:
    def __init__(self, window_size=100):
        self.window_size = window_size
        self.red = deque(maxlen=window_size)
        self.ir = deque(maxlen=window_size)

    def add(self, r, ir):
        self.red.append(r)
        self.ir.append(ir)

    def estimate(self):
        if len(self.red) < 3:
            return None, 0.0
        mean_r = sum(self.red)/len(self.red)
        mean_i = sum(self.ir)/len(self.ir)
        min_r, max_r = min(self.red), max(self.red)
        min_i, max_i = min(self.ir), max(self.ir)
        ac_r = (max_r - min_r)/2.0
        ac_i = (max_i - min_i)/2.0
        if mean_r <= 0 or mean_i <= 0 or ac_i <= 0:
            return None, 0.0
        R = (ac_r/mean_r) / (ac_i/mean_i)
        spo2 = 110.0 - 25.0 * R
        spo2 = max(0.0, min(100.0, spo2))
        quality = min(1.0, (ac_i/mean_i + ac_r/mean_r) * 5.0)
        return spo2, quality


def synth_ppg(t, hr_bpm=70, spo2=98, noise=0.02):
    # create a synthetic PPG waveform: fundamental at heart rate, plus harmonics
    hr = hr_bpm / 60.0
    # amplitude scales with SpO2 roughly: lower spo2 -> change ratio between red/ir
    base_ir = 50000.0
    base_red = 50000.0
    ac = 0.02 * base_ir
    val_ir = base_ir + ac * math.sin(2*math.pi*hr*t) + 0.005*base_ir*math.sin(2*math.pi*2*hr*t)
    # make red slightly different amplitude depending on spo2
    val_red = base_red + ac * (1.0 - (100-spo2)/100.0*0.2) * math.sin(2*math.pi*hr*t)
    # add noise
    val_ir *= 1.0 + random.uniform(-noise, noise)
    val_red *= 1.0 + random.uniform(-noise, noise)
    return val_red, val_ir


def run_sim():
    est = SimpleEstimator(window_size=200)
    t = 0.0
    dt = 1.0/100.0
    for i in range(2000):
        r, ir = synth_ppg(t, hr_bpm=72, spo2=96, noise=0.01)
        est.add(r, ir)
        if i % 50 == 0:
            spo2, q = est.estimate()
            print(f"t={t:.2f}s spo2={spo2} quality={q:.2f}")
        t += dt


if __name__ == '__main__':
    run_sim()
