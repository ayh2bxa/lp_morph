import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
import soundfile as sf
from scipy.signal import lfilter
ORDER = 32
def levinson_durbin():
    a = [0]*(ORDER+1)
    a[0] = 1
    a_prev = [0]*(ORDER+1)
    a_prev[0] = 1
    # phi = [
    #     0.0367769264, 0.0367712453, 0.0367598236, 0.0367461145, 0.0367287695,
    #     0.0367091261, 0.0366882868, 0.0366659723, 0.0366411805, 0.036614053,
    #     0.0365829505, 0.0365466885, 0.036506135, 0.0364611633, 0.0364119709,
    #     0.0363602452, 0.0363060571, 0.036248751, 0.036189016, 0.0361266211,
    #     0.0360603295, 0.0359897576, 0.035916239, 0.0358397812, 0.0357596241,
    #     0.0356764719, 0.0355904065, 0.0355015211, 0.0354106538, 0.0353173688,
    #     0.0352204926, 0.0351199359, 0.0350164063
    # ]
    
    phi = [
        0.0011650967644527555,
        0.0011243076578477997,
        0.0010183736016510155,
        0.000980842337886674,
        0.0009079763990920253,
        0.0008495362665627827,
        0.0008949163939777697,
        0.0007906173902333849,
        0.0007305769888115067,
        0.0006528870110322423,
        0.0005926066666382289,
        0.0005230877063955064,
        0.0004712608939163837,
        0.0004317071728429491,
        0.0003753278812374771,
        0.0003267771126703321,
        0.0002971448763450384,
        0.0002391124700087236,
        0.0001886364288442284,
        0.0001490962861505826,
        0.00010788938742510259,
        8.137553121819049e-05,
        6.321871960133731e-05,
        4.658054978374911e-05,
        3.474539502960279e-05,
        2.3869690046633524e-05,
        1.7648430336684734e-05,
        1.2455921366106452e-05,
        9.162902167414978e-06,
        6.916217623507664e-06,
        5.225410923638218e-06,
        3.750263107387992e-06,
        2.5076934922300722e-06
    ]

    E = phi[0]
    reflections = [0]*(ORDER)
    for k in range(ORDER):
        lbda = 0.0
        for j in range(k + 1):
            lbda -= a[j] * phi[k + 1 - j]
        lbda /= E
        reflections[k] = lbda
        for n in range(1 + (k + 1) // 2):
            tmp = a[k + 1 - n] + lbda * a[n]
            a[n] = a[n] + lbda * a[k + 1 - n]
            a[k + 1 - n] = tmp
        E *= (1 - lbda * lbda)
    print("reflection coefficients: ")
    for idx, value in enumerate(reflections):
        print(idx, value)
    print("alpha coefficients: ")
    for idx, value in enumerate(a):
        print(idx, value)
    return reflections

levinson_durbin()

# ks = np.array(levinson_durbin())
# ks /= (10*np.max(np.abs(ks)))
# alphas = [0]*ORDER
# prev_alphas = [0]*ORDER
# for i in range(ORDER):
#     alphas[i] = ks[i]
#     if i > 0:
#         for j in range(1, i):
#             alphas[j] = alphas[j]-ks[i]*prev_alphas[i-j]
#     for i in range(ORDER):
#         prev_alphas[i] = alphas[i]
# # print("k to a: "+str(alphas))
# # Assuming FIR numerator (all-pass):
# # for i in range(ORDER):
# #     alphas[i] *= -1
# num = [1]  # or set your own if you have zeros
# alphas = [1]+alphas
# alphas = np.array(alphas)
# # Compute zeros and poles
# z, p, k = signal.tf2zpk(num, alphas)


# # Plot
# plt.figure(figsize=(6,6))
# plt.axhline(0, color='black')
# plt.axvline(0, color='black')

# # Unit circle for reference
# circle = plt.Circle((0,0), radius=1, fill=False, color='gray', linestyle='--')
# plt.gca().add_artist(circle)

# # Plot poles and zeros
# plt.plot(np.real(z), np.imag(z), 'go', label='Zeros')  # green circles
# plt.plot(np.real(p), np.imag(p), 'rx', label='Poles')  # red crosses
# plt.title("Pole-Zero Plot")
# plt.xlabel("Real")
# plt.ylabel("Imaginary")
# plt.legend()
# plt.grid()
# plt.axis("equal")
# plt.show()

# noise, fs = sf.read('resources/excitations/WhiteNoise.wav')
# noise = 0.5*(noise[:, 0]+noise[:, 1])
# y = lfilter([1.0], alphas, noise)
# sf.write('output.wav', y, fs)