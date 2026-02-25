#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GuitarVoicer.h"
#include "StepSequencer.h"
#include "StrumEngine.h"

class GuitarStrumSequencerProcessor : public juce::AudioProcessor
{
public:
    GuitarStrumSequencerProcessor();
    ~GuitarStrumSequencerProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Expose current step for GUI highlight
    std::atomic<int> currentStepForUI { -1 };

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    GuitarVoicer voicer;
    StepSequencer sequencer;
    StrumEngine strumEngine;

    // MIDI state
    std::vector<int> heldNotes;    // sorted ascending
    std::vector<int> voicedNotes;  // guitar-voiced pitches (sorted ascending)
    int ccPositionOverride = -1;
    bool ccPositionUsed = false;

    double currentSampleRate = 44100.0;
    bool wasPlaying = false;
    bool lastStepHadNoNotes = false;  // grace period for chord transitions

    // Re-trigger state: detect chord changes that arrive one buffer late
    std::vector<int> lastStrumNotes;
    int lastStepIndex = 0;
    StepDirection lastStepDirection = StepDirection::Down;
    float lastStepVelocity = 0.0f;
    double lastStepBeat = -1.0;

    // Beat-based pending MIDI event queue (for cross-buffer strum scheduling)
    struct PendingMidiEvent
    {
        juce::MidiMessage message;
        double beatPosition;
    };
    std::vector<PendingMidiEvent> pendingEvents;

    void insertSorted (std::vector<int>& arr, int pitch);
    void removeFromArray (std::vector<int>& arr, int pitch);
    void updateVoicedNotes();
    std::vector<int> getNotesToStrum();
    void scheduleEvent (const juce::MidiMessage& msg, double beatPos);
    void emitPendingEvents (juce::MidiBuffer& buffer, double blockStartBeat,
                            double blockEndBeat, double beatsPerSample, int numSamples);
    void killActiveNotesAt (double beatPos);

    VoicingParams readVoicingParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarStrumSequencerProcessor)
};
