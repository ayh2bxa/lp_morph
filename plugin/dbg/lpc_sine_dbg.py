import numpy as np
import soundfile as sf
from utils.lpc import LPC

fs = 44100
# f = 440
# t = np.array(range(5*fs))/fs
# x = 0.1*np.sin(2*np.pi*f*t)
x, _ = sf.read('breakloop.wav')
print(x.dtype)
x = x.astype(np.float32)
print(x.dtype)
x = x.astype(np.float64)
print(x.dtype)
sysBlocksize = 4096
num_blocks = len(x)//sysBlocksize 
frameLens = [2205]
orders = [64]
out = np.zeros(sysBlocksize*num_blocks)
for framelen in frameLens:
    for order in orders:
        lpc = LPC(framelen, order)
        lpc.prepare_to_play()
        noise, _ = sf.read('audio/source/WhiteNoise.wav')
        lpc.noise = noise
        for i in range(num_blocks):
            xBlock = x[i*sysBlocksize:(i+1)*sysBlocksize]
            lpc.apply_lpc(xBlock, out[i*sysBlocksize:(i+1)*sysBlocksize], sysBlocksize, 1.0, 1.0, 0, 0, previous_gain=1.0, current_gain=1.0)
out_norm = out / np.max(1.1*np.abs(out))
sf.write('out_norm_break.wav', out_norm, fs)
sf.write('out_break.wav', out, fs)
            # lpc.analyse(xBlock, sysBlocksize, 0)
    # def apply_lpc(self, input_data, output, num_samples, lpc_mix, ex_percentage, ch, 
    #               ex_start_pos, previous_gain=1.0, current_gain=1.0):