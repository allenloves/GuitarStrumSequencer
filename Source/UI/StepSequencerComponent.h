#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../StepSequencer.h"

class StepSequencerComponent : public juce::Component,
                                public juce::Timer
{
public:
    StepSequencerComponent (juce::AudioProcessorValueTreeState& apvts,
                            std::atomic<int>& currentStepRef);
    ~StepSequencerComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::atomic<int>& currentStep;

    int lastDisplayedStep = -1;

    juce::Rectangle<int> getBarBounds (int step) const;
    int getStepAtPosition (juce::Point<int> pos) const;
    void setStepFromMouse (const juce::MouseEvent& event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencerComponent)
};
