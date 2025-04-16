import math
import numpy as np
from typing import List, Optional
# from alphas_plot import analyse_alphas

class LPC:
    def __init__(self, num_channels: int):
        self.total_num_channels = num_channels
        self.FRAMELEN = 441
        self.prev_frame_len = self.FRAMELEN
        self.HOPSIZE = self.FRAMELEN // 2
        self.BUFLEN = 16384
        self.SAMPLERATE = 44100
        self.MAX_EXLEN = self.SAMPLERATE // 6
        self.EXLEN = self.MAX_EXLEN
        self.ORDER = 32
        self.ex_type = 0
        self.order_changed = False
        self.ex_type_changed = False
        self.ex_start_changed = False
        self.ex_start = 0.0
        self.start = False
        self.noise = None
        self.G = 1
        
        # Initialize pointers
        self.in_wt_ptrs = [0] * num_channels
        self.in_rd_ptrs = [0] * num_channels
        self.smp_cnts = [0] * num_channels
        self.out_wt_ptrs = [self.HOPSIZE] * num_channels
        self.out_rd_ptrs = [0] * num_channels
        self.ex_ptrs = [0] * num_channels
        self.hist_ptrs = [0] * num_channels
        self.ex_cnt_ptrs = [0] * num_channels
        
        # Initialize buffers
        self.phi = np.zeros(self.ORDER + 1)
        self.alphas = np.zeros(self.ORDER + 1)
        self.reflections = np.zeros(self.ORDER)
        
        ordered_buf_size = int(self.SAMPLERATE * 0.01)
        self.ordered_in_buf = np.zeros(ordered_buf_size)
        self.ordered_sc_buf = np.zeros(ordered_buf_size)
        self.window = np.zeros(ordered_buf_size)
        
        # Initialize channel-specific buffers
        self.in_buf = [np.zeros(self.BUFLEN) for _ in range(num_channels)]
        self.sc_buf = [np.zeros(self.BUFLEN) for _ in range(num_channels)]
        self.out_buf = [np.zeros(self.BUFLEN) for _ in range(num_channels)]
        self.out_hist = [np.zeros(self.ORDER) for _ in range(num_channels)]
        
        # Initialize window
        for i in range(self.FRAMELEN):
            self.window[i] = 0.5 * (1.0 - math.cos(2.0 * math.pi * i / (self.FRAMELEN - 1)))
        
        self.reset_a()
    
    def reset_a(self):
        self.alphas[0] = 1.0
        for i in range(1, self.ORDER + 1):
            self.alphas[i] = 0.0
    
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
    
    def autocorrelate(self, x: np.ndarray, frame_size: int, lag: int) -> float:
        res = 0.0
        for n in range(frame_size - lag):
            res += x[n] * x[n + lag]
        return res
    
    def prepare_to_play(self):
        self.in_wt_ptrs = [0] * self.total_num_channels
        self.in_rd_ptrs = [0] * self.total_num_channels
        self.smp_cnts = [0] * self.total_num_channels
        self.out_wt_ptrs = [self.HOPSIZE] * self.total_num_channels
        self.out_rd_ptrs = [0] * self.total_num_channels
        self.ex_ptrs = [0] * self.total_num_channels
        self.hist_ptrs = [0] * self.total_num_channels
        self.ex_cnt_ptrs = [0] * self.total_num_channels
        
        self.in_buf = [np.zeros(self.BUFLEN) for _ in range(self.total_num_channels)]
        self.out_buf = [np.zeros(self.BUFLEN) for _ in range(self.total_num_channels)]
        self.out_hist = [np.zeros(self.ORDER) for _ in range(self.total_num_channels)]
        self.sc_buf = [np.zeros(self.BUFLEN) for _ in range(self.total_num_channels)]
    
    def apply_lpc(self, input_signal: np.ndarray, output_signal: np.ndarray, num_samples: int, 
                 lpc_mix: float, ex_percentage: float, ch: int, ex_start_pos: float, 
                 sidechain: Optional[np.ndarray] = None, previous_gain: float = 1.0, 
                 current_gain: float = 1.0):
        
        in_wt_ptr = self.in_wt_ptrs[ch]
        in_rd_ptr = self.in_rd_ptrs[ch]
        smp_cnt = self.smp_cnts[ch]
        out_rd_ptr = self.out_rd_ptrs[ch]
        out_wt_ptr = (out_rd_ptr + self.HOPSIZE) % self.BUFLEN
        
        if self.HOPSIZE < self.prev_frame_len // 2:
            for i in range(self.prev_frame_len // 2 - self.HOPSIZE):
                idx = out_wt_ptr + i
                if idx >= self.BUFLEN:
                    idx -= self.BUFLEN
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
            self.in_buf[ch][in_wt_ptr] = input_signal[s]
            
            if sidechain is not None:
                self.sc_buf[ch][in_wt_ptr] = sidechain[s]
            
            in_wt_ptr += 1
            if in_wt_ptr >= self.BUFLEN:
                in_wt_ptr = 0
            
            out = self.out_buf[ch][out_rd_ptr]
            in_val = self.in_buf[ch][(in_rd_ptr + self.BUFLEN - self.FRAMELEN) % self.BUFLEN]
            
            in_rd_ptr += 1
            if in_rd_ptr >= self.BUFLEN:
                in_rd_ptr = 0
            
            gain_factor = previous_gain + slope * float(s)
            output_signal[s] = lpc_mix * gain_factor * out + (1 - lpc_mix) * in_val
            self.out_buf[ch][out_rd_ptr] = 0
            
            out_rd_ptr += 1
            if out_rd_ptr >= self.BUFLEN:
                out_rd_ptr = 0
            
            smp_cnt += 1
            
            if smp_cnt >= valid_hop_size:
                smp_cnt = 0
                
                if sidechain is not None:
                    for i in range(self.FRAMELEN):
                        in_buf_idx = (in_wt_ptr + i - self.FRAMELEN + self.BUFLEN) % self.BUFLEN
                        self.ordered_in_buf[i] = self.window[i] * self.in_buf[ch][in_buf_idx]
                        self.ordered_sc_buf[i] = self.sc_buf[ch][in_buf_idx]
                else:
                    for i in range(self.FRAMELEN):
                        in_buf_idx = (in_wt_ptr + i - self.FRAMELEN + self.BUFLEN) % self.BUFLEN
                        self.ordered_in_buf[i] = self.window[i] * self.in_buf[ch][in_buf_idx]
                
                for lag in range(self.ORDER + 1):
                    self.phi[lag] = self.autocorrelate(self.ordered_in_buf, self.FRAMELEN, lag)
                
                if self.phi[0] != 0:
                    self.levinson_durbin()
                    
                    self.G = self.phi[0]
                    for k in range(self.ORDER):
                        self.G -= self.alphas[k + 1] * self.phi[k + 1]
                    
                    # self.G = math.sqrt(G / self.FRAMELEN / math.sqrt(float(self.ORDER)))
                    self.G = math.sqrt(self.G)
                    if sidechain is None:
                        for n in range(self.FRAMELEN):
                            ex = self.noise[ex_ptr]
                            ex_cnt_ptr += 1
                            ex_ptr += 1
                            
                            if ex_cnt_ptr >= int(ex_percentage * self.EXLEN):
                                ex_cnt_ptr = 0
                                ex_ptr = ex_start
                            
                            if ex_ptr >= self.EXLEN:
                                ex_ptr = 0
                            
                            out_n = G * ex
                            for k in range(self.ORDER):
                                idx = (hist_ptr + k) % self.ORDER
                                out_n -= self.alphas[k + 1] * self.out_hist[ch][idx]
                            
                            hist_ptr -= 1
                            if hist_ptr < 0:
                                hist_ptr += self.ORDER
                            
                            self.out_hist[ch][hist_ptr] = out_n
                            wt_idx = (out_wt_ptr + n) % self.BUFLEN
                            self.out_buf[ch][wt_idx] += out_n
                    else:
                        for n in range(self.FRAMELEN):
                            ex = self.ordered_sc_buf[n]
                            ex_cnt_ptr += 1
                            ex_ptr += 1
                            
                            if ex_cnt_ptr >= int(ex_percentage * self.EXLEN):
                                ex_cnt_ptr = 0
                                ex_ptr = ex_start
                            
                            if ex_ptr >= self.EXLEN:
                                ex_ptr = 0
                            
                            out_n = self.G * ex
                            for k in range(self.ORDER):
                                idx = (hist_ptr + k) % self.ORDER
                                out_n -= self.alphas[k + 1] * self.out_hist[ch][idx]
                            
                            hist_ptr -= 1
                            if hist_ptr < 0:
                                hist_ptr += self.ORDER
                            
                            self.out_hist[ch][hist_ptr] = out_n
                            wt_idx = (out_wt_ptr + n) % self.BUFLEN
                            self.out_buf[ch][wt_idx] += out_n
                    
                    self.out_hist[ch].fill(0)
                    self.hist_ptrs[ch] = 0
                
                out_wt_ptr += self.HOPSIZE
                if out_wt_ptr >= self.BUFLEN:
                    out_wt_ptr -= self.BUFLEN
        
        # Update pointers
        self.in_rd_ptrs[ch] = in_rd_ptr
        self.in_wt_ptrs[ch] = in_wt_ptr
        self.smp_cnts[ch] = smp_cnt
        self.out_wt_ptrs[ch] = out_wt_ptr
        self.out_rd_ptrs[ch] = out_rd_ptr
        self.ex_ptrs[ch] = ex_ptr
        self.hist_ptrs[ch] = hist_ptr
    
    def set_exlen(self, val: int):
        self.EXLEN = val
    
    def get_exlen(self) -> int:
        return self.EXLEN
    
    def get_max_exlen(self) -> int:
        return self.MAX_EXLEN