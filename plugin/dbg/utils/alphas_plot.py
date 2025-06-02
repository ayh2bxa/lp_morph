import numpy as np 
from scipy import signal 
import matplotlib.pyplot as plt 
from scipy.signal import tf2zpk

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

def analyse_alphas(G, alphas, input, nfft, sr):
    # Get the frequency response
    w, h = signal.freqz(G, alphas, worN=nfft, fs=sr)
    
    plt.figure(figsize=(12, 6))
    
    # Plot magnitude response in dB
    plt.semilogx(w, 20 * np.log10(np.abs(h)))
    plt.title(f'Frequency Response')
    plt.ylabel('Magnitude (dB)')
    plt.grid(True)
    plt.semilogx(w, 20 * np.log10(np.abs(np.fft.fft(input, nfft))))
    plt.tight_layout()
    plt.show()
