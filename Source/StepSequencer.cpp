#include "StepSequencer.h"
#include <cmath>

StepSequencer::StepSequencer()
{
    reset();
}

void StepSequencer::setStepVelocity (int step, float velocity)
{
    if (step >= 0 && step < STEP_COUNT)
        stepVelocities[static_cast<size_t> (step)] = velocity;
}

float StepSequencer::getStepVelocity (int step) const
{
    if (step >= 0 && step < STEP_COUNT)
        return stepVelocities[static_cast<size_t> (step)];
    return 0.0f;
}

double StepSequencer::getStepDuration (int subdivisionIndex)
{
    return (subdivisionIndex == 0) ? SUBDIV_8TH : SUBDIV_16TH;
}

double StepSequencer::quantizeToStep (double beat, double stepDuration)
{
    // Use floor to land on the step boundary at-or-just-before the beat.
    // Small epsilon handles floating-point imprecision at exact boundaries
    // (e.g. beat -0.0000001 should resolve to 0.0, not -stepDuration).
    double result = std::floor ((beat + 1e-6) / stepDuration) * stepDuration;

    // If we're past the midpoint of a step, the current step was already
    // handled — target the next boundary instead.
    if (beat - result > stepDuration * 0.5)
        result += stepDuration;

    return result;
}

int StepSequencer::calculateStepIndex (double beat, double stepDuration)
{
    double patternLength = STEP_COUNT * stepDuration;
    double pos = std::fmod (beat, patternLength);
    if (pos < 0.0) pos += patternLength;
    return static_cast<int> (std::floor (pos / stepDuration)) % STEP_COUNT;
}

std::vector<StepSequencer::StepEvent> StepSequencer::processBlock (
    double blockStartBeat,
    double blockEndBeat,
    bool isPlaying,
    bool isCycling,
    double cycleStart,
    double cycleEnd,
    int subdivisionIndex)
{
    std::vector<StepEvent> events;

    if (! isPlaying)
    {
        previousBlockEnd = -1.0;
        return events;
    }

    double stepDuration = getStepDuration (subdivisionIndex);

    // Detect transport jump (user seek, etc.) and reinitialize if needed.
    // After a cycle wrap, nextStepBeat is already set correctly by the wrap
    // logic below, so we must NOT blindly reinit on every backward jump.
    bool needsReinit = (nextStepBeat < 0.0);

    if (! needsReinit && previousBlockEnd > 0.0
        && blockStartBeat < previousBlockEnd - stepDuration)
    {
        // Backward jump detected — only reinit if nextStepBeat is unreasonable
        // (cycle wrap already positioned it correctly near cycleStart)
        if (nextStepBeat > blockEndBeat + stepDuration * STEP_COUNT)
            needsReinit = true;
    }

    // Forward jump: nextStepBeat is far behind the current position
    if (! needsReinit && nextStepBeat < blockStartBeat - stepDuration)
        needsReinit = true;

    // nextStepBeat way ahead (shouldn't happen, but guard)
    if (! needsReinit && nextStepBeat > blockEndBeat + stepDuration * STEP_COUNT)
        needsReinit = true;

    previousBlockEnd = blockEndBeat;

    if (needsReinit)
    {
        nextStepBeat = quantizeToStep (blockStartBeat, stepDuration);
        currentStep = calculateStepIndex (nextStepBeat, stepDuration);
    }

    // Validate cycle parameters — only use cycling if the range is sensible
    bool validCycle = isCycling && (cycleEnd > cycleStart + stepDuration * 0.5);

    // Scan for step boundaries within this block (with safety limit).
    // No ">= blockStartBeat" check: after floor-based quantization or cycle
    // wrap, nextStepBeat may be up to 1 step before blockStartBeat, and we
    // still want to emit that event (it gets placed at sample 0 by the caller).
    int iterations = 0;
    while (nextStepBeat < blockEndBeat && iterations < MAX_STEPS_PER_BLOCK)
    {
        ++iterations;

        // Use the unwrapped beat position for scheduling.  The block's beat
        // range is in absolute (unwrapped) space, so the event must match.
        // processBlock handles cycle normalisation for any pending events that
        // end up past cycleEnd (e.g. strum notes that spread across the boundary).
        float vel = stepVelocities[static_cast<size_t> (currentStep)];

        events.push_back ({ nextStepBeat, currentStep, vel });

        // Advance
        nextStepBeat += stepDuration;
        currentStep = (currentStep + 1) % STEP_COUNT;

        // Stop scanning once we've passed the cycle end.  Don't wrap
        // nextStepBeat here — the transport will handle the actual position
        // change, and reinit will pick up the new position.
        if (validCycle && nextStepBeat >= cycleEnd)
            break;
    }

    return events;
}

void StepSequencer::reset()
{
    currentStep = -1;
    nextStepBeat = -1.0;
    previousBlockEnd = -1.0;
}
