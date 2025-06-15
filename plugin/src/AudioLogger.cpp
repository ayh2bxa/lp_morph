#include "AudioLogger.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

class AutoWriteTimer : public juce::Timer
{
public:
    AutoWriteTimer(AudioLogger* logger) : audioLogger(logger) {}
    
    void timerCallback() override
    {
        if (audioLogger)
            audioLogger->autoWriteCallback();
    }
    
private:
    AudioLogger* audioLogger;
};

AudioLogger::AudioLogger()
{
    clearLog();
    
    // Try multiple fallback locations for log directory
    juce::File logDir;
    
    // Try 1: Current working directory
    juce::File projectDir = juce::File::getCurrentWorkingDirectory();
    logDir = projectDir.getChildFile("audio_logs");
    std::cout << "Trying to create directory at: " << logDir.getFullPathName() << std::endl;
    
    if (!logDir.createDirectory()) {
        // Try 2: User's Documents folder
        logDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("lp_morph_audio_logs");
        std::cout << "Fallback: Trying Documents folder at: " << logDir.getFullPathName() << std::endl;
        
        if (!logDir.createDirectory()) {
            // Try 3: Desktop as last resort
            logDir = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("lp_morph_audio_logs");
            std::cout << "Final fallback: Trying Desktop at: " << logDir.getFullPathName() << std::endl;
            logDir.createDirectory();
        }
    }
    
    logDirectory = logDir.getFullPathName().toStdString();
    std::cout << "Audio logs will be saved to: " << logDirectory << std::endl;
    
    // Set up auto-write timer
    autoWriteTimer = std::make_unique<AutoWriteTimer>(this);
}

AudioLogger::~AudioLogger()
{
    if (autoWriteTimer && autoWriteTimer->isTimerRunning())
    {
        autoWriteTimer->stopTimer();
        // Final write on destruction
        if (logReady.load())
            autoWriteCallback();
    }
}

void AudioLogger::logInputBuffer(const float* inputBuffer, int bufferSize) noexcept
{
    if (!loggingEnabled.load() || bufferSize > MAX_BUFFER_SIZE)
        return;
    
    // Get current write index for new entry
    currentWriteIndex = fifoWriteIndex.load();
    int nextWriteIndex = (currentWriteIndex + 1) % MAX_FIFO_SIZE;
    
    // Check if FIFO is full
    if (nextWriteIndex == fifoReadIndex.load())
        return; // FIFO full, drop this entry
    
    // Start building new log entry
    currentEntry = &logFifo[currentWriteIndex];
    currentEntry->isComplete = false;
    currentEntry->inputBufferSize = bufferSize;
    currentEntry->timestamp = juce::Time::getHighResolutionTicks();
    
    // Copy input buffer
    for (int i = 0; i < bufferSize; ++i) {
        currentEntry->inputBuffer[i] = inputBuffer[i];
    }
}

void AudioLogger::logAlphas(const double* alphas, int order) noexcept
{
    if (!loggingEnabled.load() || !currentEntry || order > MAX_LPC_ORDER)
        return;
    
    // Start building new log entry
    currentEntry->alphasCount = order;
    
    // Copy alphas
    for (int i = 0; i < order; ++i) {
        currentEntry->alphas[i] = alphas[i];
    }
}

void AudioLogger::logOutputBuffer(const float* outputBuffer, int bufferSize) noexcept
{
    if (!loggingEnabled.load() || !currentEntry || bufferSize > MAX_BUFFER_SIZE)
        return;
    
    // Copy output buffer
    for (int i = 0; i < bufferSize; ++i) {
        currentEntry->outputBuffer[i] = outputBuffer[i];
    }
    
    // Mark entry as complete and finalize
    finalizeLogEntry();
}

void AudioLogger::finalizeLogEntry() noexcept
{
    if (!currentEntry)
        return;
    
    currentEntry->isComplete = true;
    
    // Atomically advance write index to make entry available for reading
    int nextWriteIndex = (currentWriteIndex + 1) % MAX_FIFO_SIZE;
    fifoWriteIndex.store(nextWriteIndex);
    logReady.store(true);
    
    currentEntry = nullptr;
}

void AudioLogger::writeLogToFile(const std::string& filePath)
{
    if (!logReady.load())
        return;
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
        return;
    }
    
    // Write CSV header
    file << "timestamp,bufferSize,alphas_count,";
    
//    // Input buffer columns
//    for (int i = 0; i < MAX_BUFFER_SIZE; ++i) {
//        file << "input_" << i << ",";
//    }
//    
//    // Output buffer columns
//    for (int i = 0; i < MAX_BUFFER_SIZE; ++i) {
//        file << "output_" << i << ",";
//    }
    
//    // Alphas columns
//    for (int i = 0; i < MAX_LPC_ORDER; ++i) {
//        file << "alpha_" << i;
//        if (i < MAX_LPC_ORDER - 1) file << ",";
//    }
    file << "\n";
    
    // Read all available entries from FIFO
    int readIndex = fifoReadIndex.load();
    int writeIndex = fifoWriteIndex.load();
    
    while (readIndex != writeIndex) {
        const LogEntry& entry = logFifo[readIndex];
        
        // Only write complete entries
        if (entry.isComplete) {
            file << entry.timestamp << "," 
                 << entry.inputBufferSize << ","
                 << entry.alphasCount << ",";
            
//            // Write input buffer
//            for (int i = 0; i < MAX_BUFFER_SIZE; ++i) {
//                if (i < entry.inputBufferSize) {
//                    file << entry.inputBuffer[i];
//                } else {
//                    file << "0";
//                }
//                file << ",";
//            }
//            
//            // Write output buffer
//            for (int i = 0; i < MAX_BUFFER_SIZE; ++i) {
//                if (i < entry.outputBufferSize) {
//                    file << entry.outputBuffer[i];
//                } else {
//                    file << "0";
//                }
//                file << ",";
//            }
            
            // Write alphas
            for (int i = 0; i < MAX_LPC_ORDER; ++i) {
                if (i < entry.alphasCount) {
                    file << entry.alphas[i];
                }
                if (i < entry.alphasCount - 1) file << ",";
            }
            file << "\n";
        }
        
        readIndex = (readIndex + 1) % MAX_FIFO_SIZE;
    }
    
    // Update read index to mark all entries as consumed
    fifoReadIndex.store(writeIndex);
    
    file.close();
    std::cout << "Audio log written to: " << filePath << std::endl;
}

void AudioLogger::enableLogging(bool enable) noexcept
{
    loggingEnabled.store(enable);
    
    if (enable && autoWriteTimer)
    {
        autoWriteTimer->startTimer(autoWriteIntervalMs);
    }
    else if (autoWriteTimer)
    {
        autoWriteTimer->stopTimer();
    }
}

std::string AudioLogger::generateLogFilename() const
{
    auto now = juce::Time::getCurrentTime();
    auto timeString = now.formatted("%Y%m%d_%H%M%S").toStdString();
    return logDirectory + "/audio_log_" + timeString + ".csv";
}

void AudioLogger::autoWriteCallback()
{
    if (logReady.load())
    {
        std::string filename = generateLogFilename();
        writeLogToFile(filename);
    }
}

void AudioLogger::clearLog() noexcept
{
    fifoWriteIndex.store(0);
    fifoReadIndex.store(0);
    logReady.store(false);
    currentEntry = nullptr;
}
