import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz, lfilter
import soundfile as sf

# Sampling rate and desired cutoff
fs = 44100
cutoff = 2000
order = 8

# Design a 2nd-order lowpass Butterworth filter
# We want 5 denominator coefficients, so we use order 2 (which gives 3 'a' coefficients).
# To force 5, we design a higher-order and truncate.
b_butter, a_butter = butter(N=order, Wn=cutoff/(fs/2), btype='low', analog=False)

# Truncate or pad the denominator to 5 elements
a = a_butter[:order+1]
b = [1.0]  # LPC-style all-pole IIR
gain_dc = sum(b) / sum(a)
b = [x / gain_dc for x in b]
print(a, b)
# Plot frequency response
w, h = freqz(b, a, worN=8000, fs=fs)

plt.figure(figsize=(10, 4))
plt.plot(w, 20 * np.log10(abs(h)))
plt.title("Frequency Response of IIR Filter (Cutoff 2000 Hz)")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude (dB)")
plt.grid(True)
plt.tight_layout()
plt.show()

# Apply filter to example audio (assume a mono 44.1kHz file)
try:
    audio, rate = sf.read("../resources/excitations/WhiteNoise.wav")
    if rate != fs:
        raise ValueError("Audio sample rate must be 44100 Hz")
    if audio.ndim > 1:
        audio = audio[:, 0]  # Use first channel if stereo

    filtered = lfilter(b, a, audio)
    sf.write("filtered_output8.wav", filtered, fs)
    result = "Filtering complete. Saved as 'filtered_output.wav'"
except FileNotFoundError:
    result = "Audio file 'input.wav' not found."
except Exception as e:
    result = str(e)

result
