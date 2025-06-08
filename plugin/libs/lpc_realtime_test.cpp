#include "lpc.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>

int main() {
    const int sample_rate = 44100;
    const int num_channels = 2;
    const int test_duration_seconds = 10;
    const int total_samples = sample_rate * test_duration_seconds;
    const int block_size = 197;
    
    std::cout << "LPC Real-time Performance Test" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Test duration: " << test_duration_seconds << " seconds" << std::endl;
    std::cout << "Sample rate: " << sample_rate << " Hz" << std::endl;
    std::cout << "Block size: " << block_size << " samples" << std::endl;
    std::cout << "Channels: " << num_channels << std::endl;
    
    // Generate test input signal
    std::vector<float> input_signal(total_samples);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    
    for (int i = 0; i < total_samples; i++) {
        input_signal[i] = dist(gen);
    }
    
    // Generate noise excitation
    std::vector<double> noise(sample_rate);
    std::normal_distribution<double> noise_dist(0.0, 1.0);
    for (int i = 0; i < sample_rate; i++) {
        noise[i] = noise_dist(gen);
    }
    
    // Create LPC processor
    LPC lpc(num_channels);
    lpc.ORDER = 64;
    lpc.FRAMELEN = 2205;
    lpc.HOPSIZE = lpc.FRAMELEN / 2;
    lpc.SAMPLERATE = sample_rate;
    lpc.noise = &noise;
    
    std::vector<float> output_signal(total_samples);
    
    std::cout << "\nStarting processing..." << std::endl;
    const int num_blocks = total_samples / block_size;
    auto start_time = std::chrono::high_resolution_clock::now();
    // Process in blocks like a real DAW would
    for (int block = 0; block < num_blocks; block++) {
        const int start_idx = block * block_size;
        
        for (int ch = 0; ch < num_channels; ch++) {
            lpc.applyLPC(
                &input_signal[start_idx],
                &output_signal[start_idx],
                block_size,
                1.0f,  // lpc_mix
                1.0f,  // ex_percentage
                ch,    // channel
                0.0f,  // ex_start
                1.0,   // previous_gain
                1.0    // current_gain
            );
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double processing_time_seconds = duration.count() / 1000.0;
    
    double real_time_factor = test_duration_seconds / processing_time_seconds;
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "Audio duration: " << test_duration_seconds << " seconds" << std::endl;
    std::cout << "Processing time: " << processing_time_seconds << " seconds" << std::endl;
    std::cout << "Real-time factor: " << real_time_factor << "x" << std::endl;
    
    if (real_time_factor >= 1.0) {
        std::cout << "✓ REAL-TIME CAPABLE (" << real_time_factor << "x faster than real-time)" << std::endl;
    } else {
        std::cout << "✗ NOT REAL-TIME CAPABLE (" << (1.0/real_time_factor) << "x slower than real-time)" << std::endl;
    }
    
    return 0;
}