#pragma once

#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
#include <algorithm>

struct StringNote
{
    int pitch = -1; // -1 = muted
    int fret  = -1;
};

struct VoicingResult
{
    std::array<StringNote, 6> voicing;
    int score = -10000;
};

struct VoicingParams
{
    std::array<int, 6> openPitches;
    int fretSpan       = 4;
    int maxFret        = 12;
    bool preferOpen    = true;
    int searchRange    = 5;
    int initialPosition = 0;
};

class GuitarVoicer
{
public:
    GuitarVoicer();

    // Tuning definitions: [string6..string1] as MIDI note numbers
    static constexpr int NUM_TUNINGS = 5;
    static constexpr int NUM_STRINGS = 6;

    static const std::array<std::array<int, NUM_STRINGS>, NUM_TUNINGS> TUNINGS;
    static const std::array<int, 8> CC_MAP;

    // Scoring weights
    static constexpr int SCORE_ALL_PITCHCLASSES  = 1000;
    static constexpr int SCORE_ROOT_IN_BASS      = 200;
    static constexpr int SCORE_NO_INNER_MUTE     = 100;
    static constexpr int SCORE_PER_SOUNDING      = 50;
    static constexpr int SCORE_PROXIMITY         = 40;
    static constexpr int SCORE_SMALL_SPAN        = 30;
    static constexpr int SCORE_OPEN_STRING_BONUS = 20;

    std::array<int, NUM_STRINGS> getStringOpenPitches (int tuningIndex, int capo) const;

    int scoreVoicing (const std::array<StringNote, NUM_STRINGS>& voicing,
                      const std::vector<int>& pitchClasses,
                      int rootPitchClass,
                      bool preferOpen) const;

    VoicingResult findBestVoicing (const std::vector<int>& pitchClasses,
                                   int rootPitchClass,
                                   int position,
                                   const VoicingParams& params);

    VoicingResult findBestPosition (const std::vector<int>& pitchClasses,
                                    int rootPitchClass,
                                    const VoicingParams& params,
                                    int ccPositionOverride);

    int getCurrentPosition() const { return currentPosition; }
    void setCurrentPosition (int pos) { currentPosition = pos; }
    void clearCache() { voicingCache.clear(); }
    void reset();

private:
    int currentPosition = -1;
    std::unordered_map<std::string, VoicingResult> voicingCache;

    static std::string makeCacheKey (const std::vector<int>& pitchClasses, int position,
                                     const std::array<int, NUM_STRINGS>& openPitches);
};
