#pragma once

#include <vector>
#include <cmath>
#include <string>

class LPCDebug {
public:
    LPCDebug(int framelen = 441, int order = 32, int buflen = 16384, int num_channels = 1, int sr = 44100);
    
    void set_exlen(int val);
    int get_exlen() const;
    int get_max_exlen() const;
    void prepare_to_play();
    void set_noise(const std::vector<double>& noise_data);
    
    void apply_lpc(const std::vector<double>& input_data, std::vector<double>& output, 
                   int num_samples, double lpc_mix, double ex_percentage, int ch, 
                   double ex_start_pos, double previous_gain = 1.0, double current_gain = 1.0);

private:
    // Core parameters
    int total_num_channels;
    int FRAMELEN;
    int nfft;
    int prev_frame_len;
    int HOPSIZE;
    int BUFLEN;
    int SAMPLERATE;
    int MAX_EXLEN;
    int EXLEN;
    int ORDER;
    
    // State variables
    int ex_type;
    bool order_changed;
    bool ex_type_changed;
    bool ex_start_changed;
    double ex_start;
    bool start;
    int i;
    
    // Per-channel state arrays
    std::vector<int> in_wt_ptrs;
    std::vector<int> in_rd_ptrs;
    std::vector<int> smp_cnts;
    std::vector<int> out_wt_ptrs;
    std::vector<int> out_rd_ptrs;
    std::vector<int> ex_ptrs;
    std::vector<int> hist_ptrs;
    std::vector<int> ex_cnt_ptrs;
    
    // LPC working arrays
    double maxFrameDurS;
    std::vector<double> phi;
    std::vector<double> alphas;
    std::vector<double> ordered_in_buf;
    std::vector<double> window;
    
    // Multi-channel buffers
    std::vector<std::vector<double>> in_buf;
    std::vector<std::vector<double>> out_buf;
    std::vector<std::vector<double>> out_hist;
    
    // Noise excitation source
    std::vector<double> noise;
    
    // Private methods
    void reset_a();
    double autocorrelate(const std::vector<double>& x, int frame_size, int lag);
    double levinson_durbin();
};