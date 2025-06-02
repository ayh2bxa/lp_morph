import numpy as np
import soundfile as sf
from utils.lpc import LPC

fs = 44100
f = 440
t = np.array(range(fs))/fs
x = np.sin(2*np.pi*f*t)
sysBlocksize = 512
num_blocks = len(x)//sysBlocksize 
frameLens = [441]
orders = [2]
outBlock = np.zeros(sysBlocksize)
for framelen in frameLens:
    for order in orders:
        lpc = LPC(framelen, order)
        lpc.prepare_to_play()
        for i in range(num_blocks):
            xBlock = x[i*sysBlocksize:(i+1)*sysBlocksize]
            lpc.analyse(xBlock, sysBlocksize, 0)
            break
    # def apply_lpc(self, input_data, output, num_samples, lpc_mix, ex_percentage, ch, 
    #               ex_start_pos, previous_gain=1.0, current_gain=1.0):