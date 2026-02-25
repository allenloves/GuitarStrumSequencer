#include "StrumEngine.h"
#include <cmath>

StrumEngine::StrumEngine()
    : rng (std::random_device{}())
{
}

int StrumEngine::clamp (int value, int minVal, int maxVal)
{
    return std::min (maxVal, std::max (minVal, value));
}

std::vector<StrumNote> StrumEngine::generateStrum (const std::vector<int>& notesToStrum,
                                                    StepDirection direction,
                                                    float velocity,
                                                    float strumSpeedMs,
                                                    float humanizeAmount,
                                                    bool multiChannel,
                                                    double tempo)
{
    std::vector<StrumNote> result;
    if (notesToStrum.empty() || velocity <= 0.0f)
        return result;

    double msPerBeat = 60000.0 / tempo;

    bool isDownStrum = (direction == StepDirection::Down);

    std::vector<int> ordered = notesToStrum;
    if (! isDownStrum)
        std::reverse (ordered.begin(), ordered.end());

    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);

    // Global step timing offset: shift the entire strum slightly early/late (±15ms at full)
    double globalOffsetMs = 0.0;
    if (humanizeAmount > 0.0f)
    {
        globalOffsetMs = dist (rng) * 15.0 * humanizeAmount;
        if (globalOffsetMs < 0.0) globalOffsetMs = 0.0; // only delay, never early
    }

    std::vector<ActiveNote> newActiveNotes;

    for (size_t i = 0; i < ordered.size(); ++i)
    {
        StrumNote note;
        note.pitch = ordered[i];
        note.channel = multiChannel ? (static_cast<int> (i) % 6 + 1) : 1;

        // Velocity with humanization (±30 at full)
        int velVariation = 0;
        if (humanizeAmount > 0.0f)
            velVariation = static_cast<int> (std::round (dist (rng) * 30.0f * humanizeAmount));

        note.velocity = clamp (static_cast<int> (velocity) + velVariation, 1, 127);

        // Strum delay in ms
        double delayMs = globalOffsetMs + static_cast<double> (i) * strumSpeedMs;

        // Per-note timing humanization (±10ms at full)
        if (humanizeAmount > 0.0f && i > 0)
        {
            delayMs += dist (rng) * 10.0 * humanizeAmount;
            if (delayMs < 0.0) delayMs = 0.0;
        }

        // Convert ms to beat offset
        note.beatOffset = delayMs / msPerBeat;

        result.push_back (note);
        newActiveNotes.push_back ({ note.pitch, note.channel });
    }

    activeNotes = std::move (newActiveNotes);
    return result;
}
