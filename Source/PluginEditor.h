#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/StepSequencerComponent.h"
#include "UI/ControlPanelComponent.h"

class GuitarStrumSequencerEditor : public juce::AudioProcessorEditor
{
public:
    explicit GuitarStrumSequencerEditor (GuitarStrumSequencerProcessor& processor);
    ~GuitarStrumSequencerEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    GuitarStrumSequencerProcessor& processorRef;
    CustomLookAndFeel customLookAndFeel;

    // Header
    juce::ComboBox subdivisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subdivisionAttach;

    // Sections
    StepSequencerComponent stepSequencerComp;
    ControlPanelComponent controlPanelComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarStrumSequencerEditor)
};
