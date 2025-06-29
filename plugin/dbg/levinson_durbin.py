import numpy as np

class LPC:
    def __init__(self, order=100):
        self.ORDER = order
        import pandas as pd

        # 1) Read the CSV
        df = pd.read_csv('audio_log.csv')

        # 2) Build the list of phi-columns
        phi_cols = [f'phi_{i}' for i in range(order+1)]

        # 3) Extract row 5 (1-based) → index 4 (0-based)
        self.phi = df.loc[4, phi_cols].to_numpy(dtype=float)
        print(self.phi)
        self.alphas = np.zeros(order + 1)
        self.reset_a()

    def reset_a(self):
        """Reset alpha coefficients"""
        self.alphas[0] = 1.0
        for i in range(1, self.ORDER + 1):
            self.alphas[i] = 0.0

    def autocorrelate(self, x, frame_size, lag):
        """
        Compute autocorrelation at given lag
        
        Args:
            x: input signal array
            frame_size: length of frame to analyze
            lag: autocorrelation lag
            
        Returns:
            autocorrelation value at specified lag
        """
        res = 0.0
        for n in range(frame_size - lag):
            res += x[n] * x[n + lag]
        return res

    def levinson_durbin(self):
        """
        Levinson-Durbin algorithm for LPC coefficient computation
        
        Returns:
            prediction error (E)
        """
        # Reset alpha coefficients
        self.reset_a()

        # Initialize prediction error with autocorrelation at lag 0
        E = self.phi[0]

        # Levinson-Durbin recursion
        for k in range(self.ORDER):
            # Compute reflection coefficient (lambda)
            lbda = 0.0
            for j in range(k + 1):
                lbda += self.alphas[j] * self.phi[k + 1 - j]
            lbda = -lbda / E

            # Update coefficients using step-down recursion
            half = (k + 1) // 2
            for n in range(half + 1):
                tmp = self.alphas[k + 1 - n] + lbda * self.alphas[n]
                self.alphas[n] += lbda * self.alphas[k + 1 - n]
                self.alphas[k + 1 - n] = tmp

            # Update prediction error
            E *= (1.0 - lbda * lbda)

        return E

if __name__ == "__main__":
    lpc = LPC()
    print(lpc.levinson_durbin())