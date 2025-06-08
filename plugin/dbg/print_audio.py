import soundfile as sf 
import numpy as np

x, fs = sf.read('vst_break.wav')
x = 0.5*(x[:, 0]+x[:, 1])
x = x[:(int)(0.5*fs)]
for i, v in enumerate(x):
    print(i, v)