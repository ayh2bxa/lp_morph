import numpy as np 
from scipy import signal 
import matplotlib.pyplot as plt 
from scipy.signal import tf2zpk
from scipy.fft import fft, fftfreq

def plot_pole_zero(G, alphas, nfft, sr):
    plt.figure(figsize=(10, 8))
    
    # Get poles from LPC coefficients (roots of A(z))
    z, poles, k = tf2zpk(G, alphas)
    print(np.abs(poles))
    print(poles)
    
    # Plot unit circle
    circle = plt.Circle((0, 0), 1, fill=False, color='black', linestyle='--', alpha=0.3)
    plt.gca().add_patch(circle)
    # Plot poles
    plt.scatter(np.real(poles), np.imag(poles), marker='x', color='red', label='Poles')
    
    # Plot zero at origin (since this is an all-pole model)
    plt.scatter([0], [0], marker='o', facecolors='none', edgecolors='blue', label='Zero')
    
    plt.title(f'Pole-Zero Plot')
    plt.xlabel('Real')
    plt.ylabel('Imaginary')
    plt.axis('equal')
    plt.grid(True)
    plt.legend()
    plt.show()

def analyse_alphas(G, alphas, input, nfft, sr, i):
    # Get the frequency response
    w, h = signal.freqz(G, alphas, worN=nfft, whole=True, fs=sr)
    plt.figure(figsize=(12, 6))
    if len(input) < nfft:
        tmp = np.zeros(nfft)
        tmp[:len(input)] = input 
        input = tmp
    freqs = np.fft.fftfreq(nfft, 1/sr)
    pos_mask = freqs >= 0
    freqs = freqs[pos_mask]
    Xmag = np.abs(np.fft.fft(input))
    Xmag = Xmag[pos_mask]*2/nfft
    h_pos = h[pos_mask]
    # plt.semilogx(freqs, 20*np.log(Xmag))
    # plt.semilogx(freqs, 20*np.log(np.abs(h_pos)))
    t = np.arange(nfft)/sr
    ht = np.real(np.fft.ifft(h))
    plt.plot(t, input)
    plt.plot(t, ht)
    plt.title(str(i))
    plt.show()
    # # Plot magnitude response in dB
    # # plt.semilogx(w, 20 * np.log10(np.abs(h)))
    # plt.plot(w, 20 * np.log10(np.abs(h)))
    # plt.title(f'Frequency Response')
    # plt.ylabel('Magnitude (dB)')
    # plt.grid(True)
    # plt.semilogx(w, 20 * np.log10(np.abs(fft(input))))
    # plt.tight_layout()
    # plt.show()
