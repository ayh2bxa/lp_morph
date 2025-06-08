#include "lpc_debug.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sndfile.h>
#include <algorithm>
#include <cmath>

std::vector<double> read_wav_file(const std::string& filename, int& sample_rate) {
    SF_INFO sfinfo;
    SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return {};
    }
    
    sample_rate = sfinfo.samplerate;
    std::vector<double> data(sfinfo.frames);
    sf_read_double(file, data.data(), sfinfo.frames);
    sf_close(file);
    for (int i = 0; i < data.size(); i++) {
        data[i] = (float)(data[i]);
        data[i] = (double)(data[i]);
    }
    return data;
}

void write_wav_file(const std::string& filename, const std::vector<double>& data, int sample_rate) {
    SF_INFO sfinfo;
    sfinfo.samplerate = sample_rate;
    sfinfo.channels = 1;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    
    SNDFILE* file = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);
    if (!file) {
        std::cerr << "Error creating file: " << filename << std::endl;
        return;
    }
    
    sf_write_double(file, data.data(), data.size());
    sf_close(file);
}

int main() {
    const int fs = 44100;
    
    // Read input audio file
    int input_sr;
    std::vector<double> x = read_wav_file("breakloop.wav", input_sr);
    
    if (x.empty()) {
        std::cerr << "Failed to read input file" << std::endl;
        return 1;
    }
    
    std::cout << "Input data type: double (64-bit)" << std::endl;
    std::cout << "Sample rate: " << input_sr << std::endl;
    std::cout << "Input size: " << x.size() << " samples" << std::endl;
    
    // Process parameters
    const int sysBlocksize = 4096;
    const int num_blocks = static_cast<int>(x.size()) / sysBlocksize;
    std::vector<int> frameLens = {2205};
    std::vector<int> orders = {64};
    
    // Output buffer
    std::vector<double> out(sysBlocksize * num_blocks, 0.0);
    
    // Read noise excitation
    int noise_sr;
    std::vector<double> noise = read_wav_file("audio/source/WhiteNoise.wav", noise_sr);
    
    if (noise.empty()) {
        std::cerr << "Failed to read noise file" << std::endl;
        return 1;
    }
    
    // Process with different frame lengths and orders
    for (int framelen : frameLens) {
        for (int order : orders) {
            std::cout << "Processing with framelen=" << framelen << ", order=" << order << std::endl;
            
            // Create LPC processor
            LPCDebug lpc(framelen, order);
            lpc.prepare_to_play();
            lpc.set_noise(noise);
            
            // Process audio in blocks
            for (int i = 0; i < num_blocks; ++i) {
                // Extract block
                std::vector<double> xBlock(x.begin() + i * sysBlocksize, 
                                         x.begin() + (i + 1) * sysBlocksize);
                
                // Process block
                std::vector<double> outBlock(sysBlocksize);
                lpc.apply_lpc(xBlock, outBlock, sysBlocksize, 1.0, 1.0, 0, 0.0, 1.0, 1.0);
                
                // Copy to output buffer
                std::copy(outBlock.begin(), outBlock.end(), out.begin() + i * sysBlocksize);
            }
        }
    }
    
    // Normalize output
    double max_abs = 0.0;
    for (double sample : out) {
        max_abs = std::max(max_abs, std::abs(sample));
    }
    
    std::vector<double> out_norm(out.size());
    const double norm_factor = 1.0 / (1.1 * max_abs);
    for (size_t i = 0; i < out.size(); ++i) {
        out_norm[i] = out[i] * norm_factor;
    }
    
    // Write output files
    write_wav_file("out_norm_break.wav", out_norm, fs);
    write_wav_file("out_break.wav", out, fs);
    
    std::cout << "Processing complete. Output files written." << std::endl;
    
    return 0;
}