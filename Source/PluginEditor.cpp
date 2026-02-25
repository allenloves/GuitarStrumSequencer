#include "PluginEditor.h"

GuitarStrumSequencerEditor::GuitarStrumSequencerEditor (GuitarStrumSequencerProcessor& p)
    : AudioProcessorEditor (p),
      processorRef (p),
      stepSequencerComp (p.getAPVTS(), p.currentStepForUI),
      controlPanelComp (p.getAPVTS())
{
    setLookAndFeel (&customLookAndFeel);
    setSize (700, 500);

    // Subdivision selector in header
    subdivisionBox.addItemList ({ "8th Notes", "16th Notes" }, 1);
    addAndMakeVisible (subdivisionBox);
    subdivisionAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        p.getAPVTS(), "subdivision", subdivisionBox);

    addAndMakeVisible (stepSequencerComp);
    addAndMakeVisible (controlPanelComp);
}

GuitarStrumSequencerEditor::~GuitarStrumSequencerEditor()
{
    setLookAndFeel (nullptr);
}

void GuitarStrumSequencerEditor::paint (juce::Graphics& g)
{
    g.fillAll (CustomLookAndFeel::bgDark);

    // Header bar
    auto headerBounds = getLocalBounds().removeFromTop (40);
    g.setColour (CustomLookAndFeel::bgMedium);
    g.fillRect (headerBounds);

    g.setColour (CustomLookAndFeel::textPrimary);
    g.setFont (juce::Font (18.0f).boldened());
    g.drawText ("GUITAR STRUM SEQUENCER", headerBounds.reduced (12, 0),
                juce::Justification::centredLeft);

    // Section divider
    auto seqBottom = 40 + 220;
    g.setColour (CustomLookAndFeel::bgLight);
    g.drawHorizontalLine (seqBottom, 0.0f, static_cast<float> (getWidth()));
}

void GuitarStrumSequencerEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header
    auto header = bounds.removeFromTop (40);
    subdivisionBox.setBounds (header.removeFromRight (140).reduced (10, 6));

    // Step sequencer area
    auto seqArea = bounds.removeFromTop (220);
    stepSequencerComp.setBounds (seqArea);

    // Control panel takes the rest
    controlPanelComp.setBounds (bounds);
}
