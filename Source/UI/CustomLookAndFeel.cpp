#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bgDark);
    setColour (juce::ComboBox::backgroundColourId, bgMedium);
    setColour (juce::ComboBox::outlineColourId, bgLight);
    setColour (juce::ComboBox::textColourId, textPrimary);
    setColour (juce::ComboBox::arrowColourId, textSecondary);
    setColour (juce::PopupMenu::backgroundColourId, bgMedium);
    setColour (juce::PopupMenu::textColourId, textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Label::textColourId, textPrimary);
    setColour (juce::Slider::textBoxTextColourId, textPrimary);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, textPrimary);
    setColour (juce::ToggleButton::tickColourId, accent);
}

void CustomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    auto radius = std::min (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Track
    g.setColour (sliderTrack);
    juce::Path track;
    track.addCentredArc (centreX, centreY, radius - 2, radius - 2,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.strokePath (track, juce::PathStrokeType (3.0f));

    // Value arc
    g.setColour (accent);
    juce::Path valueArc;
    valueArc.addCentredArc (centreX, centreY, radius - 2, radius - 2,
                            0.0f, rotaryStartAngle, angle, true);
    g.strokePath (valueArc, juce::PathStrokeType (3.0f));

    // Thumb
    juce::Path thumb;
    auto thumbWidth = 3.0f;
    thumb.addRectangle (-thumbWidth / 2, -radius + 4, thumbWidth, radius * 0.4f);
    g.setColour (textPrimary);
    g.fillPath (thumb, juce::AffineTransform::rotation (angle).translated (centreX, centreY));
}

void CustomLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                           juce::Slider::SliderStyle style, juce::Slider&)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        auto trackY = static_cast<float> (y + height / 2);
        auto trackHeight = 4.0f;

        // Background track
        g.setColour (sliderTrack);
        g.fillRoundedRectangle (static_cast<float> (x), trackY - trackHeight / 2,
                                static_cast<float> (width), trackHeight, 2.0f);

        // Value track
        g.setColour (accent);
        g.fillRoundedRectangle (static_cast<float> (x), trackY - trackHeight / 2,
                                sliderPos - static_cast<float> (x), trackHeight, 2.0f);

        // Thumb
        g.setColour (textPrimary);
        g.fillEllipse (sliderPos - 6.0f, trackY - 6.0f, 12.0f, 12.0f);
    }
    else
    {
        // Vertical slider fallback
        auto trackX = static_cast<float> (x + width / 2);
        auto trackWidth = 4.0f;

        g.setColour (sliderTrack);
        g.fillRoundedRectangle (trackX - trackWidth / 2, static_cast<float> (y),
                                trackWidth, static_cast<float> (height), 2.0f);

        g.setColour (accent);
        g.fillRoundedRectangle (trackX - trackWidth / 2, sliderPos,
                                trackWidth, static_cast<float> (y + height) - sliderPos, 2.0f);

        g.setColour (textPrimary);
        g.fillEllipse (trackX - 6.0f, sliderPos - 6.0f, 12.0f, 12.0f);
    }
}

void CustomLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                       int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                       juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    // Arrow
    auto arrowZone = juce::Rectangle<float> (static_cast<float> (width - 20), 0.0f, 14.0f, static_cast<float> (height));
    juce::Path arrow;
    arrow.addTriangle (arrowZone.getX(), arrowZone.getCentreY() - 3.0f,
                       arrowZone.getRight(), arrowZone.getCentreY() - 3.0f,
                       arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void CustomLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto boxSize = 18.0f;
    auto boxBounds = juce::Rectangle<float> (4.0f, (bounds.getHeight() - boxSize) / 2.0f, boxSize, boxSize);

    g.setColour (button.getToggleState() ? accent : sliderTrack);
    g.fillRoundedRectangle (boxBounds, 3.0f);

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.fillRoundedRectangle (boxBounds, 3.0f);
    }

    if (button.getToggleState())
    {
        g.setColour (juce::Colours::white);
        juce::Path tick;
        tick.startNewSubPath (boxBounds.getX() + 4, boxBounds.getCentreY());
        tick.lineTo (boxBounds.getCentreX() - 1, boxBounds.getBottom() - 5);
        tick.lineTo (boxBounds.getRight() - 4, boxBounds.getY() + 5);
        g.strokePath (tick, juce::PathStrokeType (2.0f));
    }

    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.drawText (button.getButtonText(),
                boxBounds.getRight() + 6, 0,
                static_cast<int> (bounds.getWidth() - boxBounds.getRight() - 6),
                static_cast<int> (bounds.getHeight()),
                juce::Justification::centredLeft);
}

juce::Font CustomLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (14.0f);
}

juce::Font CustomLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (13.0f);
}
