//
//  logdbg.h
//  voicemorph
//
//  Created by Anthony Hong on 2025-03-26.
//

#ifndef logdbg_h
#define logdbg_h
// #define kSharedMemoryMapFile ...

static const size_t kMaxDebugLogEntries = 2048;
struct LogDebugEntry {
    int64 timeAtStart;
    int64 timeAtEnd;
    int numSamples;
}

struct LogDebug {
    LogDebugEntry m_logDebugEntries[kMaxDebugLogEntries];
    volatile size_t m_logHead;
}

#define DUMP_ENTRY(f, entry) fprintf (f, "")

#endif /* logdbg_h */
