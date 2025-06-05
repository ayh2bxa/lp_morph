import numpy as np
import matplotlib.pyplot as plt

# Parameters for the sine wave
fs = 44100            # Sampling rate (Hz)
duration = 0.02      # Duration of the signal (seconds)
f = 440               # Frequency of the sine wave (Hz)

# Generate time vector and sine wave
t = np.arange(0, duration, 1/fs)
x = np.sin(2 * np.pi * f * t)

# Compute FFT
X = np.fft.fft(x)
N = len(X)
freqs = np.fft.fftfreq(N, 1/fs)

# Keep only positive frequencies
pos_mask = freqs >= 0
freqs_pos = freqs[pos_mask]
X_mag = np.abs(X[pos_mask]) * 2 / N  # Scale magnitude

# Plot the time-domain sine wave
plt.figure()
plt.plot(t, x)
plt.xlabel('Time [s]')
plt.ylabel('Amplitude')
plt.title(f'Sine Wave ({f} Hz)')

# Plot the magnitude spectrum
plt.figure()
plt.semilogx(freqs_pos, 20*np.log10(X_mag))
plt.xlabel('Frequency [Hz]')
plt.ylabel('Magnitude')
plt.title('FFT Magnitude Spectrum')
plt.tight_layout()
plt.show()
