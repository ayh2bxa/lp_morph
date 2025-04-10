import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import butter, freqz, lfilter
import soundfile as sf

# Sampling rate and desired cutoff
fs = 44100
cutoff = 2000

# Design a 2nd-order lowpass Butterworth filter
# We want 5 denominator coefficients, so we use order 2 (which gives 3 'a' coefficients).
# To force 5, we design a higher-order and truncate.
order = 32
b_butter, a_butter = butter(N=4, Wn=cutoff/(fs/2), btype='low', analog=False)
# Truncate or pad the denominator to 5 elements
a = np.zeros(order+1)
a[:min(order+1, len(a_butter))] = a_butter[:order+1]
b = [1.0]  # LPC-style all-pole IIR
gain_dc = sum(b) / sum(a)
b = [x / gain_dc for x in b]
input, rate = sf.read("../resources/excitations/WhiteNoise.wav")
input = input[:, 0]
# order = len(a)-1
out_hist = np.array([0.0]*order)
histPtr = 0
output = np.zeros(len(input))
for n in range(len(input)):
    out_n = b[0]*input[n]
    for k in range(order):
        idx = (histPtr+k)%order
        out_n -= (a[k+1]*out_hist[idx])
    histPtr = (histPtr-1)%order
    out_hist[histPtr] = out_n
    # if n < 10:
    #     print(n, out_hist)
    output[n] = out_n
sf.write('ringbuf_out32.wav', output, rate)

# import numpy as np
# import soundfile as sf
# order = len(a) - 1

# # Ring buffer implementation of IIR filtering
# class IIRRingBuffer:
#     def __init__(self, b, a):
#         self.b = np.array(b)
#         self.a = np.array(a)
#         self.order_b = len(b)
#         self.order_a = len(a)
#         self.x_buf = np.zeros(self.order_b)
#         self.y_buf = np.zeros(self.order_a - 1)
#         self.x_idx = 0
#         self.y_idx = 0
#         print(self.b, self.a, self.order_b, self.order_a)

#     def process_sample(self, x, idx):
#         self.x_buf[self.x_idx] = x

#         # Compute output: y[n] = sum(b[k] * x[n - k]) - sum(a[k] * y[n - k])
#         y = 0.0
#         for i in range(self.order_b):
#             xi = self.x_buf[(self.x_idx - i) % self.order_b]
#             y += self.b[i] * xi
#         for i in range(1, self.order_a):
#             yi = self.y_buf[(self.y_idx - i) % (self.order_a - 1)]
#             y -= self.a[i] * yi

#         self.y_buf[self.y_idx] = y
#         if idx < 10:
#             print(idx, self.y_buf)
#         # Move buffer indices forward
#         self.x_idx = (self.x_idx + 1) % self.order_b
#         self.y_idx = (self.y_idx + 1) % (self.order_a - 1)

#         return y

#     def process_block(self, x_block):
#         output = np.zeros(len(x_block))
#         for i in range(len(output)):
#             output[i] = self.process_sample(x_block[i], i)
#         return output
#         # return np.array([self.process_sample(x) for x in x_block])


# # Try processing an audio file
# try:
#     audio, rate = sf.read("../resources/excitations/WhiteNoise.wav")
#     if audio.ndim > 1:
#         audio = audio[:, 0]  # Use first channel if stereo

#     filter_instance = IIRRingBuffer(b, a)
#     filtered_audio = filter_instance.process_block(audio)

#     sf.write("ringbuf_out.wav", filtered_audio, rate)
#     result = "Filtering complete. Output written to 'filtered_ringbuffer_output.wav'."
# except FileNotFoundError:
#     result = "Audio file 'input.wav' not found."
# except Exception as e:
#     result = str(e)

# result
