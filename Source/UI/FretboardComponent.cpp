#include "FretboardComponent.h"
#include "CustomLookAndFeel.h"
#include "../PluginProcessor.h"
#include "../GuitarVoicer.h"

FretboardComponent::FretboardComponent (GuitarStrumSequencerProcessor& processor)
    : processorRef (processor)
{
    startTimerHz (10);
}

FretboardComponent::~FretboardComponent()
{
    stopTimer();
}

void FretboardComponent::timerCallback()
{
    repaint();
}

void FretboardComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    if (bounds.getWidth() < 10 || bounds.getHeight() < 10)
        return;

    // Check if voicing is enabled
    auto& apvts = processorRef.getAPVTS();
    bool voicingEnabled = apvts.getRawParameterValue ("guitarVoicing")->load() >= 0.5f;

    bool hasData = voicingEnabled && processorRef.isVoicingForUIValid();
    VoicingResult voicing;
    if (hasData)
        voicing = processorRef.getVoicingForUI();

    int capo = static_cast<int> (apvts.getRawParameterValue ("capo")->load());

    // Horizontal layout: strings are horizontal, frets are vertical
    // Top = high e, Bottom = low E (guitar player's perspective)
    constexpr int numStrings = 6;
    constexpr int maxFrets = 7;

    const float leftMargin = 18.0f;   // string name labels
    const float oxWidth = 14.0f;      // O/X indicator column
    const float rightMargin = 6.0f;
    const float topMargin = 14.0f;    // fret number labels
    const float bottomMargin = 4.0f;

    const float fretboardLeft = leftMargin + oxWidth;
    const float fretboardWidth = bounds.getWidth() - fretboardLeft - rightMargin;
    const float fretboardHeight = bounds.getHeight() - topMargin - bottomMargin;
    const float stringSpacing = fretboardHeight / (numStrings - 1);

    // Determine fret range to display (voicing frets are relative to capo)
    int startFret = 0;
    int numFrets = maxFrets;

    if (hasData)
    {
        int minFret = 99, maxFret = 0;
        bool hasFrettedNote = false;
        bool hasOpenString = false;

        for (int s = 0; s < numStrings; ++s)
        {
            auto& sn = voicing.voicing[static_cast<size_t> (s)];
            if (sn.pitch >= 0)
            {
                if (sn.fret == 0)
                    hasOpenString = true;
                else if (sn.fret > 0)
                {
                    hasFrettedNote = true;
                    minFret = std::min (minFret, sn.fret);
                    maxFret = std::max (maxFret, sn.fret);
                }
            }
        }

        if (hasFrettedNote)
        {
            // Adaptive fret count: show just enough frets to frame the notes
            int effectiveMin = hasOpenString ? 0 : minFret;
            int range = maxFret - effectiveMin;
            numFrets = std::clamp (range + 2, 4, maxFrets);

            if (hasOpenString)
            {
                // Must show nut for open strings
                startFret = 0;
                // Ensure all fretted notes are visible
                if (maxFret > numFrets)
                    numFrets = std::min (maxFret + 1, maxFrets);
            }
            else
            {
                // Center the fretted notes within the display
                int span = maxFret - minFret + 1;
                int padding = (numFrets - span) / 2;
                startFret = std::max (0, minFret - padding - 1);

                if (maxFret - startFret >= numFrets)
                    startFret = maxFret - numFrets + 1;
            }
        }
    }

    bool showNut = (startFret == 0);

    const float fretSpacing = fretboardWidth / numFrets;

    // Draw fret bars (vertical lines)
    g.setColour (CustomLookAndFeel::textSecondary.withAlpha (0.5f));
    for (int f = 0; f <= numFrets; ++f)
    {
        float x = fretboardLeft + f * fretSpacing;

        if (f == 0 && showNut)
        {
            if (capo > 0)
            {
                // Draw capo bar: rounded rectangle across all strings
                float capoWidth = 6.0f;
                g.setColour (CustomLookAndFeel::accent);
                g.fillRoundedRectangle (x - capoWidth * 0.5f, topMargin - 3.0f,
                                        capoWidth, fretboardHeight + 6.0f, 2.0f);
            }
            else
            {
                // Thick nut line on the left
                g.setColour (CustomLookAndFeel::textPrimary);
                g.fillRect (x - 1.5f, topMargin - 2.0f, 3.0f, fretboardHeight + 4.0f);
            }
            g.setColour (CustomLookAndFeel::textSecondary.withAlpha (0.5f));
        }
        else
        {
            g.drawVerticalLine (static_cast<int> (x), topMargin, topMargin + fretboardHeight);
        }
    }

    // Draw strings (horizontal lines)
    for (int s = 0; s < numStrings; ++s)
    {
        float y = topMargin + s * stringSpacing;
        g.drawHorizontalLine (static_cast<int> (y), fretboardLeft, fretboardLeft + fretboardWidth);
    }

    // String names on the left (top to bottom: e B G D A E)
    const char* stringNamesTopDown[] = { "e", "B", "G", "D", "A", "E" };
    g.setColour (CustomLookAndFeel::textSecondary);
    auto smallFont = juce::FontOptions (11.0f);
    g.setFont (smallFont);
    for (int s = 0; s < numStrings; ++s)
    {
        float y = topMargin + s * stringSpacing;
        g.drawText (stringNamesTopDown[s],
                    0, static_cast<int> (y - 7), static_cast<int> (leftMargin), 14,
                    juce::Justification::centredRight);
    }

    // Fret number labels at top (physical fret = voicing fret + capo)
    if (showNut && capo > 0)
    {
        // Show capo label above the nut position
        g.setColour (CustomLookAndFeel::accent);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText ("C" + juce::String (capo),
                    static_cast<int> (fretboardLeft - 12), 0,
                    24, static_cast<int> (topMargin),
                    juce::Justification::centred);

        // Show first physical fret number
        g.setColour (CustomLookAndFeel::textSecondary);
        g.setFont (smallFont);
        g.drawText (juce::String (capo + 1),
                    static_cast<int> (fretboardLeft + fretSpacing * 0.5f - 10), 0,
                    20, static_cast<int> (topMargin),
                    juce::Justification::centred);
    }
    else if (! showNut)
    {
        // Show physical fret number for first visible fret
        int physicalFret = startFret + capo + 1;
        g.setColour (CustomLookAndFeel::textSecondary);
        g.setFont (smallFont);
        g.drawText (juce::String (physicalFret) + "fr",
                    static_cast<int> (fretboardLeft + fretSpacing * 0.5f - 14), 0,
                    28, static_cast<int> (topMargin),
                    juce::Justification::centred);
    }

    if (! hasData)
        return;

    // Build finger assignment: group fretted notes by fret, assign fingers 1-4
    // in ascending fret order
    std::vector<int> uniqueFrets;
    for (int vi = 0; vi < numStrings; ++vi)
    {
        int fret = voicing.voicing[static_cast<size_t> (vi)].fret;
        if (voicing.voicing[static_cast<size_t> (vi)].pitch >= 0 && fret > 0)
        {
            if (std::find (uniqueFrets.begin(), uniqueFrets.end(), fret) == uniqueFrets.end())
                uniqueFrets.push_back (fret);
        }
    }
    std::sort (uniqueFrets.begin(), uniqueFrets.end());

    // Map fret → finger number (1-based)
    auto getFingerForFret = [&] (int fret) -> int
    {
        for (size_t i = 0; i < uniqueFrets.size(); ++i)
        {
            if (uniqueFrets[i] == fret)
                return static_cast<int> (i) + 1;
        }
        return 1;
    };

    // Draw finger dots, O/X indicators, and finger numbers
    auto boldFont = juce::FontOptions (12.0f);
    auto fingerFont = juce::FontOptions (10.0f).withStyle ("Bold");

    for (int vi = 0; vi < numStrings; ++vi)
    {
        int displayRow = numStrings - 1 - vi;  // flip: low E at bottom
        float y = topMargin + displayRow * stringSpacing;
        auto& sn = voicing.voicing[static_cast<size_t> (vi)];

        if (sn.pitch < 0)
        {
            // Muted: draw X
            g.setColour (CustomLookAndFeel::textSecondary);
            g.setFont (boldFont);
            g.drawText ("X",
                        static_cast<int> (leftMargin), static_cast<int> (y - 7),
                        static_cast<int> (oxWidth), 14,
                        juce::Justification::centred);
        }
        else if (sn.fret == 0)
        {
            // Open string: draw O
            g.setColour (CustomLookAndFeel::textPrimary);
            g.setFont (boldFont);
            g.drawText ("O",
                        static_cast<int> (leftMargin), static_cast<int> (y - 7),
                        static_cast<int> (oxWidth), 14,
                        juce::Justification::centred);
        }
        else
        {
            // Fretted note: filled circle with finger number
            int displayFret = sn.fret - startFret;
            if (displayFret >= 1 && displayFret <= numFrets)
            {
                float dotX = fretboardLeft + (displayFret - 0.5f) * fretSpacing;
                float dotRadius = std::min (fretSpacing, stringSpacing) * 0.32f;

                g.setColour (CustomLookAndFeel::stepActive);
                g.fillEllipse (dotX - dotRadius, y - dotRadius,
                               dotRadius * 2.0f, dotRadius * 2.0f);

                // Finger number inside dot
                int finger = getFingerForFret (sn.fret);
                if (finger >= 1 && finger <= 4)
                {
                    g.setColour (CustomLookAndFeel::bgDark);
                    g.setFont (fingerFont);
                    g.drawText (juce::String (finger),
                                static_cast<int> (dotX - dotRadius),
                                static_cast<int> (y - dotRadius),
                                static_cast<int> (dotRadius * 2.0f),
                                static_cast<int> (dotRadius * 2.0f),
                                juce::Justification::centred);
                }
            }
        }
    }
}
