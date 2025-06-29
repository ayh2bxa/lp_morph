#include "AudioLogger.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>

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

void AudioLogger::logInputBuffer(const float* inputBuffer, int bufferSize, int channel) noexcept
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
    currentEntry->channel = channel;
    
    // Copy input buffer
//    for (int i = 0; i < bufferSize; ++i) {
//        currentEntry->inputBuffer[i] = inputBuffer[i];
//    }
}

void AudioLogger::logAlphas(const double* alphas, int order) noexcept
{
    if (!loggingEnabled.load() || !currentEntry || order > MAX_LPC_ORDER)
        return;
    
    // Start building new log entry
    currentEntry->alphasCount = order+1;
    
    // Copy alphas
    for (int i = 0; i < order; ++i) {
        currentEntry->alphas[i] = alphas[i];
    }
}

void AudioLogger::logPhis(const double* phis, int order) noexcept
{
    if (!loggingEnabled.load() || !currentEntry || order > MAX_LPC_ORDER)
        return;
    
    // Start building new log entry
    currentEntry->alphasCount = order;
    
    // Copy alphas
    for (int i = 0; i < MAX_LPC_ORDER+1; ++i) {
        currentEntry->phis[i] = phis[i];
    }
}

void AudioLogger::logG(double G, double E) noexcept
{
    if (!loggingEnabled.load() || !currentEntry)
        return;
    
    // Start building new log entry
    currentEntry->G = G;
    currentEntry->E = E;
}

void AudioLogger::logOutputBuffer(const float* outputBuffer, int bufferSize) noexcept
{
    if (!loggingEnabled.load() || !currentEntry || bufferSize > MAX_BUFFER_SIZE)
        return;
    
//    // Copy output buffer
//    for (int i = 0; i < bufferSize; ++i) {
//        currentEntry->outputBuffer[i] = outputBuffer[i];
//    }
    
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
    
    // Signal background thread to write (real-time safe)
    if (autoWriteTimer && !autoWriteTimer->isTimerRunning())
    {
        autoWriteTimer->startTimer(1); // Trigger immediate write on background thread
    }
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
    
    // Set maximum precision for floating point numbers
    file << std::setprecision(std::numeric_limits<double>::max_digits10);
    
    // Write CSV header
    file << "timestamp,channel,bufferSize,G,E,";
    
//    // Input buffer columns
//    for (int i = 0; i < MAX_BUFFER_SIZE; ++i) {
//        file << "input_" << i << ",";
//    }
//    
//    // Output buffer columns
//    for (int i = 0; i < MAX_BUFFER_SIZE; ++i) {
//        file << "output_" << i << ",";
//    }
    
    // Alphas columns
    for (int i = 0; i < MAX_LPC_ORDER+1; ++i) {
        file << "phi_" << i;
        if (i < MAX_LPC_ORDER) file << ",";
    }
    file << "\n";
    
    // Write all entries currently in the FIFO (entire circular buffer contents)
    for (int i = 0; i < MAX_FIFO_SIZE; ++i) {
        const LogEntry& entry = logFifo[i];
        
        // Only write complete entries
        if (entry.isComplete) {
            file << entry.timestamp << "," 
                 << entry.channel << ","
                 << entry.inputBufferSize << ","
                 << entry.G << ","
                 << entry.E << ",";
            
            // Write alphas
            for (int j = 0; j < MAX_LPC_ORDER+1; j++) {
                file << entry.phis[j];
                if (j < MAX_LPC_ORDER) file << ",";
            }
            file << "\n";
        }
    }
    
    // Don't update read index - we want to keep data in FIFO for next write
    
    file.close();
    std::cout << "Audio log written to: " << filePath << std::endl;
}

void AudioLogger::enableLogging(bool enable) noexcept
{
    loggingEnabled.store(enable);
    
    // No automatic timer - we write immediately on each new entry
    if (autoWriteTimer && autoWriteTimer->isTimerRunning())
    {
        autoWriteTimer->stopTimer();
    }
}

std::string AudioLogger::generateLogFilename() const
{
    // Always use the same filename - it will be overwritten each time
    return logDirectory + "/audio_log.csv";
}

void AudioLogger::autoWriteCallback()
{
    if (logReady.load())
    {
        std::string filename = generateLogFilename();
        writeLogToFile(filename);
        
        // Stop timer after writing - will restart when next entry is ready
        if (autoWriteTimer)
            autoWriteTimer->stopTimer();
    }
}

void AudioLogger::clearLog() noexcept
{
    fifoWriteIndex.store(0);
    fifoReadIndex.store(0);
    logReady.store(false);
    currentEntry = nullptr;
}
