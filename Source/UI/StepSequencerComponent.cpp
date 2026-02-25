#include "StepSequencerComponent.h"
#include "CustomLookAndFeel.h"

StepSequencerComponent::StepSequencerComponent (juce::AudioProcessorValueTreeState& a,
                                                  std::atomic<int>& currentStepRef)
    : apvts (a), currentStep (currentStepRef)
{
    startTimerHz (30);
}

StepSequencerComponent::~StepSequencerComponent()
{
    stopTimer();
}

juce::Rectangle<int> StepSequencerComponent::getBarBounds (int step) const
{
    auto area = getLocalBounds().reduced (4, 20);
    auto barWidth = area.getWidth() / StepSequencer::STEP_COUNT;
    auto gap = 2;

    return juce::Rectangle<int> (
        area.getX() + step * barWidth + gap,
        area.getY(),
        barWidth - gap * 2,
        area.getHeight());
}

int StepSequencerComponent::getStepAtPosition (juce::Point<int> pos) const
{
    auto area = getLocalBounds().reduced (4, 20);
    auto barWidth = area.getWidth() / StepSequencer::STEP_COUNT;

    int step = (pos.getX() - area.getX()) / barWidth;
    return std::clamp (step, 0, StepSequencer::STEP_COUNT - 1);
}

void StepSequencerComponent::setStepFromMouse (const juce::MouseEvent& event)
{
    int step = getStepAtPosition (event.getPosition());
    auto barBounds = getBarBounds (step);

    // Map Y position to velocity (top = 127, bottom = 0)
    float normalised = 1.0f - static_cast<float> (event.getPosition().getY() - barBounds.getY())
                             / static_cast<float> (barBounds.getHeight());
    normalised = std::clamp (normalised, 0.0f, 1.0f);
    float velocity = std::round (normalised * 127.0f);

    auto paramId = "step" + juce::String (step + 1);
    if (auto* param = apvts.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (velocity));

    repaint();
}

void StepSequencerComponent::mouseDown (const juce::MouseEvent& event)
{
    setStepFromMouse (event);
}

void StepSequencerComponent::mouseDrag (const juce::MouseEvent& event)
{
    setStepFromMouse (event);
}

void StepSequencerComponent::timerCallback()
{
    int step = currentStep.load();
    if (step != lastDisplayedStep)
    {
        lastDisplayedStep = step;
        repaint();
    }
}

void StepSequencerComponent::paint (juce::Graphics& g)
{
    g.fillAll (CustomLookAndFeel::bgDark);

    int activeStep = currentStep.load();

    for (int i = 0; i < StepSequencer::STEP_COUNT; ++i)
    {
        auto bounds = getBarBounds (i);

        // Background
        g.setColour (i == activeStep ? CustomLookAndFeel::bgLight : CustomLookAndFeel::sliderTrack);
        g.fillRoundedRectangle (bounds.toFloat(), 3.0f);

        // Velocity bar
        auto paramId = "step" + juce::String (i + 1);
        float velocity = 0.0f;
        if (auto* param = apvts.getRawParameterValue (paramId))
            velocity = param->load();

        float normalised = velocity / 127.0f;
        if (normalised > 0.0f)
        {
            auto filledHeight = static_cast<int> (bounds.getHeight() * normalised);
            auto filledBounds = juce::Rectangle<int> (
                bounds.getX(),
                bounds.getBottom() - filledHeight,
                bounds.getWidth(),
                filledHeight);

            auto barColour = (velocity <= 0.0f) ? CustomLookAndFeel::stepGhost
                           : (i == activeStep)  ? CustomLookAndFeel::accentBright
                                                : CustomLookAndFeel::stepActive;
            g.setColour (barColour);
            g.fillRoundedRectangle (filledBounds.toFloat(), 3.0f);
        }

        // Active step highlight border
        if (i == activeStep)
        {
            g.setColour (CustomLookAndFeel::accentBright);
            g.drawRoundedRectangle (bounds.toFloat(), 3.0f, 2.0f);
        }
    }

    // Direction arrows below bars
    auto area = getLocalBounds().reduced (4, 0);
    auto barWidth = area.getWidth() / StepSequencer::STEP_COUNT;
    auto arrowY = getLocalBounds().getBottom() - 16;

    g.setFont (12.0f);
    for (int i = 0; i < StepSequencer::STEP_COUNT; ++i)
    {
        auto arrowX = area.getX() + i * barWidth;
        bool isDown = (i % 2 == 0);

        g.setColour (i == activeStep ? CustomLookAndFeel::accentBright : CustomLookAndFeel::textSecondary);
        g.drawText (isDown ? juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xbc"))
                           : juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb2")),
                    arrowX, arrowY, barWidth, 14, juce::Justification::centred);
    }
}

void StepSequencerComponent::resized()
{
}
