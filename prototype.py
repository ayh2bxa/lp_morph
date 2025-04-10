import wave
import numpy as np
import math
import random
from scipy.io import wavfile

FRAMELEN = 882
HOPSIZE = int(FRAMELEN/2)
P = 25

# http://www.emptyloop.com/technotes/A%20tutorial%20on%20linear%20prediction%20and%20Levinson-Durbin.pdf
def levinson_durbin(phi, order):
    a = np.zeros(order+1)
    a[0] = 1
    E = phi[0]
    for k in range(order):
        lbda = 0
        for j in range(k+1):
            lbda -= a[j]*phi[k+1-j]
        lbda /= E
        for n in range(1+int((k+1)//2)):
            temp = a[k+1-n]+lbda*a[n]
            a[n] += lbda*a[k+1-n]
            a[k+1-n] = temp
        E *= (1-lbda*lbda)
    return a

# # Open the WAV file
# with wave.open('input.wav', 'rb') as wav_file:
#     # Extract raw audio from the WAV file
#     n_channels = wav_file.getnchannels()
#     sample_width = wav_file.getsampwidth()
#     framerate = wav_file.getframerate()
#     n_frames = wav_file.getnframes()
#     audio_data = wav_file.readframes(n_frames)

# # Convert raw audio data to numpy array
# if sample_width == 1:  # 8-bit audio
#     print("8")
#     audio_array = np.frombuffer(audio_data, dtype=np.uint8) - 128
#     audio_array = audio_array.astype(np.float32) / 128.0
# elif sample_width == 2:  # 16-bit audio
#     print("16")
#     audio_array = np.frombuffer(audio_data, dtype=np.int16)
#     audio_array = audio_array.astype(np.float32) / 32768.0
# elif sample_width == 4:  # 32-bit audio
#     print("32")
#     audio_array = np.frombuffer(audio_data, dtype=np.int32)
#     audio_array = audio_array.astype(np.float32) / 2147483648.0

# K = int(1+(n_frames-FRAMELEN)//HOPSIZE)

# window = np.zeros(FRAMELEN)
# win_sq_sum = 0
# for i in range(FRAMELEN):
#     window[i] = 0.5*(1-math.cos(2*math.pi*i/(FRAMELEN-1)))
#     win_sq_sum += window[i]*window[i]

# def autocorrelate(x, lag):
#     res = 0
#     # x -= np.mean(x)
#     for n in range(0, len(x)-lag):
#         res += x[n]*x[n+lag]
#     return res

# white_noise_l = np.random.normal(0, 1, n_frames)
# white_noise_l /= (np.amax(white_noise_l)*1.1)
# # white_noise_l = np.zeros(n_frames)
# # for i in range(len(white_noise_l)):
# #     if i%100 == 0:
# #         white_noise_l[i] = 1
# white_noise_r = np.random.normal(0, 1, n_frames)
# white_noise = [white_noise_l, white_noise_r]
# hist_ptr = [0, 0]
# residual = np.zeros(FRAMELEN)
# def process(x, ch):
#     exPtr = 0
#     x_windowed = np.zeros(FRAMELEN)
#     output = np.zeros(n_frames)
#     output_frame = np.zeros(FRAMELEN)
#     out_prev = np.zeros(P)
#     for frame in range(K):
#         start = frame*HOPSIZE
#         end = min(len(x), start+FRAMELEN)
#         x_windowed = window*x[start:end]
#         phi = np.zeros(1+P)
#         for p in range(P+1):
#             phi[p] = autocorrelate(x_windowed, p)
#         if phi[0] != 0:
#             # phi /= np.amax(np.abs(phi))
#             a = levinson_durbin(phi, P)
#             for n in range(FRAMELEN):
#                 x_hat_n = 0
#                 for k in range(1, P+1):
#                     if n-k >= 0: 
#                         x_hat_n += a[k]*x_windowed[n-k]
#                 residual[n] = x_windowed[n]-x_hat_n
#             G = math.sqrt(np.sum((x_windowed-residual)*(x_windowed-residual)))/(FRAMELEN-P)
#             # G = phi[0]
#             # for k in range(1, P+1):
#             #     G -= a[k]*phi[k]
#             # G = math.sqrt(G)
#             # G = 1
#             # if frame == 0:
#                 # print("FRAME "+str(frame)+" prewindow: \n"+str(x[start:end]))
#                 # print("FRAME "+str(frame)+": \n"+str(x_windowed))
#                 # print("phi: "+str(phi))
#                 # print("a: "+str(a))
#                 # print("G: "+str(G))
#             # excitation = white_noise[ch][start:end]
#             excitation = np.zeros(FRAMELEN)
#             for i in range(FRAMELEN):
#                 excitation[i] = white_noise[ch][exPtr]
#                 exPtr += 1
#                 if exPtr >= n_frames: 
#                     exPtr = 0
#             for n in range(FRAMELEN):
#                 output_frame[n] = G*excitation[n]
#                 for k in range(P):
#                     idx = hist_ptr[ch]-k-1
#                     if idx < 0:
#                         idx += P
#                     output_frame[n] -= a[k+1]*out_prev[idx] 
#                 out_prev[hist_ptr[ch]] = output_frame[n] 
#                 hist_ptr[ch] += 1 
#                 if hist_ptr[ch] >= P: 
#                     hist_ptr[ch] = 0
#             output[start:end] += output_frame
#     for i in range(len(output)):
#         if np.abs(output[i]) > 1:
#             print(output[i])
#     return output

# def write_wav(file_path, audio_data, n_channels, sampwidth, framerate):
#     with wave.open(file_path, 'wb') as wav_file:
#         wav_file.setnchannels(n_channels)
#         wav_file.setsampwidth(sampwidth)
#         wav_file.setframerate(framerate)
#         wav_file.writeframes(audio_data.tobytes())

# def lpc(x):
#     n_channels = 2
#     x = x.reshape((-1, n_channels))
#     xleft = x[:, 0]
#     xright = x[:, 1]
#     yleft = process(xleft, 0)
#     # yright = process(xright, 1)
#     min_length = len(yleft)
#     yleft = yleft[:min_length]
#     # yright = yright[:min_length]
#     interleaved = np.empty((yleft.size + yleft.size,), dtype=np.int16)
#     interleaved[0::2] = np.int16(yleft*32767.0)
#     interleaved[1::2] = np.int16(yleft*32767.0)
#     # WAV file parameters
#     sampwidth = 2  # 2 bytes for int16
#     framerate = 44100  # Standard sample rate for audio

#     # Output path
#     output_path = 'output.wav'

#     # Write the interleaved audio data to a WAV file
#     write_wav(output_path, interleaved, n_channels, sampwidth, framerate)

#     print(f"Interleaved audio written to {output_path}")

# if __name__ == '__main__':
#     lpc(audio_array)
