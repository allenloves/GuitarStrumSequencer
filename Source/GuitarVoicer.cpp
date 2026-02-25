#include "GuitarVoicer.h"

const std::array<std::array<int, GuitarVoicer::NUM_STRINGS>, GuitarVoicer::NUM_TUNINGS> GuitarVoicer::TUNINGS = {{
    {{ 40, 45, 50, 55, 59, 64 }},  // Standard:       E2 A2 D3 G3 B3 E4
    {{ 38, 45, 50, 55, 59, 64 }},  // Drop D:         D2 A2 D3 G3 B3 E4
    {{ 38, 43, 50, 55, 59, 62 }},  // Open G:         D2 G2 D3 G3 B3 D4
    {{ 38, 45, 50, 55, 57, 62 }},  // DADGAD:         D2 A2 D3 G3 A3 D4
    {{ 39, 44, 49, 54, 58, 63 }}   // Half Step Down: Eb2 Ab2 Db3 Gb3 Bb3 Eb4
}};

const std::array<int, 8> GuitarVoicer::CC_MAP = {{ 85, 86, 87, 102, 103, 104, 105, 106 }};

GuitarVoicer::GuitarVoicer()
{
    reset();
}

void GuitarVoicer::reset()
{
    currentPosition = -1;
    voicingCache.clear();
}

std::array<int, GuitarVoicer::NUM_STRINGS> GuitarVoicer::getStringOpenPitches (int tuningIndex, int capo) const
{
    tuningIndex = std::clamp (tuningIndex, 0, NUM_TUNINGS - 1);
    auto base = TUNINGS[static_cast<size_t> (tuningIndex)];
    std::array<int, NUM_STRINGS> result;
    for (int s = 0; s < NUM_STRINGS; ++s)
        result[static_cast<size_t> (s)] = base[static_cast<size_t> (s)] + capo;
    return result;
}

int GuitarVoicer::scoreVoicing (const std::array<StringNote, NUM_STRINGS>& voicing,
                                const std::vector<int>& pitchClasses,
                                int rootPitchClass,
                                bool preferOpen) const
{
    int score = 0;
    int soundingCount = 0;
    bool coveredClasses[12] = {};
    int coveredCount = 0;
    int minFret = 99, maxFret = 0;
    int lowestSoundingPitch = 999;
    int openStringCount = 0;
    int firstSounding = -1, lastSounding = -1;

    for (int s = 0; s < NUM_STRINGS; ++s)
    {
        if (voicing[static_cast<size_t> (s)].pitch >= 0)
        {
            int pc = voicing[static_cast<size_t> (s)].pitch % 12;
            if (! coveredClasses[pc])
            {
                coveredClasses[pc] = true;
                ++coveredCount;
            }
            ++soundingCount;

            if (voicing[static_cast<size_t> (s)].fret > 0)
            {
                minFret = std::min (minFret, voicing[static_cast<size_t> (s)].fret);
                maxFret = std::max (maxFret, voicing[static_cast<size_t> (s)].fret);
            }
            else
            {
                ++openStringCount;
            }

            if (voicing[static_cast<size_t> (s)].pitch < lowestSoundingPitch)
                lowestSoundingPitch = voicing[static_cast<size_t> (s)].pitch;

            if (firstSounding < 0) firstSounding = s;
            lastSounding = s;
        }
    }

    if (soundingCount == 0) return -10000;

    // All pitch classes covered?
    bool allCovered = true;
    for (auto pc : pitchClasses)
    {
        if (! coveredClasses[pc])
        {
            allCovered = false;
            break;
        }
    }

    if (allCovered)
        score += SCORE_ALL_PITCHCLASSES;
    else
        score += static_cast<int> (std::round (
            static_cast<double> (coveredCount) / static_cast<double> (pitchClasses.size())
            * SCORE_ALL_PITCHCLASSES * 0.5));

    score += soundingCount * SCORE_PER_SOUNDING;

    if ((lowestSoundingPitch % 12) == rootPitchClass)
        score += SCORE_ROOT_IN_BASS;

    if (minFret <= maxFret && minFret < 99)
    {
        int span = maxFret - minFret;
        score += (4 - span) * SCORE_SMALL_SPAN;
    }

    // No inner mutes
    bool hasInnerMute = false;
    for (int s = firstSounding + 1; s < lastSounding; ++s)
    {
        if (voicing[static_cast<size_t> (s)].pitch < 0)
        {
            hasInnerMute = true;
            break;
        }
    }
    if (! hasInnerMute)
        score += SCORE_NO_INNER_MUTE;

    if (preferOpen)
        score += openStringCount * SCORE_OPEN_STRING_BONUS;

    return score;
}

std::string GuitarVoicer::makeCacheKey (const std::vector<int>& pitchClasses, int position,
                                        const std::array<int, NUM_STRINGS>& openPitches)
{
    std::string key;
    for (size_t i = 0; i < pitchClasses.size(); ++i)
    {
        if (i > 0) key += ',';
        key += std::to_string (pitchClasses[i]);
    }
    key += '@';
    key += std::to_string (position);
    // Include open pitches (encodes tuning + capo) so cache invalidates on change
    key += '#';
    for (size_t i = 0; i < openPitches.size(); ++i)
    {
        if (i > 0) key += ',';
        key += std::to_string (openPitches[i]);
    }
    return key;
}

VoicingResult GuitarVoicer::findBestVoicing (const std::vector<int>& pitchClasses,
                                              int rootPitchClass,
                                              int position,
                                              const VoicingParams& params)
{
    auto cacheKey = makeCacheKey (pitchClasses, position, params.openPitches);
    auto it = voicingCache.find (cacheKey);
    if (it != voicingCache.end())
        return it->second;

    int lowFret = position;
    int highFret = std::min (position + params.fretSpan - 1, params.maxFret);

    // Build candidates per string
    std::array<std::vector<StringNote>, NUM_STRINGS> candidates;

    for (int s = 0; s < NUM_STRINGS; ++s)
    {
        auto& opts = candidates[static_cast<size_t> (s)];
        opts.push_back ({ -1, -1 }); // mute option

        int openPitch = params.openPitches[static_cast<size_t> (s)];

        // Open string
        for (auto pc : pitchClasses)
        {
            if ((openPitch % 12) == pc)
            {
                opts.push_back ({ openPitch, 0 });
                break;
            }
        }

        // Fretted notes in range
        int startFret = std::max (1, lowFret);
        for (int f = startFret; f <= highFret; ++f)
        {
            int frettedPitch = openPitch + f;
            for (auto pc : pitchClasses)
            {
                if ((frettedPitch % 12) == pc)
                {
                    opts.push_back ({ frettedPitch, f });
                    break;
                }
            }
        }
    }

    // Exhaustive 6-string search
    int bestScore = -10000;
    std::array<StringNote, NUM_STRINGS> bestVoicing {};
    bool found = false;

    for (size_t i0 = 0; i0 < candidates[0].size(); ++i0)
    {
        for (size_t i1 = 0; i1 < candidates[1].size(); ++i1)
        {
            for (size_t i2 = 0; i2 < candidates[2].size(); ++i2)
            {
                for (size_t i3 = 0; i3 < candidates[3].size(); ++i3)
                {
                    for (size_t i4 = 0; i4 < candidates[4].size(); ++i4)
                    {
                        for (size_t i5 = 0; i5 < candidates[5].size(); ++i5)
                        {
                            std::array<StringNote, NUM_STRINGS> v = {{
                                candidates[0][i0], candidates[1][i1],
                                candidates[2][i2], candidates[3][i3],
                                candidates[4][i4], candidates[5][i5]
                            }};

                            int sc = scoreVoicing (v, pitchClasses, rootPitchClass, params.preferOpen);
                            if (sc > bestScore)
                            {
                                bestScore = sc;
                                bestVoicing = v;
                                found = true;
                            }
                        }
                    }
                }
            }
        }
    }

    VoicingResult result;
    if (found)
    {
        result.voicing = bestVoicing;
        result.score = bestScore;
    }

    voicingCache[cacheKey] = result;
    return result;
}

VoicingResult GuitarVoicer::findBestPosition (const std::vector<int>& pitchClasses,
                                               int rootPitchClass,
                                               const VoicingParams& params,
                                               int ccPositionOverride)
{
    // CC override: use that exact position
    if (ccPositionOverride >= 0)
    {
        auto result = findBestVoicing (pitchClasses, rootPitchClass, ccPositionOverride, params);
        currentPosition = ccPositionOverride;
        return result;
    }

    int referencePos = params.initialPosition;

    std::vector<int> positionsToTry = { 0 }; // always try open position
    int maxStartFret = params.maxFret - params.fretSpan + 1;
    int lo = std::max (0, referencePos - params.searchRange);
    int hi = std::min (maxStartFret, referencePos + params.searchRange);

    for (int p = lo; p <= hi; ++p)
    {
        if (p != 0)
            positionsToTry.push_back (p);
    }

    VoicingResult bestResult;
    int bestCombinedScore = -10000;
    int bestPos = referencePos;

    for (auto pos : positionsToTry)
    {
        auto result = findBestVoicing (pitchClasses, rootPitchClass, pos, params);
        if (result.score <= -10000) continue;

        int distance = std::abs (pos - referencePos);
        int proximityBonus = SCORE_PROXIMITY * std::max (0, params.searchRange - distance);
        if (pos == 0 && referencePos != 0)
            proximityBonus = 0;

        int combinedScore = result.score + proximityBonus;
        if (combinedScore > bestCombinedScore)
        {
            bestCombinedScore = combinedScore;
            bestResult = result;
            bestPos = pos;
        }
    }

    currentPosition = bestPos;
    return bestResult;
}
