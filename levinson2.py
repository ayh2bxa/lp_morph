import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
ORDER = 32
def levinson_durbin():
    a = [0]*(ORDER+1)
    a[0] = 1
    a_prev = [0]*(ORDER+1)
    a_prev[0] = 1
    phi = [
        0.0367769264, 0.0367712453, 0.0367598236, 0.0367461145, 0.0367287695,
        0.0367091261, 0.0366882868, 0.0366659723, 0.0366411805, 0.036614053,
        0.0365829505, 0.0365466885, 0.036506135, 0.0364611633, 0.0364119709,
        0.0363602452, 0.0363060571, 0.036248751, 0.036189016, 0.0361266211,
        0.0360603295, 0.0359897576, 0.035916239, 0.0358397812, 0.0357596241,
        0.0356764719, 0.0355904065, 0.0355015211, 0.0354106538, 0.0353173688,
        0.0352204926, 0.0351199359, 0.0350164063
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
    # print("reflection coefficients: ")
    # for idx, value in enumerate(reflections):
    #     print(idx, value)
    print("alpha coefficients: ")
    for idx, value in enumerate(a):
        print(idx, value)
    return reflections

ks = np.array(levinson_durbin())
# ks /= (10*np.max(np.abs(ks)))
# print(ks)
alphas = [0]*ORDER
prev_alphas = [0]*ORDER
for i in range(ORDER):
    alphas[i] = ks[i]
    if i > 0:
        for j in range(1, i):
            alphas[j] = alphas[j]-ks[i]*prev_alphas[i-j]
    for i in range(ORDER):
        prev_alphas[i] = alphas[i]
# print("k to a: "+str(alphas))
# Assuming FIR numerator (all-pass):
num = [1]  # or set your own if you have zeros
alphas = [1]+alphas
# Compute zeros and poles
z, p, k = signal.tf2zpk(num, alphas)

# Plot
plt.figure(figsize=(6,6))
plt.axhline(0, color='black')
plt.axvline(0, color='black')

# Unit circle for reference
circle = plt.Circle((0,0), radius=1, fill=False, color='gray', linestyle='--')
plt.gca().add_artist(circle)

# Plot poles and zeros
plt.plot(np.real(z), np.imag(z), 'go', label='Zeros')  # green circles
plt.plot(np.real(p), np.imag(p), 'rx', label='Poles')  # red crosses
plt.title("Pole-Zero Plot")
plt.xlabel("Real")
plt.ylabel("Imaginary")
plt.legend()
plt.grid()
plt.axis("equal")
plt.show()