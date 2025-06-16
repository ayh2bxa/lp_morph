#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>

class AudioLogger {
public:
    AudioLogger();
    ~AudioLogger();
    
    // Sequential logging methods - real-time safe
    void logInputBuffer(const float* inputBuffer, int bufferSize, int channel) noexcept;
    void logAlphas(const double* alphas, int order) noexcept;
    void logOutputBuffer(const float* outputBuffer, int bufferSize) noexcept;
    void logG(double G, double E) noexcept;
    
    void enableLogging(bool enable) noexcept;
    bool isLoggingEnabled() const noexcept { return loggingEnabled.load(); }
    
    // Background thread operations
    void writeLogToFile(const std::string& filePath);
    void clearLog() noexcept;
    
    // Automatic periodic writing
    void setLogDirectory(const std::string& directory) { logDirectory = directory; }
    void autoWriteCallback();
    
private:
    static constexpr int MAX_FIFO_SIZE = 4096;
    static constexpr int MAX_BUFFER_SIZE = 4096;
    static constexpr int MAX_LPC_ORDER = 100;
    
    struct LogEntry {
        std::array<float, MAX_BUFFER_SIZE> inputBuffer;
        std::array<float, MAX_BUFFER_SIZE> outputBuffer;
        std::array<double, MAX_LPC_ORDER> alphas;
        int inputBufferSize;
        int alphasCount;
        juce::uint64 timestamp;
        bool isComplete;
        double G;
        double E;
        int channel;
    };
    
    void finalizeLogEntry() noexcept;
    
    std::array<LogEntry, MAX_FIFO_SIZE> logFifo;
    std::atomic<int> fifoWriteIndex{0};
    std::atomic<int> fifoReadIndex{0};
    std::atomic<bool> loggingEnabled{false};
    std::atomic<bool> logReady{false};
    
    // Current log entry being built
    LogEntry* currentEntry{nullptr};
    int currentWriteIndex{0};
    
    // Auto-write functionality
    std::unique_ptr<juce::Timer> autoWriteTimer;
    std::string logDirectory;
    std::string generateLogFilename() const;
};
