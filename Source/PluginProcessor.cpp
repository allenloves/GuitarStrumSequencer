#include "PluginProcessor.h"
#include "PluginEditor.h"

GuitarStrumSequencerProcessor::GuitarStrumSequencerProcessor()
    : AudioProcessor (BusesProperties()),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

GuitarStrumSequencerProcessor::~GuitarStrumSequencerProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout GuitarStrumSequencerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "subdivision", 1 }, "Subdivision",
        juce::StringArray { "8th Notes", "16th Notes" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "strumSpeed", 1 }, "Strum Speed",
        juce::NormalisableRange<float> (5.0f, 50.0f, 1.0f), 8.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "humanize", 1 }, "Humanize",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "guitarVoicing", 1 }, "Guitar Voicing", true));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tuning", 1 }, "Tuning",
        juce::StringArray { "Standard", "Drop D", "Open G", "DADGAD", "Half Step Down" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "capo", 1 }, "Capo", 0, 12, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "initialPosition", 1 }, "Initial Position", 0, 12, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "maxFret", 1 }, "Max Fret", 5, 15, 12));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "fretSpan", 1 }, "Fret Span", 3, 5, 4));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "preferOpenStrings", 1 }, "Prefer Open Strings", true));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "positionCC", 1 }, "Position CC",
        juce::StringArray { "CC 85", "CC 86", "CC 87", "CC 102", "CC 103", "CC 104", "CC 105", "CC 106" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "searchRange", 1 }, "Search Range", 2, 7, 5));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "multiChannel", 1 }, "Multi-Channel", false));

    // Step velocities 1-16 (default: repeating High/Medium/Low pattern)
    const float defaultStepVelocities[StepSequencer::STEP_COUNT] = {
        107, 0, 90, 90, 0, 90, 100, 90,
        107, 0, 90, 90, 0, 90, 100, 90
    };
    for (int i = 0; i < StepSequencer::STEP_COUNT; ++i)
    {
        auto id = "step" + juce::String (i + 1);
        auto name = "Step " + juce::String (i + 1);
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.0f, 127.0f, 1.0f), defaultStepVelocities[i]));
    }

    return { params.begin(), params.end() };
}

void GuitarStrumSequencerProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    sequencer.reset();
    strumEngine.clearActiveNotes();
    heldNotes.clear();
    voicedNotes.clear();
    ccPositionOverride = -1;
    ccPositionUsed = false;
    wasPlaying = false;
    lastStepHadNoNotes = false;
    lastStrumNotes.clear();
    lastStepIndex = 0;
    lastStepVelocity = 0.0f;
    lastStepBeat = -1.0;
    voicer.reset();
    pendingEvents.clear();

    // Sync step velocities from parameters
    for (int i = 0; i < StepSequencer::STEP_COUNT; ++i)
    {
        auto* param = apvts.getRawParameterValue ("step" + juce::String (i + 1));
        if (param != nullptr)
            sequencer.setStepVelocity (i, param->load());
    }
}

void GuitarStrumSequencerProcessor::releaseResources() {}

bool GuitarStrumSequencerProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

void GuitarStrumSequencerProcessor::insertSorted (std::vector<int>& arr, int pitch)
{
    for (auto it = arr.begin(); it != arr.end(); ++it)
    {
        if (*it == pitch) return;
        if (*it > pitch)
        {
            arr.insert (it, pitch);
            return;
        }
    }
    arr.push_back (pitch);
}

void GuitarStrumSequencerProcessor::removeFromArray (std::vector<int>& arr, int pitch)
{
    auto it = std::find (arr.begin(), arr.end(), pitch);
    if (it != arr.end())
        arr.erase (it);
}

VoicingParams GuitarStrumSequencerProcessor::readVoicingParams()
{
    VoicingParams params;
    int tuningIndex = static_cast<int> (apvts.getRawParameterValue ("tuning")->load());
    int capo = static_cast<int> (apvts.getRawParameterValue ("capo")->load());
    params.openPitches = voicer.getStringOpenPitches (tuningIndex, capo);
    params.fretSpan = static_cast<int> (apvts.getRawParameterValue ("fretSpan")->load());
    params.maxFret = static_cast<int> (apvts.getRawParameterValue ("maxFret")->load());
    params.preferOpen = apvts.getRawParameterValue ("preferOpenStrings")->load() >= 0.5f;
    params.searchRange = static_cast<int> (apvts.getRawParameterValue ("searchRange")->load());
    params.initialPosition = static_cast<int> (apvts.getRawParameterValue ("initialPosition")->load());
    return params;
}

void GuitarStrumSequencerProcessor::updateVoicedNotes()
{
    if (heldNotes.empty())
    {
        voicedNotes.clear();
        return;
    }

    // Extract unique pitch classes
    std::vector<int> pitchClasses;
    bool seen[12] = {};
    for (auto pitch : heldNotes)
    {
        int pc = pitch % 12;
        if (! seen[pc])
        {
            seen[pc] = true;
            pitchClasses.push_back (pc);
        }
    }

    int rootPitchClass = heldNotes[0] % 12;
    auto params = readVoicingParams();

    auto result = voicer.findBestPosition (pitchClasses, rootPitchClass, params, ccPositionOverride);

    if (result.score > -10000)
    {
        std::vector<int> pitches;
        for (int s = 0; s < GuitarVoicer::NUM_STRINGS; ++s)
        {
            if (result.voicing[static_cast<size_t> (s)].pitch >= 0)
                pitches.push_back (result.voicing[static_cast<size_t> (s)].pitch);
        }
        std::sort (pitches.begin(), pitches.end());
        voicedNotes = pitches;

        if (ccPositionUsed)
        {
            ccPositionOverride = -1;
            ccPositionUsed = false;
        }
    }
    else
    {
        voicedNotes = heldNotes;
    }
}

std::vector<int> GuitarStrumSequencerProcessor::getNotesToStrum()
{
    if (apvts.getRawParameterValue ("guitarVoicing")->load() >= 0.5f)
        return voicedNotes;
    return heldNotes;
}

// ── Beat-based pending event scheduling ──────────────────────────────

void GuitarStrumSequencerProcessor::scheduleEvent (const juce::MidiMessage& msg, double beatPos)
{
    pendingEvents.push_back ({ msg, beatPos });
}

void GuitarStrumSequencerProcessor::emitPendingEvents (juce::MidiBuffer& buffer,
                                                         double blockStartBeat,
                                                         double blockEndBeat,
                                                         double beatsPerSample,
                                                         int numSamples)
{
    // Sort by beat position so NoteOffs (scheduled slightly earlier) come before NoteOns
    std::sort (pendingEvents.begin(), pendingEvents.end(),
        [] (const PendingMidiEvent& a, const PendingMidiEvent& b)
        {
            return a.beatPosition < b.beatPosition;
        });

    auto it = pendingEvents.begin();
    while (it != pendingEvents.end())
    {
        if (it->beatPosition < blockStartBeat - 0.1)
        {
            // Stale event (e.g. from before a cycle wrap) — drop silently
            it = pendingEvents.erase (it);
        }
        else if (it->beatPosition < blockStartBeat)
        {
            // Slightly past event (cross-buffer strum note) — emit at start of block
            buffer.addEvent (it->message, 0);
            it = pendingEvents.erase (it);
        }
        else if (it->beatPosition < blockEndBeat)
        {
            // Within this block — calculate exact sample position
            double beatOffset = it->beatPosition - blockStartBeat;
            int samplePos = static_cast<int> (std::round (beatOffset / beatsPerSample));
            samplePos = std::max (0, std::min (samplePos, numSamples - 1));
            buffer.addEvent (it->message, samplePos);
            it = pendingEvents.erase (it);
        }
        else
        {
            // Future event — keep for next block
            ++it;
        }
    }
}

void GuitarStrumSequencerProcessor::killActiveNotesAt (double beatPos)
{
    for (auto& note : strumEngine.getActiveNotes())
    {
        scheduleEvent (juce::MidiMessage::noteOff (note.channel, note.pitch, (juce::uint8) 0),
                       beatPos);
    }
    strumEngine.clearActiveNotes();
}

// ── Main processing ──────────────────────────────────────────────────

void GuitarStrumSequencerProcessor::processBlock (juce::AudioBuffer<float>& audioBuffer,
                                                    juce::MidiBuffer& midiMessages)
{
    audioBuffer.clear();

    // Sync step velocities from parameters
    for (int i = 0; i < StepSequencer::STEP_COUNT; ++i)
    {
        auto* param = apvts.getRawParameterValue ("step" + juce::String (i + 1));
        if (param != nullptr)
            sequencer.setStepVelocity (i, param->load());
    }

    bool voicingEnabled = apvts.getRawParameterValue ("guitarVoicing")->load() >= 0.5f;

    // Determine Position CC number
    int positionCCIndex = static_cast<int> (apvts.getRawParameterValue ("positionCC")->load());
    int positionCCNumber = GuitarVoicer::CC_MAP[static_cast<size_t> (positionCCIndex)];

    // Process input MIDI
    juce::MidiBuffer outputBuffer;

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            insertSorted (heldNotes, msg.getNoteNumber());
            if (voicingEnabled)
                updateVoicedNotes();
        }
        else if (msg.isNoteOff())
        {
            removeFromArray (heldNotes, msg.getNoteNumber());
            if (heldNotes.empty())
            {
                // Don't kill active strum notes or clear pending events.
                // The current strum was committed at the step boundary and
                // should finish playing.  The next step will generate a new
                // strum (or kill active notes if still empty).
                voicedNotes.clear();
            }
            else if (voicingEnabled)
            {
                updateVoicedNotes();
            }
        }
        else if (msg.isController())
        {
            int ccNum = msg.getControllerNumber();
            if (ccNum == 120 || ccNum == 123)
            {
                strumEngine.clearActiveNotes();
                pendingEvents.clear();
                outputBuffer.addEvent (msg, metadata.samplePosition);
            }
            else if (voicingEnabled && ccNum == positionCCNumber)
            {
                ccPositionOverride = static_cast<int> (std::round (msg.getControllerValue() * 15.0 / 127.0));
                ccPositionUsed = false;
                if (! heldNotes.empty())
                    updateVoicedNotes();
            }
            else
            {
                outputBuffer.addEvent (msg, metadata.samplePosition);
            }
        }
        else
        {
            outputBuffer.addEvent (msg, metadata.samplePosition);
        }
    }

    // Get transport info
    auto playHead = getPlayHead();
    if (playHead != nullptr)
    {
        auto posInfo = playHead->getPosition();
        if (posInfo.hasValue())
        {
            bool isPlaying = posInfo->getIsPlaying();
            bool isCycling = posInfo->getIsLooping();
            double cycleStart = 0.0, cycleEnd = 0.0;

            if (isCycling)
            {
                if (auto loopPoints = posInfo->getLoopPoints())
                {
                    cycleStart = loopPoints->ppqStart;
                    cycleEnd = loopPoints->ppqEnd;
                }
                // Disable cycling if loop points are invalid
                if (cycleEnd <= cycleStart)
                    isCycling = false;
            }

            double bpm = 120.0;
            if (auto t = posInfo->getBpm())
                bpm = std::max (*t, 1.0); // guard against 0 BPM

            double ppqPosition = 0.0;
            if (auto p = posInfo->getPpqPosition())
                ppqPosition = *p;

            int numSamples = audioBuffer.getNumSamples();
            double beatsPerSample = bpm / (60.0 * currentSampleRate);
            double blockStartBeat = ppqPosition;
            double blockEndBeat = ppqPosition + numSamples * beatsPerSample;

            // Detect transport stop → kill all active notes immediately
            if (! isPlaying && wasPlaying)
            {
                for (auto& note : strumEngine.getActiveNotes())
                {
                    outputBuffer.addEvent (
                        juce::MidiMessage::noteOff (note.channel, note.pitch, (juce::uint8) 0), 0);
                }
                strumEngine.clearActiveNotes();
                pendingEvents.clear();
                sequencer.reset();
                lastStepHadNoNotes = false;
                lastStrumNotes.clear();
                lastStepBeat = -1.0;
                currentStepForUI.store (-1);
            }
            wasPlaying = isPlaying;

            // Prune stale pending events (e.g. after loop wraparound or seek)
            if (! pendingEvents.empty())
            {
                double minPastBeat = blockStartBeat - 0.5;
                double maxFutureBeat = blockEndBeat + 2.0;
                pendingEvents.erase (
                    std::remove_if (pendingEvents.begin(), pendingEvents.end(),
                        [minPastBeat, maxFutureBeat] (const PendingMidiEvent& e)
                        {
                            return e.beatPosition < minPastBeat
                                || e.beatPosition > maxFutureBeat;
                        }),
                    pendingEvents.end());
            }

            int subdivisionIndex = static_cast<int> (apvts.getRawParameterValue ("subdivision")->load());

            auto stepEvents = sequencer.processBlock (blockStartBeat, blockEndBeat,
                                                       isPlaying, isCycling,
                                                       cycleStart, cycleEnd,
                                                       subdivisionIndex);

            float strumSpeed = apvts.getRawParameterValue ("strumSpeed")->load();
            float humanize = apvts.getRawParameterValue ("humanize")->load() / 100.0f;
            bool multiChannel = voicingEnabled
                && apvts.getRawParameterValue ("multiChannel")->load() >= 0.5f;

            for (auto& event : stepEvents)
            {
                // Always update UI step indicator
                currentStepForUI.store (event.stepIndex);

                // Record step info for potential re-trigger (even if empty/ghost)
                lastStepIndex = event.stepIndex;
                lastStepBeat = event.beatPosition;
                lastStepVelocity = event.velocity;

                auto notes = getNotesToStrum();

                if (notes.empty())
                {
                    // Grace period: on the first empty step, let the old strum
                    // ring (chord transition may span a buffer boundary).  On
                    // the second consecutive empty step, kill active notes.
                    if (lastStepHadNoNotes)
                        killActiveNotesAt (event.beatPosition);
                    lastStepHadNoNotes = true;
                    continue;
                }

                lastStepHadNoNotes = false;

                if (event.velocity <= 0.0f)
                    continue;   // ghost step — let previous strum ring

                // Kill previous strum's notes slightly before the new step
                // (tiny epsilon ensures NoteOff sorts before NoteOn at same beat)
                killActiveNotesAt (event.beatPosition - 0.0001);

                // Remove any orphaned pending NoteOns from the previous strum
                // (they would play notes that are no longer tracked as active)
                pendingEvents.erase (
                    std::remove_if (pendingEvents.begin(), pendingEvents.end(),
                        [] (const PendingMidiEvent& e) { return e.message.isNoteOn(); }),
                    pendingEvents.end());

                // Generate strum with beat-based offsets
                auto strumNotes = strumEngine.generateStrum (
                    notes, event.stepIndex, event.velocity,
                    strumSpeed, humanize, multiChannel, bpm);

                // Schedule each strum note at stepBeat + individual beatOffset
                for (auto& sn : strumNotes)
                {
                    scheduleEvent (
                        juce::MidiMessage::noteOn (sn.channel, sn.pitch, (juce::uint8) sn.velocity),
                        event.beatPosition + sn.beatOffset);
                }

                lastStrumNotes = notes;
            }

            // ── Re-trigger check ──────────────────────────────────────
            // When the track is selected, Logic may deliver chord-change
            // MIDI one buffer late.  If the chord changed since the last
            // strum (or the strum was killed), regenerate immediately.
            if (isPlaying && lastStepBeat >= 0.0)
            {
                auto currentNotes = getNotesToStrum();
                bool needsRetrigger = false;

                if (! currentNotes.empty())
                {
                    if (strumEngine.getActiveNotes().empty())
                        needsRetrigger = true;   // strum was killed (CC#120, grace period, etc.)
                    else if (currentNotes != lastStrumNotes)
                        needsRetrigger = true;   // chord changed since last strum
                }

                if (needsRetrigger)
                {
                    killActiveNotesAt (blockStartBeat);

                    // Remove pending NoteOns from the old strum
                    pendingEvents.erase (
                        std::remove_if (pendingEvents.begin(), pendingEvents.end(),
                            [] (const PendingMidiEvent& e) { return e.message.isNoteOn(); }),
                        pendingEvents.end());

                    auto strumNotes = strumEngine.generateStrum (
                        currentNotes, lastStepIndex, lastStepVelocity,
                        strumSpeed, humanize, multiChannel, bpm);

                    for (auto& sn : strumNotes)
                    {
                        scheduleEvent (
                            juce::MidiMessage::noteOn (sn.channel, sn.pitch, (juce::uint8) sn.velocity),
                            blockStartBeat + sn.beatOffset);
                    }

                    lastStrumNotes = currentNotes;
                    lastStepHadNoNotes = false;
                }
            }

            // Normalise pending events whose beat positions overshot cycleEnd
            // (e.g. strum notes that spread past the cycle boundary).
            // After the transport wraps, these need to map into the new cycle.
            if (isCycling && cycleEnd > cycleStart && ! pendingEvents.empty())
            {
                double cycleLength = cycleEnd - cycleStart;
                for (auto& evt : pendingEvents)
                {
                    while (evt.beatPosition >= cycleEnd)
                        evt.beatPosition -= cycleLength;
                }
            }

            // Emit all pending events that fall within this block's beat range
            emitPendingEvents (outputBuffer, blockStartBeat, blockEndBeat,
                               beatsPerSample, numSamples);
        }
    }

    midiMessages.swapWith (outputBuffer);
}

juce::AudioProcessorEditor* GuitarStrumSequencerProcessor::createEditor()
{
    return new GuitarStrumSequencerEditor (*this);
}

void GuitarStrumSequencerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GuitarStrumSequencerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarStrumSequencerProcessor();
}
