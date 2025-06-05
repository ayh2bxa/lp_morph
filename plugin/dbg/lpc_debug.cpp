#include "lpc_debug.h"
#include <algorithm>
#include <cmath>

LPCDebug::LPCDebug(int framelen, int order, int buflen, int num_channels, int sr)
    : total_num_channels(num_channels), FRAMELEN(framelen), ORDER(order), 
      BUFLEN(buflen), SAMPLERATE(sr), i(0)
{
    nfft = static_cast<int>(std::pow(2, std::ceil(std::log2(FRAMELEN))));
    prev_frame_len = FRAMELEN;
    HOPSIZE = FRAMELEN / 2;
    MAX_EXLEN = SAMPLERATE / 6;
    EXLEN = MAX_EXLEN;
    ex_type = 0;
    order_changed = false;
    ex_type_changed = false;
    ex_start_changed = false;
    ex_start = 0.0;
    start = false;
    
    // Initialize per-channel arrays
    in_wt_ptrs.resize(num_channels, 0);
    in_rd_ptrs.resize(num_channels, 0);
    smp_cnts.resize(num_channels, 0);
    out_wt_ptrs.resize(num_channels, HOPSIZE);
    out_rd_ptrs.resize(num_channels, 0);
    ex_ptrs.resize(num_channels, 0);
    hist_ptrs.resize(num_channels, 0);
    ex_cnt_ptrs.resize(num_channels, 0);
    
    // Initialize LPC coefficients and working arrays
    maxFrameDurS = static_cast<double>(framelen) / sr;
    phi.resize(ORDER + 1, 0.0);
    alphas.resize(ORDER + 1, 0.0);
    ordered_in_buf.resize(static_cast<int>(SAMPLERATE * maxFrameDurS), 0.0);
    window.resize(static_cast<int>(SAMPLERATE * maxFrameDurS), 0.0);
    
    // Initialize multi-channel buffers
    in_buf.resize(num_channels);
    out_buf.resize(num_channels);
    out_hist.resize(num_channels);
    
    for (int ch = 0; ch < num_channels; ++ch) {
        in_buf[ch].resize(BUFLEN, 0.0);
        out_buf[ch].resize(BUFLEN, 0.0);
        out_hist[ch].resize(ORDER, 0.0);
    }
    
    // Initialize Hanning window
    for (int i = 0; i < FRAMELEN; ++i) {
        window[i] = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FRAMELEN - 1)));
    }
    
    // Initialize LPC coefficients
    reset_a();
}

void LPCDebug::set_exlen(int val) {
    EXLEN = val;
}

int LPCDebug::get_exlen() const {
    return EXLEN;
}

int LPCDebug::get_max_exlen() const {
    return MAX_EXLEN;
}

void LPCDebug::prepare_to_play() {
    // Resize arrays for new channel count
    in_wt_ptrs.assign(total_num_channels, 0);
    in_rd_ptrs.assign(total_num_channels, 0);
    smp_cnts.assign(total_num_channels, 0);
    out_wt_ptrs.assign(total_num_channels, HOPSIZE);
    out_rd_ptrs.assign(total_num_channels, 0);
    ex_ptrs.assign(total_num_channels, 0);
    hist_ptrs.assign(total_num_channels, 0);
    ex_cnt_ptrs.assign(total_num_channels, 0);
    
    // Resize buffers
    in_buf.clear();
    out_buf.clear();
    out_hist.clear();
    
    in_buf.resize(total_num_channels);
    out_buf.resize(total_num_channels);
    out_hist.resize(total_num_channels);
    
    for (int ch = 0; ch < total_num_channels; ++ch) {
        in_buf[ch].resize(BUFLEN, 0.0);
        out_buf[ch].resize(BUFLEN, 0.0);
        out_hist[ch].resize(ORDER, 0.0);
    }
}

void LPCDebug::set_noise(const std::vector<double>& noise_data) {
    noise = noise_data;
}

void LPCDebug::reset_a() {
    alphas[0] = 1.0;
    for (int i = 1; i <= ORDER; ++i) {
        alphas[i] = 0.0;
    }
}

double LPCDebug::autocorrelate(const std::vector<double>& x, int frame_size, int lag) {
    double res = 0.0;
    for (int n = 0; n < frame_size - lag; ++n) {
        res += x[n] * x[n + lag];
    }
    return res;
}

double LPCDebug::levinson_durbin() {
    reset_a();
    double E = phi[0];
    
    for (int k = 0; k < ORDER; ++k) {
        double lbda = 0.0;
        for (int j = 0; j <= k; ++j) {
            lbda += alphas[j] * phi[k + 1 - j];
        }
        lbda = -lbda / E;
        
        int half = (k + 1) / 2;
        for (int n = 0; n <= half; ++n) {
            double tmp = alphas[k + 1 - n] + lbda * alphas[n];
            alphas[n] += lbda * alphas[k + 1 - n];
            alphas[k + 1 - n] = tmp;
        }
        
        E *= (1.0 - lbda * lbda);
    }
    
    return E;
}

void LPCDebug::apply_lpc(const std::vector<double>& input_data, std::vector<double>& output, 
                        int num_samples, double lpc_mix, double ex_percentage, int ch, 
                        double ex_start_pos, double previous_gain, double current_gain) {
    if (noise.empty()) {
        return;
    }
    
    // Get per-channel state
    int in_wt_ptr = in_wt_ptrs[ch];
    int in_rd_ptr = in_rd_ptrs[ch];
    int smp_cnt = smp_cnts[ch];
    int out_rd_ptr = out_rd_ptrs[ch];
    int out_wt_ptr = (out_rd_ptr + HOPSIZE) % BUFLEN;
    
    if (HOPSIZE < prev_frame_len / 2) {
        for (int i = 0; i < prev_frame_len / 2 - HOPSIZE; ++i) {
            int idx = (out_wt_ptr + i) % BUFLEN;
            out_buf[ch][idx] = 0.0;
        }
    }
    
    int ex_start = static_cast<int>(ex_start_pos * EXLEN);
    
    if (ex_type_changed) {
        ex_ptrs[ch] = ex_start;
        ex_cnt_ptrs[ch] = 0;
        hist_ptrs[ch] = 0;
    }
    
    if (order_changed) {
        std::fill(out_hist[ch].begin(), out_hist[ch].end(), 0.0);
        hist_ptrs[ch] = 0;
    }
    
    int ex_ptr = ex_ptrs[ch];
    int ex_cnt_ptr = ex_cnt_ptrs[ch];
    int hist_ptr = hist_ptrs[ch];
    
    double slope = (current_gain - previous_gain) / num_samples;
    int valid_hop_size = std::min(HOPSIZE, prev_frame_len / 2);
    
    for (int s = 0; s < num_samples; ++s) {
        // Store input sample
        in_buf[ch][in_wt_ptr] = input_data[s];
        
        in_wt_ptr = (in_wt_ptr + 1) % BUFLEN;
        
        // Get output sample
        double out = out_buf[ch][out_rd_ptr];
        
        // Get delayed input sample
        double in_sample = in_buf[ch][(in_rd_ptr + BUFLEN - FRAMELEN) % BUFLEN];
        in_rd_ptr = (in_rd_ptr + 1) % BUFLEN;
        
        // Apply gain ramp
        double gain_factor = previous_gain + slope * s;
        output[s] = lpc_mix * gain_factor * out + (1.0 - lpc_mix) * in_sample;
        
        // Clear output buffer
        out_buf[ch][out_rd_ptr] = 0.0;
        out_rd_ptr = (out_rd_ptr + 1) % BUFLEN;
        
        smp_cnt += 1;
        
        // Process frame when hop size is reached
        if (smp_cnt >= valid_hop_size) {
            smp_cnt = 0;
            
            // Fill ordered buffers with windowed input
            for (int i = 0; i < FRAMELEN; ++i) {
                int in_buf_idx = (in_wt_ptr + i - FRAMELEN + BUFLEN) % BUFLEN;
                ordered_in_buf[i] = window[i] * in_buf[ch][in_buf_idx];
            }
            
            // Compute autocorrelation
            for (int lag = 0; lag <= ORDER; ++lag) {
                phi[lag] = autocorrelate(ordered_in_buf, FRAMELEN, lag);
            }
            
            if (phi[0] != 0.0) {
                // Compute LPC coefficients
                double var = levinson_durbin();
                double G = std::sqrt(var);
                
                // Generate output frame
                for (int n = 0; n < FRAMELEN; ++n) {
                    double ex = noise[ex_ptr];
                    ex_cnt_ptr += 1;
                    ex_ptr += 1;
                    
                    if (ex_cnt_ptr >= static_cast<int>(ex_percentage * EXLEN)) {
                        ex_cnt_ptr = 0;
                        ex_ptr = ex_start;
                    }
                    
                    if (ex_ptr >= EXLEN) {
                        ex_ptr = 0;
                    }
                    
                    // Apply LPC synthesis filter
                    double out_n = G * ex;
                    for (int k = 0; k < ORDER; ++k) {
                        int idx = (hist_ptr + k) % ORDER;
                        out_n -= alphas[k + 1] * out_hist[ch][idx];
                    }
                    
                    hist_ptr = (hist_ptr - 1 + ORDER) % ORDER;
                    out_hist[ch][hist_ptr] = out_n;
                    
                    int wt_idx = (out_wt_ptr + n) % BUFLEN;
                    out_buf[ch][wt_idx] += out_n;
                }
                
                // Reset history (as in Python code)
                std::fill(out_hist[ch].begin(), out_hist[ch].end(), 0.0);
                hist_ptrs[ch] = 0;
            }
            
            out_wt_ptr = (out_wt_ptr + HOPSIZE) % BUFLEN;
        }
    }
    
    // Store per-channel state
    in_rd_ptrs[ch] = in_rd_ptr;
    in_wt_ptrs[ch] = in_wt_ptr;
    smp_cnts[ch] = smp_cnt;
    out_wt_ptrs[ch] = out_wt_ptr;
    out_rd_ptrs[ch] = out_rd_ptr;
    ex_ptrs[ch] = ex_ptr;
    hist_ptrs[ch] = hist_ptr;
    ex_cnt_ptrs[ch] = ex_cnt_ptr;
}