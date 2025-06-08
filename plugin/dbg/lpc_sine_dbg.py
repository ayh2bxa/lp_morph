import numpy as np
import soundfile as sf
from utils.lpc import LPC

fs = 44100
# f = 440
# t = np.array(range(5*fs))/fs
# x = 0.1*np.sin(2*np.pi*f*t)
x, _ = sf.read('breakloop.wav')
# x = 0.5*(x[:, 0]+x[:, 1])
print(x.dtype)
x = x.astype(np.float32)
print(x.dtype)
x = x.astype(np.float64)
print(x.dtype)
print(x.shape)
# sysBlocksize = 4096
# num_blocks = len(x)//sysBlocksize 
# frameLens = [2205]
# orders = [64]
# out = np.zeros(sysBlocksize*num_blocks)
# for framelen in frameLens:
#     for order in orders:
#         lpc = LPC(framelen, order)
#         lpc.prepare_to_play()
#         noise, _ = sf.read('audio/source/WhiteNoise.wav')
#         lpc.noise = noise
#         for ch in range(x.shape[1]):
#             x_ch = x[:, ch]
#             for i in range(num_blocks):
#                 xBlock = x_ch[i*sysBlocksize:(i+1)*sysBlocksize]
#                 lpc.apply_lpc(xBlock, out[i*sysBlocksize:(i+1)*sysBlocksize], sysBlocksize, 1.0, 1.0, ch, 0, previous_gain=1.0, current_gain=1.0)
# out_norm = out / np.max(1.1*np.abs(out))
# sf.write('out_norm_break.wav', out_norm, fs)
# sf.write('out_break.wav', out, fs)