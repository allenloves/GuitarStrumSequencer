#include "ControlPanelComponent.h"
#include "CustomLookAndFeel.h"
#include "../PluginProcessor.h"

ControlPanelComponent::ControlPanelComponent (juce::AudioProcessorValueTreeState& a,
                                                GuitarStrumSequencerProcessor& processor)
    : apvts (a),
      fretboardComp (processor)
{
    addAndMakeVisible (fretboardComp);

    // Strum controls
    setupSlider (strumSpeedSlider, strumSpeedLabel, "Speed", " ms");
    strumSpeedSlider.setRange (5, 50, 1);
    strumSpeedAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "strumSpeed", strumSpeedSlider);

    setupSlider (humanizeSlider, humanizeLabel, "Humanize", "%");
    humanizeSlider.setRange (0, 100, 1);
    humanizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "humanize", humanizeSlider);

    // Voicing toggle
    voicingToggle.setButtonText ("Enable Guitar Voicing");
    addAndMakeVisible (voicingToggle);
    voicingAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "guitarVoicing", voicingToggle);

    // Tuning
    setupComboBox (tuningBox, tuningLabel, "Tuning",
                   { "Standard", "Drop D", "Open G", "DADGAD", "Half Step Down" });
    tuningAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "tuning", tuningBox);

    // Capo
    setupSlider (capoSlider, capoLabel, "Capo", "");
    capoSlider.setRange (0, 12, 1);
    capoAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "capo", capoSlider);

    // Position
    setupSlider (positionSlider, positionLabel, "Position", "");
    positionSlider.setRange (0, 12, 1);
    positionAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "initialPosition", positionSlider);

    // Max Fret
    setupSlider (maxFretSlider, maxFretLabel, "Max Fret", "");
    maxFretSlider.setRange (5, 15, 1);
    maxFretAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "maxFret", maxFretSlider);

    // Fret Span
    setupSlider (fretSpanSlider, fretSpanLabel, "Fret Span", "");
    fretSpanSlider.setRange (3, 5, 1);
    fretSpanAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "fretSpan", fretSpanSlider);

    // Prefer Open
    preferOpenToggle.setButtonText ("Prefer Open Strings");
    addAndMakeVisible (preferOpenToggle);
    preferOpenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "preferOpenStrings", preferOpenToggle);

    // Position CC
    setupComboBox (positionCCBox, positionCCLabel, "Position CC",
                   { "CC 85", "CC 86", "CC 87", "CC 102", "CC 103", "CC 104", "CC 105", "CC 106" });
    positionCCAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "positionCC", positionCCBox);

    // Search Range
    setupSlider (searchRangeSlider, searchRangeLabel, "Search Range", "");
    searchRangeSlider.setRange (2, 7, 1);
    searchRangeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "searchRange", searchRangeSlider);

    // Multi-Channel
    multiChannelToggle.setButtonText ("Multi-Channel Output");
    addAndMakeVisible (multiChannelToggle);
    multiChannelAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "multiChannel", multiChannelToggle);
}

ControlPanelComponent::~ControlPanelComponent() = default;

void ControlPanelComponent::setupSlider (juce::Slider& slider, juce::Label& label,
                                          const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    if (suffix.isNotEmpty())
        slider.setTextValueSuffix (suffix);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

void ControlPanelComponent::setupComboBox (juce::ComboBox& box, juce::Label& label,
                                            const juce::String& text, const juce::StringArray& items)
{
    box.addItemList (items, 1);
    addAndMakeVisible (box);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

void ControlPanelComponent::paint (juce::Graphics& g)
{
    g.fillAll (CustomLookAndFeel::bgDark);

    auto bounds = getLocalBounds();
    auto halfWidth = bounds.getWidth() / 2;

    // Left panel header: STRUM
    g.setColour (CustomLookAndFeel::accent);
    g.setFont (juce::Font (14.0f).boldened());
    g.drawText ("STRUM", 10, 4, halfWidth - 20, 20, juce::Justification::centredLeft);

    // Right panel header: GUITAR VOICING
    g.drawText ("GUITAR VOICING", halfWidth + 10, 4, halfWidth - 20, 20, juce::Justification::centredLeft);

    // Divider line
    g.setColour (CustomLookAndFeel::bgLight);
    g.drawVerticalLine (halfWidth, 0.0f, static_cast<float> (bounds.getHeight()));
}

void ControlPanelComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    auto halfWidth = bounds.getWidth() / 2;
    auto rowHeight = 28;
    auto labelWidth = 85;
    auto startY = 28;

    // === Left panel: Strum ===
    auto leftArea = bounds.withWidth (halfWidth - 10);

    strumSpeedLabel.setBounds (leftArea.getX(), startY, labelWidth, rowHeight);
    strumSpeedSlider.setBounds (leftArea.getX() + labelWidth, startY,
                                leftArea.getWidth() - labelWidth, rowHeight);

    humanizeLabel.setBounds (leftArea.getX(), startY + rowHeight + 4, labelWidth, rowHeight);
    humanizeSlider.setBounds (leftArea.getX() + labelWidth, startY + rowHeight + 4,
                              leftArea.getWidth() - labelWidth, rowHeight);

    // Fretboard diagram below humanize
    int fretboardY = startY + (rowHeight + 4) * 2;
    int fretboardHeight = bounds.getHeight() - fretboardY;
    if (fretboardHeight > 20)
        fretboardComp.setBounds (leftArea.getX(), fretboardY,
                                 leftArea.getWidth(), fretboardHeight);

    // === Right panel: Guitar Voicing ===
    auto rightArea = bounds.withTrimmedLeft (halfWidth + 10);
    int ry = startY;

    voicingToggle.setBounds (rightArea.getX(), ry, rightArea.getWidth(), rowHeight);
    ry += rowHeight + 4;

    tuningLabel.setBounds (rightArea.getX(), ry, labelWidth, rowHeight);
    tuningBox.setBounds (rightArea.getX() + labelWidth, ry,
                         rightArea.getWidth() - labelWidth, rowHeight);
    ry += rowHeight + 4;

    capoLabel.setBounds (rightArea.getX(), ry, labelWidth, rowHeight);
    capoSlider.setBounds (rightArea.getX() + labelWidth, ry,
                          rightArea.getWidth() - labelWidth, rowHeight);
    ry += rowHeight + 4;

    positionLabel.setBounds (rightArea.getX(), ry, labelWidth, rowHeight);
    positionSlider.setBounds (rightArea.getX() + labelWidth, ry,
                              rightArea.getWidth() - labelWidth, rowHeight);
    ry += rowHeight + 4;

    // Max Fret and Fret Span side by side
    auto subWidth = (rightArea.getWidth() - 10) / 2;
    maxFretLabel.setBounds (rightArea.getX(), ry, 60, rowHeight);
    maxFretSlider.setBounds (rightArea.getX() + 60, ry, subWidth - 60, rowHeight);
    fretSpanLabel.setBounds (rightArea.getX() + subWidth + 10, ry, 60, rowHeight);
    fretSpanSlider.setBounds (rightArea.getX() + subWidth + 70, ry, subWidth - 60, rowHeight);
    ry += rowHeight + 4;

    preferOpenToggle.setBounds (rightArea.getX(), ry, rightArea.getWidth(), rowHeight);
    ry += rowHeight + 4;

    positionCCLabel.setBounds (rightArea.getX(), ry, labelWidth, rowHeight);
    positionCCBox.setBounds (rightArea.getX() + labelWidth, ry,
                             rightArea.getWidth() - labelWidth, rowHeight);
    ry += rowHeight + 4;

    searchRangeLabel.setBounds (rightArea.getX(), ry, labelWidth, rowHeight);
    searchRangeSlider.setBounds (rightArea.getX() + labelWidth, ry,
                                 rightArea.getWidth() - labelWidth, rowHeight);
    ry += rowHeight + 4;

    multiChannelToggle.setBounds (rightArea.getX(), ry, rightArea.getWidth(), rowHeight);
}
