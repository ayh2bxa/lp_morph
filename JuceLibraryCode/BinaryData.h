/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   BassyTrainNoise_wav_bin;
    const int            BassyTrainNoise_wav_binSize = 81416;

    extern const char*   CherubScreams_wav_bin;
    const int            CherubScreams_wav_binSize = 101770;

    extern const char*   MicScratch_wav_bin;
    const int            MicScratch_wav_binSize = 40708;

    extern const char*   Ring_wav_bin;
    const int            Ring_wav_binSize = 30530;

    extern const char*   TrainScreech1_wav_bin;
    const int            TrainScreech1_wav_binSize = 203538;

    extern const char*   TrainScreech2_wav_bin;
    const int            TrainScreech2_wav_binSize = 244246;

    extern const char*   WhiteNoise_wav_bin;
    const int            WhiteNoise_wav_binSize = 162830;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 7;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
