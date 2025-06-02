import numpy as np
import math
from .alphas_plot import analyse_alphas, plot_pole_zero
import matplotlib.pyplot as plt

class LPC:
    def __init__(self, framelen=441, order=32, buflen=16384, num_channels=1, sr=44100):
        self.total_num_channels = num_channels
        
        # Initialize per-channel arrays
        self.in_wt_ptrs = [0] * num_channels
        self.in_rd_ptrs = [0] * num_channels
        self.smp_cnts = [0] * num_channels
        self.out_wt_ptrs = [0] * num_channels
        self.out_rd_ptrs = [0] * num_channels
        self.ex_ptrs = [0] * num_channels
        self.hist_ptrs = [0] * num_channels
        self.ex_cnt_ptrs = [0] * num_channels
        
        # Initialize default parameters
        self.FRAMELEN = framelen
        self.nfft = int(2**np.ceil(np.log2(self.FRAMELEN)))
        self.prev_frame_len = self.FRAMELEN
        self.HOPSIZE = self.FRAMELEN // 2
        self.BUFLEN = buflen
        self.SAMPLERATE = sr
        self.MAX_EXLEN = self.SAMPLERATE // 6
        self.EXLEN = self.MAX_EXLEN
        self.ORDER = order
        self.ex_type = 0
        self.order_changed = False
        self.ex_type_changed = False
        self.ex_start_changed = False
        self.ex_start = 0.0
        self.start = False
        
        # Initialize per-channel settings
        for ch in range(num_channels):
            self.in_wt_ptrs[ch] = 0
            self.smp_cnts[ch] = 0
            self.out_wt_ptrs[ch] = self.HOPSIZE
            self.out_rd_ptrs[ch] = 0
            self.ex_ptrs[ch] = 0
            self.hist_ptrs[ch] = 0
            self.ex_cnt_ptrs[ch] = 0
            self.in_rd_ptrs[ch] = 0
        
        # Initialize LPC coefficients and working arrays
        self.phi = np.zeros(self.ORDER + 1)
        self.alphas = np.zeros(self.ORDER + 1)
        self.reflections = np.zeros(self.ORDER)
        self.ordered_in_buf = np.zeros(int(self.SAMPLERATE * 0.01))
        self.window = np.zeros(int(self.SAMPLERATE * 0.01))
        
        # Initialize multi-channel buffers
        self.in_buf = []
        self.out_buf = []
        self.out_hist = []
        
        for ch in range(num_channels):
            self.in_buf.append(np.zeros(self.BUFLEN))
            self.out_buf.append(np.zeros(self.BUFLEN))
            self.out_hist.append(np.zeros(self.ORDER))
        
        # Initialize Hanning window
        for i in range(self.FRAMELEN):
            self.window[i] = 0.5 * (1.0 - math.cos(2.0 * math.pi * i / (self.FRAMELEN - 1)))
        
        # Initialize LPC coefficients
        self.reset_a()
        
        # Noise excitation source (to be set externally)
        self.noise = None
    
    def set_exlen(self, val):
        self.EXLEN = val
    
    def get_exlen(self):
        return self.EXLEN
    
    def get_max_exlen(self):
        return self.MAX_EXLEN
    
    def reset_a(self):
        self.alphas[0] = 1.0
        for i in range(1, self.ORDER + 1):
            self.alphas[i] = 0.0
    
    def autocorrelate(self, x, frame_size, lag):
        res = 0.0
        for n in range(frame_size - lag):
            res += x[n] * x[n + lag]
        return res
    
    def levinson_durbin(self):
        self.reset_a()
        E = self.phi[0]
        
        for k in range(self.ORDER):
            lbda = 0.0
            for j in range(k + 1):
                lbda += self.alphas[j] * self.phi[k + 1 - j]
            lbda = -lbda / E
            self.reflections[k] = lbda
            
            half = (k + 1) // 2
            for n in range(half + 1):
                tmp = self.alphas[k + 1 - n] + lbda * self.alphas[n]
                self.alphas[n] += lbda * self.alphas[k + 1 - n]
                self.alphas[k + 1 - n] = tmp
            
            E *= (1.0 - lbda * lbda)
    
    def prepare_to_play(self):
        # Resize arrays for new channel count
        self.in_wt_ptrs = [0] * self.total_num_channels
        self.in_rd_ptrs = [0] * self.total_num_channels
        self.smp_cnts = [0] * self.total_num_channels
        self.out_wt_ptrs = [0] * self.total_num_channels
        self.out_rd_ptrs = [0] * self.total_num_channels
        self.ex_ptrs = [0] * self.total_num_channels
        self.hist_ptrs = [0] * self.total_num_channels
        self.ex_cnt_ptrs = [0] * self.total_num_channels
        
        for ch in range(self.total_num_channels):
            self.in_wt_ptrs[ch] = 0
            self.smp_cnts[ch] = 0
            self.out_wt_ptrs[ch] = self.HOPSIZE
            self.out_rd_ptrs[ch] = 0
            self.ex_ptrs[ch] = 0
            self.hist_ptrs[ch] = 0
            self.ex_cnt_ptrs[ch] = 0
            self.in_rd_ptrs[ch] = 0
        
        # Resize buffers
        self.in_buf = []
        self.out_buf = []
        self.out_hist = []
        
        for ch in range(self.total_num_channels):
            self.in_buf.append(np.zeros(self.BUFLEN))
            self.out_buf.append(np.zeros(self.BUFLEN))
            self.out_hist.append(np.zeros(self.ORDER))

    def analyse(self, input_data, num_samples, ch):
        # Get per-channel state
        in_wt_ptr = self.in_wt_ptrs[ch]
        in_rd_ptr = self.in_rd_ptrs[ch]
        smp_cnt = self.smp_cnts[ch]
        out_rd_ptr = self.out_rd_ptrs[ch]
        out_wt_ptr = (out_rd_ptr + self.HOPSIZE) % self.BUFLEN
        
        if self.HOPSIZE < self.prev_frame_len // 2:
            for i in range(self.prev_frame_len // 2 - self.HOPSIZE):
                idx = (out_wt_ptr + i) % self.BUFLEN
                self.out_buf[ch][idx] = 0
        
        valid_hop_size = min(self.HOPSIZE, self.prev_frame_len // 2)
        
        for s in range(num_samples):
            # Store input sample
            self.in_buf[ch][in_wt_ptr] = input_data[s]
            
            in_wt_ptr = (in_wt_ptr + 1) % self.BUFLEN
            
            # Get delayed input sample
            in_rd_ptr = (in_rd_ptr + 1) % self.BUFLEN
            
            smp_cnt += 1
            
            # Process frame when hop size is reached
            if smp_cnt >= valid_hop_size:
                smp_cnt = 0
                
                # Fill ordered buffers with windowed input
                for i in range(self.FRAMELEN):
                    in_buf_idx = (in_wt_ptr + i - self.FRAMELEN + self.BUFLEN) % self.BUFLEN
                    self.ordered_in_buf[i] = self.window[i] * self.in_buf[ch][in_buf_idx]
                
                # Compute autocorrelation
                for lag in range(self.ORDER + 1):
                    self.phi[lag] = self.autocorrelate(self.ordered_in_buf, self.FRAMELEN, lag)
                
                if self.phi[0] != 0:
                    # Compute LPC coefficients
                    self.levinson_durbin()
                    
                    # Compute gain
                    G = self.phi[0]
                    for k in range(self.ORDER):
                        G -= self.alphas[k + 1] * self.phi[k + 1]
                    G = math.sqrt(G)
                    fs = 44100
                    f = 440
                    t = np.array(range(fs))/fs
                    x = np.sin(2*np.pi*f*t[:self.FRAMELEN])
                    x *= self.window
                    analyse_alphas(G, self.alphas, x, self.nfft, self.SAMPLERATE)
        
        # Store per-channel state
        self.in_rd_ptrs[ch] = in_rd_ptr
        self.in_wt_ptrs[ch] = in_wt_ptr
        self.smp_cnts[ch] = smp_cnt
    
    def apply_lpc(self, input_data, output, num_samples, lpc_mix, ex_percentage, ch, 
                  ex_start_pos, previous_gain=1.0, current_gain=1.0):
        if self.noise is None:
            return
        
        # Get per-channel state
        in_wt_ptr = self.in_wt_ptrs[ch]
        in_rd_ptr = self.in_rd_ptrs[ch]
        smp_cnt = self.smp_cnts[ch]
        out_rd_ptr = self.out_rd_ptrs[ch]
        out_wt_ptr = (out_rd_ptr + self.HOPSIZE) % self.BUFLEN
        
        if self.HOPSIZE < self.prev_frame_len // 2:
            for i in range(self.prev_frame_len // 2 - self.HOPSIZE):
                idx = (out_wt_ptr + i) % self.BUFLEN
                self.out_buf[ch][idx] = 0
        
        ex_start = int(ex_start_pos * self.EXLEN)
        
        if self.ex_type_changed:
            self.ex_ptrs[ch] = ex_start
            self.ex_cnt_ptrs[ch] = 0
            self.hist_ptrs[ch] = 0
        
        if self.order_changed:
            self.out_hist[ch].fill(0)
            self.hist_ptrs[ch] = 0
        
        ex_ptr = self.ex_ptrs[ch]
        ex_cnt_ptr = self.ex_cnt_ptrs[ch]
        hist_ptr = self.hist_ptrs[ch]
        
        slope = (current_gain - previous_gain) / num_samples
        valid_hop_size = min(self.HOPSIZE, self.prev_frame_len // 2)
        
        for s in range(num_samples):
            # Store input sample
            self.in_buf[ch][in_wt_ptr] = input_data[s]
            
            in_wt_ptr = (in_wt_ptr + 1) % self.BUFLEN
            
            # Get output sample
            out = self.out_buf[ch][out_rd_ptr]
            
            # Get delayed input sample
            in_sample = self.in_buf[ch][(in_rd_ptr + self.BUFLEN - self.FRAMELEN) % self.BUFLEN]
            in_rd_ptr = (in_rd_ptr + 1) % self.BUFLEN
            
            # Apply gain ramp
            gain_factor = previous_gain + slope * s
            output[s] = lpc_mix * gain_factor * out + (1 - lpc_mix) * in_sample
            
            # Clear output buffer
            self.out_buf[ch][out_rd_ptr] = 0
            out_rd_ptr = (out_rd_ptr + 1) % self.BUFLEN
            
            smp_cnt += 1
            
            # Process frame when hop size is reached
            if smp_cnt >= valid_hop_size:
                smp_cnt = 0
                
                # Fill ordered buffers with windowed input
                for i in range(self.FRAMELEN):
                    in_buf_idx = (in_wt_ptr + i - self.FRAMELEN + self.BUFLEN) % self.BUFLEN
                    self.ordered_in_buf[i] = self.window[i] * self.in_buf[ch][in_buf_idx]
                
                # Compute autocorrelation
                for lag in range(self.ORDER + 1):
                    self.phi[lag] = self.autocorrelate(self.ordered_in_buf, self.FRAMELEN, lag)
                
                if self.phi[0] != 0:
                    # Compute LPC coefficients
                    self.levinson_durbin()
                    
                    # Compute gain
                    G = self.phi[0]
                    for k in range(self.ORDER):
                        G -= self.alphas[k + 1] * self.phi[k + 1]
                    G = math.sqrt(G)
                    
                    # Generate output frame
                    for n in range(self.FRAMELEN):
                        ex = self.noise[ex_ptr]
                        ex_cnt_ptr += 1
                        ex_ptr += 1
                        
                        if ex_cnt_ptr >= int(ex_percentage * self.EXLEN):
                            ex_cnt_ptr = 0
                            ex_ptr = ex_start
                        
                        if ex_ptr >= self.EXLEN:
                            ex_ptr = 0
                        
                        # Apply LPC synthesis filter
                        out_n = G * ex
                        for k in range(self.ORDER):
                            idx = (hist_ptr + k) % self.ORDER
                            out_n -= self.alphas[k + 1] * self.out_hist[ch][idx]
                        
                        hist_ptr = (hist_ptr - 1) % self.ORDER
                        self.out_hist[ch][hist_ptr] = out_n
                        
                        wt_idx = (out_wt_ptr + n) % self.BUFLEN
                        self.out_buf[ch][wt_idx] += out_n
                    
                    # Reset history (as in C++ code)
                    self.out_hist[ch].fill(0)
                    self.hist_ptrs[ch] = 0
                
                out_wt_ptr = (out_wt_ptr + self.HOPSIZE) % self.BUFLEN
        
        # Store per-channel state
        self.in_rd_ptrs[ch] = in_rd_ptr
        self.in_wt_ptrs[ch] = in_wt_ptr
        self.smp_cnts[ch] = smp_cnt
        self.out_wt_ptrs[ch] = out_wt_ptr
        self.out_rd_ptrs[ch] = out_rd_ptr
        self.ex_ptrs[ch] = ex_ptr
        self.hist_ptrs[ch] = hist_ptr
