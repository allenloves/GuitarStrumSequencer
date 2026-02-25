#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();

    // Colours
    static inline const juce::Colour bgDark       { 0xff1a1a2e };
    static inline const juce::Colour bgMedium     { 0xff16213e };
    static inline const juce::Colour bgLight      { 0xff0f3460 };
    static inline const juce::Colour accent        { 0xffe94560 };
    static inline const juce::Colour accentBright  { 0xffff6b6b };
    static inline const juce::Colour textPrimary   { 0xffeaeaea };
    static inline const juce::Colour textSecondary { 0xff8a8a9a };
    static inline const juce::Colour sliderTrack   { 0xff2a2a4a };
    static inline const juce::Colour stepActive    { 0xff4ecdc4 };
    static inline const juce::Colour stepGhost     { 0xff3a3a5a };

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
};
