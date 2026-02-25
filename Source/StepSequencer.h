#pragma once

#include <array>
#include <vector>
#include <functional>

class StepSequencer
{
public:
    static constexpr int STEP_COUNT = 16;
    static constexpr double SUBDIV_8TH  = 0.5;
    static constexpr double SUBDIV_16TH = 0.25;

    StepSequencer();

    // Step velocity management
    void setStepVelocity (int step, float velocity);
    float getStepVelocity (int step) const;

    // Transport-driven step scanning
    struct StepEvent
    {
        double beatPosition;
        int stepIndex;
        float velocity;
    };

    // Scan for step events within a block, returns list of triggered steps
    std::vector<StepEvent> processBlock (double blockStartBeat,
                                          double blockEndBeat,
                                          bool isPlaying,
                                          bool isCycling,
                                          double cycleStart,
                                          double cycleEnd,
                                          int subdivisionIndex);

    void reset();
    int getCurrentStep() const { return currentStep; }

private:
    std::array<float, STEP_COUNT> stepVelocities {};
    int currentStep = -1;
    double nextStepBeat = -1.0;
    double previousBlockEnd = -1.0;

    static constexpr int MAX_STEPS_PER_BLOCK = 32;

    static double getStepDuration (int subdivisionIndex);
    static double quantizeToStep (double beat, double stepDuration);
    static int calculateStepIndex (double beat, double stepDuration);
};
