import soundfile as sf 
import numpy as np
import matplotlib.pyplot as plt

audio, fs = sf.read('rt.wav')
for i in audio:
    if np.isnan(i):
        print('nan')
    else:
        print(i)