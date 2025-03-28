/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   BassyTrainNoise_wav;
    const int            BassyTrainNoise_wavSize = 306106;

    extern const char*   CherubScreams_wav;
    const int            CherubScreams_wavSize = 203722;

    extern const char*   MicScratch_wav;
    const int            MicScratch_wavSize = 81598;

    extern const char*   Ring_wav;
    const int            Ring_wavSize = 61242;

    extern const char*   TrainScreech1_wav;
    const int            TrainScreech1_wavSize = 407258;

    extern const char*   TrainScreech2_wav;
    const int            TrainScreech2_wavSize = 488674;

    extern const char*   WhiteNoise_wav;
    const int            WhiteNoise_wavSize = 325704;

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
