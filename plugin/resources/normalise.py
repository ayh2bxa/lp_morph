import numpy as np
import soundfile as sf 
import wave
from scipy import signal

# def normalize_to_rms(audio, target_rms=0.25):
#     # if (audio.shape[1] > 1):
#     #     audio = 0.5*(audio[:, 0]+audio[:, 1])
#     current_rms = np.sqrt(np.mean(audio ** 2))
#     if current_rms == 0:
#         return audio  # Avoid division by zero
#     gain = target_rms / current_rms
#     print(target_rms, current_rms)
#     return audio * gain
def processaudio(name):
    audio, fs = sf.read(name)
    audio = 0.5*(audio/np.max(np.abs(audio)))
    sf.write(name, audio, fs)
    with wave.open(name, "rb") as wav_file:
        raw_data = wav_file.readframes(wav_file.getnframes())
    with open(name+'.bin', "wb") as bin_file:
        bin_file.write(raw_data)
# processaudio('BassyTrainNoise.wav')
# processaudio('CherubScreams.wav')
# processaudio('MicScratch.wav')
# processaudio('Ring.wav')
# processaudio('TrainScreech1.wav')
# processaudio('TrainScreech2.wav')
# processaudio('WhiteNoise.wav')
# processaudio('Shake.wav')
processaudio('StringScratch.wav')