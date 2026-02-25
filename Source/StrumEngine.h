#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "StepSequencer.h"
#include <vector>
#include <random>

struct StrumNote
{
    int pitch;
    int velocity;
    int channel;
    double beatOffset; // beat offset from strum trigger point
};

class StrumEngine
{
public:
    StrumEngine();

    struct ActiveNote
    {
        int pitch;
        int channel;
    };

    // Generate strum notes with beat-based offsets
    std::vector<StrumNote> generateStrum (const std::vector<int>& notesToStrum,
                                          StepDirection direction,
                                          float velocity,
                                          float strumSpeedMs,
                                          float humanizeAmount, // 0-1
                                          bool multiChannel,
                                          double tempo);

    const std::vector<ActiveNote>& getActiveNotes() const { return activeNotes; }
    void setActiveNotes (std::vector<ActiveNote> notes) { activeNotes = std::move (notes); }
    void clearActiveNotes() { activeNotes.clear(); }

private:
    std::vector<ActiveNote> activeNotes;
    std::mt19937 rng;

    static int clamp (int value, int minVal, int maxVal);
};
