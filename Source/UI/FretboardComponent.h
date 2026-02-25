#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class GuitarStrumSequencerProcessor;

class FretboardComponent : public juce::Component,
                           private juce::Timer
{
public:
    explicit FretboardComponent (GuitarStrumSequencerProcessor& processor);
    ~FretboardComponent() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    GuitarStrumSequencerProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FretboardComponent)
};
