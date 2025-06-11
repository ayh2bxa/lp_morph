# LP Morph (Work in Progress)

### [Plugin Demo](https://drive.google.com/file/d/1vFPuKh455JisTIh8pYWHy9DkOq8HJ3YP/viewhttps://drive.google.com/file/d/1v_1yRhYxDeW2HGc8Xrev9TFbFtctd5HF/view?usp=drive_linkhttps://drive.google.com/file/d/1pSDDkjCGImsYZ_IlyGU_NulDk03rHYPb/view?usp=sharinghttps://drive.google.com/file/d/1PZ8PmsiGHm-4qqMvpU1HNK3sbAYAznpV/view?usp=sharing)

### Background

Linear predictive coding (LPC for short) is an algorithm widely used in speech signal compression and synthesis. It is especially suitable for modelling the human vocal tract.

From the input speech signal, LPC derives a set of coefficients to represent its formant characteristics, then uses these coefficients to filter an excitation signal. The output is the resynthesised speech. For example, let the input be the sound of the vowel "O", and the excitation signal be white noise. Then the output of LPC will be white noise with a very strong "O" characteristic imposed on it.

### About the plugin

But what if the input is not restricted to voice, but can also be drums, or synths, or any other sounds? Although linear prediction cannot accurately model every acoustic system as it does our vocal tracts, the unpredictability of its modelling ability on non-speech audio gives it artistic potential, which is the key motivation behind building this plugin.


LP Morph applies real-time LPC to input audio, and allows control of the following parameters:

1. Mix: controls the dry/wet percentage of LPC effect.
2. Order: the number of coefficients used to characterise the input. Higher order means greater resolution.
3. Frame Duration: determines how many input samples (in ms) to use for LPC processing.
4. Gain: controls the volume in dB. LPC tends to produce noticeably louder output, this helps prevent clipping.
5. Dropdown menu: features a variety of excitation signals, including white noise, ring tone, kid screaming, train, and more. All sounds are produced or recorded by myself.
6. Length: controls the length of excitation signal that's actually being used in output synthesis. When the length is small, you will hear a strong pitch component in the output, because the same excitation samples are being looped in a small period.
7. Start: determines the position (in fraction) of excitation to start processing.

For a more straightforward explanation, see the demo linked on top.
