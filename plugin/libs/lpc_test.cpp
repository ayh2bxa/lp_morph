//#include "lpc.h"
//#include <iostream>
//#include <vector>
//#include <fstream>
//#include <sndfile.h>
//#include <algorithm>
//#include <cmath>
//
//std::vector<double> read_wav_file(const std::string& filename, int& sample_rate, int& channels) {
//   SF_INFO sfinfo;
//   SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &sfinfo);
//   
//   if (!file) {
//       std::cerr << "Error opening file: " << filename << std::endl;
//       return {};
//   }
//   
//   sample_rate = sfinfo.samplerate;
//   channels = sfinfo.channels;
//   std::vector<double> data(sfinfo.frames * sfinfo.channels);
//   sf_read_double(file, data.data(), sfinfo.frames * sfinfo.channels);
//   sf_close(file);
//   for (int i = 0; i < sfinfo.frames * sfinfo.channels; i++) {
//        float datai = (float)data[i];
//        data[i] = (double)datai;
//   }
//   return data;
//}
//
//void write_wav_file(const std::string& filename, const std::vector<float>& data, int sample_rate, int channels) {
//   SF_INFO sfinfo;
//   sfinfo.samplerate = sample_rate;
//   sfinfo.channels = channels;
//   sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
//   
//   SNDFILE* file = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);
//   if (!file) {
//       std::cerr << "Error creating file: " << filename << std::endl;
//       return;
//   }
//   
//   sf_write_float(file, data.data(), data.size());
//   sf_close(file);
//}
//
//int main() {
//   const int sample_rate = 44100;
//   const int num_channels = 2;
//   
//   std::cout << "LPC Implementation Test" << std::endl;
//   std::cout << "======================" << std::endl;
//   
//   // Load input audio file
//   int input_sr, input_channels;
//   std::vector<double> input_data = read_wav_file("breakloop.wav", input_sr, input_channels);
//   
//   if (input_data.empty()) {
//       std::cerr << "Failed to read input file" << std::endl;
//       return 1;
//   }
//   
//   std::cout << "Loaded breakloop.wav:" << std::endl;
//   std::cout << "  Sample rate: " << input_sr << std::endl;
//   std::cout << "  Channels: " << input_channels << std::endl;
//   std::cout << "  Total samples: " << input_data.size() << std::endl;
//   std::cout << "  Frames: " << input_data.size() / input_channels << std::endl;
//   
//   // Load white noise excitation
//   int noise_sr, noise_channels;
//   std::vector<double> noise_data = read_wav_file("WhiteNoise.wav", noise_sr, noise_channels);
////    for (int i = 0; i < noise_data.size(); i++) {
////        std::cout << "i: " << i << ",  value: " << noise_data[i] << std::endl;
////    }
////    exit(0);
//   if (noise_data.empty()) {
//       std::cerr << "Failed to read noise file" << std::endl;
//       return 1;
//   }
//   
//   std::cout << "Loaded WhiteNoise.wav:" << std::endl;
//   std::cout << "  Sample rate: " << noise_sr << std::endl;
//   std::cout << "  Samples: " << noise_data.size() << std::endl;
//   
//   // Create LPC processor
//   LPC lpc(num_channels);
//   
//   // Set fixed parameters
//   lpc.ORDER = 64;
//   lpc.FRAMELEN = 2205;
//   lpc.HOPSIZE = lpc.FRAMELEN / 2;
//   lpc.SAMPLERATE = sample_rate;
//   lpc.noise = &noise_data;
//   
////    lpc.prepareToPlay();
//   
//   std::cout << "\nLPC Parameters:" << std::endl;
//   std::cout << "  Sample Rate: " << lpc.SAMPLERATE << std::endl;
//   std::cout << "  Frame Length: " << lpc.FRAMELEN << std::endl;
//   std::cout << "  Hop Size: " << lpc.HOPSIZE << std::endl;
//   std::cout << "  Order: " << lpc.ORDER << std::endl;
//   std::cout << "  Channels: " << num_channels << std::endl;
//   
//   // Separate stereo channels
//   const int frames = input_data.size() / input_channels;
//   std::vector<float> left_channel(frames);
//   std::vector<float> right_channel(frames);
//   
//   for (int i = 0; i < frames; i++) {
//       left_channel[i] = static_cast<float>(input_data[i * input_channels]);
//       right_channel[i] = static_cast<float>(input_data[i * input_channels + 1]);
//   }
//   
//   // Prepare output buffers for each channel
//   std::vector<float> left_output(frames);
//   std::vector<float> right_output(frames);
//   
//   // Fixed processing parameters
//   const float lpc_mix = 1.0f;
//   const float ex_percentage = 1.0f;
//   const float ex_start = 0.0f;
//   const double gain = 1.0;
//   
//   std::cout << "\nProcessing Parameters:" << std::endl;
//   std::cout << "  LPC Mix: " << lpc_mix << std::endl;
//   std::cout << "  Excitation Percentage: " << ex_percentage << std::endl;
//   std::cout << "  Excitation Start: " << ex_start << std::endl;
//   
//   const int block_size = 197;
//   const int num_blocks = frames / block_size;
//   
//   std::cout << "\nProcessing " << num_blocks << " blocks of " << block_size << " samples each..." << std::endl;
//   
//   // Process left channel (ch=0)
//   std::cout << "Processing left channel..." << std::endl;
//   for (int block = 0; block < num_blocks; block++) {
//       const int start_idx = block * block_size;
//       const int end_idx = std::min(start_idx + block_size, frames);
//       const int actual_block_size = end_idx - start_idx;
//       
//       if (actual_block_size <= 0) break;
//       
//       lpc.applyLPC(
//           &left_channel[start_idx],
//           &left_output[start_idx], 
//           actual_block_size,
//           lpc_mix,
//           ex_percentage,
//           0,
//           ex_start,
//           gain,
//           gain
//       );
//       
//       if (block % 10 == 0) {
//           std::cout << "  Block " << block << "/" << num_blocks << std::endl;
//       }
//   }
//   
//   // Process right channel (ch=1)
//   std::cout << "Processing right channel..." << std::endl;
//   for (int block = 0; block < num_blocks; block++) {
//       const int start_idx = block * block_size;
//       const int end_idx = std::min(start_idx + block_size, frames);
//       const int actual_block_size = end_idx - start_idx;
//       
//       if (actual_block_size <= 0) break;
//       
//       lpc.applyLPC(
//           &right_channel[start_idx],
//           &right_output[start_idx], 
//           actual_block_size,
//           lpc_mix,
//           ex_percentage,
//           1,
//           ex_start,
//           gain,
//           gain
//       );
//       
//       if (block % 10 == 0) {
//           std::cout << "  Block " << block << "/" << num_blocks << std::endl;
//       }
//   }
//   
//   // Interleave stereo output
//   std::vector<float> stereo_output(frames * 2);
//   for (int i = 0; i < frames; i++) {
//       stereo_output[i * 2] = left_output[i];
//       stereo_output[i * 2 + 1] = right_output[i];
//   }
//   
//   // Write output file
//   write_wav_file("lpc_output.wav", stereo_output, sample_rate, 2);
//   
//   std::cout << "\nOutput written to lpc_output.wav" << std::endl;
//   std::cout << "LPC Test Complete!" << std::endl;
//   
//   return 0;
//}
