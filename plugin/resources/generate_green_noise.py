import numpy as np
import scipy.signal
import soundfile as sf

def generate_green_noise(duration, sample_rate=44100, amplitude=0.5):
    """Generate stereo green noise (amplifies mid-range frequencies)"""
    samples = int(duration * sample_rate)

    # Generate white noise for both channels
    white_noise_left = np.random.normal(0, 1, samples)
    white_noise_right = np.random.normal(0, 1, samples)

    # Create green noise filter (bandpass emphasizing mid frequencies)
    nyquist = sample_rate // 2
    frequencies = np.fft.fftfreq(samples, 1/sample_rate)
    freq_abs = np.abs(frequencies)

    # Create bandpass filter response that emphasizes mid frequencies
    # Peak around 1-4 kHz range (typical for green noise)
    center_freq = 2000  # Hz
    bandwidth = 3000    # Hz

    filter_response = np.zeros_like(frequencies, dtype=float)

    # Avoid division by zero and create bandpass response
    non_zero_mask = freq_abs > 0

    # Gaussian-like bandpass filter centered on mid frequencies
    filter_response[non_zero_mask] = np.exp(-0.5 * ((freq_abs[non_zero_mask] - center_freq) / bandwidth) ** 2)

    # Boost the mid-range frequencies more
    filter_response = np.sqrt(filter_response)

    # Apply filter to both channels
    fft_left = np.fft.fft(white_noise_left)
    fft_right = np.fft.fft(white_noise_right)

    filtered_fft_left = fft_left# * filter_response
    filtered_fft_right = fft_right# * filter_response

    green_noise_left = np.real(np.fft.ifft(filtered_fft_left))
    green_noise_right = np.real(np.fft.ifft(filtered_fft_right))

    # Normalize and apply amplitude
    green_noise_left = green_noise_left / np.max(np.abs(green_noise_left)) * amplitude
    green_noise_right = green_noise_right / np.max(np.abs(green_noise_right)) * amplitude

    return np.column_stack((green_noise_left, green_noise_right))

if __name__ == "__main__":
    # Generate 10 seconds of stereo green noise
    duration = 2.0
    sample_rate = 44100

    stereo_green_noise = generate_green_noise(duration, sample_rate)

    # Save as WAV file
    sf.write('WhiteNoise.wav', stereo_green_noise, sample_rate)
    print(f"Generated {duration}s of stereo green noise saved as 'green_noise_stereo.wav'")