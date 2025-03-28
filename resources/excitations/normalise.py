import numpy as np
import soundfile as sf 

def normalize_to_rms(audio, target_rms=0.002520212887725626):
    current_rms = np.sqrt(np.mean(audio ** 2))
    if current_rms == 0:
        return audio  # Avoid division by zero
    gain = target_rms / current_rms
    return audio * gain
name = 'WhiteNoise.wav'
audio, fs = sf.read(name)
sf.write(name, normalize_to_rms(audio), fs)