import numpy as np
import soundfile as sf
from utils.lpc import LPC
from utils.alphas_plot import analyse_alphas, plot_pole_zero
# from scipy.signal import tf2zpk

class AudioProcessor:
    def __init__(self, input_file, excitation_file=None, sample_rate=44100, block_size=512):
        self.sample_rate = sample_rate
        self.block_size = block_size
        self.lpc = LPC(num_channels=2)  # Stereo by default
        
        # Load audio files
        self.input_audio, self.input_sr = sf.read(input_file)
        if self.input_audio.ndim == 1:
            self.input_audio = np.column_stack((self.input_audio, self.input_audio))  # Convert mono to stereo
            
        if excitation_file:
            self.excitation_audio, self.excitation_sr = sf.read(excitation_file)
            if self.excitation_audio.ndim == 1:
                self.excitation_audio = np.column_stack((self.excitation_audio, self.excitation_audio))
        else:
            self.excitation_audio = None
        
        # Set default parameters
        self.lpc.FRAMELEN = int(sample_rate * 0.01)
        self.lpc.ORDER = 32
        self.wet_gain = 1.0  # 0dB
        self.lpc_mix = 1.0
        self.ex_len = 1.0
        self.ex_start = 0.0
        
        # Initialize processing state
        self.previous_gain = self.wet_gain
        self.current_gain = self.wet_gain
        self.position = 0
        self.output_audio = np.zeros_like(self.input_audio)
        
    def process_block(self, input_block, excitation_block=None):
        num_samples = input_block.shape[0]
        num_channels = input_block.shape[1]
        
        output_block = np.zeros_like(input_block)
        
        for ch in range(num_channels):
            input_ch = input_block[:, ch]
            output_ch = output_block[:, ch]
            excitation_ch = excitation_block[:, ch] if excitation_block is not None else None
            
            self.lpc.apply_lpc(
                input_ch, output_ch, num_samples,
                lpc_mix=self.lpc_mix,
                ex_percentage=self.ex_len,
                ch=ch,
                ex_start_pos=self.ex_start,
                sidechain=excitation_ch,
                previous_gain=self.previous_gain,
                current_gain=self.current_gain
            )
        
        return output_block
    
    def process(self):
        total_samples = len(self.input_audio)
        num_blocks = int(np.ceil(total_samples / self.block_size))
        
        for i in range(num_blocks):
            start = i * self.block_size
            end = min((i + 1) * self.block_size, total_samples)
            
            input_block = self.input_audio[start:end]
            
            if self.excitation_audio is not None:
                # Handle case where excitation might be shorter than input
                exc_start = start % len(self.excitation_audio)
                exc_end = exc_start + (end - start)
                if exc_end > len(self.excitation_audio):
                    # Wrap around if excitation is shorter than input
                    remaining = exc_end - len(self.excitation_audio)
                    exc_block = np.vstack((
                        self.excitation_audio[exc_start:],
                        self.excitation_audio[:remaining]
                    ))
                else:
                    exc_block = self.excitation_audio[exc_start:exc_end]
            else:
                exc_block = None
            
            output_block = self.process_block(input_block, exc_block)
            self.output_audio[start:end] = output_block
            
            # Update gain smoothing for next block
            self.previous_gain = self.current_gain
            
            if i == 100:
                # print(self.lpc.G)
                # analyse_alphas(self.lpc.G, self.lpc.alphas, self.lpc.FRAMELEN, self.sample_rate)
                plot_pole_zero(self.lpc.G, self.lpc.alphas, self.lpc.FRAMELEN, self.sample_rate)
                # z, p, k = tf2zpk(self.lpc.G, self.lpc.alphas)
                # print(z)
                # print(np.abs(p))
                # print(k)
        
        return self.output_audio

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="LPC Audio Processor")
    parser.add_argument("input_file", help="Input audio file")
    parser.add_argument("output_file", help="Output audio file")
    parser.add_argument("--excitation", help="Excitation audio file (optional)")
    parser.add_argument("--sample_rate", type=int, default=44100, help="Sample rate")
    parser.add_argument("--block_size", type=int, default=512, help="Processing block size")
    parser.add_argument("--order", type=int, default=32, help="LPC order")
    parser.add_argument("--wet_gain", type=float, default=0.0, help="Wet gain in dB")
    parser.add_argument("--lpc_mix", type=float, default=1.0, help="LPC mix (0.0 to 1.0)")
    parser.add_argument("--ex_len", type=float, default=1.0, help="Excitation length (0.0 to 1.0)")
    parser.add_argument("--ex_start", type=float, default=0.0, help="Excitation start position (0.0 to 1.0)")
    
    args = parser.parse_args()
    
    # Convert dB to linear gain
    wet_gain_linear = 10 ** (args.wet_gain / 20)
    
    processor = AudioProcessor(
        input_file=args.input_file,
        excitation_file=args.excitation,
        sample_rate=args.sample_rate,
        block_size=args.block_size
    )
    
    # Set parameters from command line
    processor.lpc.ORDER = args.order
    processor.wet_gain = wet_gain_linear
    processor.lpc_mix = args.lpc_mix
    processor.ex_len = args.ex_len
    processor.ex_start = args.ex_start
    
    # Process the audio
    output_audio = processor.process()
    output_audio /= (1.05*np.max(np.abs(output_audio)))
    # Write output file
    sf.write(args.output_file, output_audio, args.sample_rate)
    
    print(f"Processing complete. Output written to {args.output_file}")