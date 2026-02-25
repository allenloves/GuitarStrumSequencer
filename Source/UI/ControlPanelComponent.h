#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "FretboardComponent.h"

class GuitarStrumSequencerProcessor;

class ControlPanelComponent : public juce::Component
{
public:
    ControlPanelComponent (juce::AudioProcessorValueTreeState& apvts,
                           GuitarStrumSequencerProcessor& processor);
    ~ControlPanelComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    // Strum controls
    juce::Slider strumSpeedSlider;
    juce::Slider humanizeSlider;
    juce::Label strumSpeedLabel;
    juce::Label humanizeLabel;

    // Voicing controls
    juce::ToggleButton voicingToggle;
    juce::ComboBox tuningBox;
    juce::Slider capoSlider;
    juce::Slider positionSlider;
    juce::Slider maxFretSlider;
    juce::Slider fretSpanSlider;
    juce::ToggleButton preferOpenToggle;
    juce::ComboBox positionCCBox;
    juce::Slider searchRangeSlider;
    juce::ToggleButton multiChannelToggle;

    juce::Label tuningLabel;
    juce::Label capoLabel;
    juce::Label positionLabel;
    juce::Label maxFretLabel;
    juce::Label fretSpanLabel;
    juce::Label positionCCLabel;
    juce::Label searchRangeLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strumSpeedAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> humanizeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> voicingAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tuningAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> capoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> positionAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxFretAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fretSpanAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preferOpenAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> positionCCAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> searchRangeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> multiChannelAttach;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text,
                      const juce::String& suffix = "");
    void setupComboBox (juce::ComboBox& box, juce::Label& label, const juce::String& text,
                        const juce::StringArray& items);

    FretboardComponent fretboardComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanelComponent)
};
