cmake_minimum_required(VERSION 3.22)
project(GuitarStrumSequencer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(GuitarStrumSequencer
    COMPANY_NAME "GuitarTools"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE GtSt
    PLUGIN_CODE GtSS
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Guitar Strum Sequencer"
    BUNDLE_ID "com.guitartools.strumsequencer"
    AU_MAIN_TYPE "kAudioUnitType_MIDIProcessor"
)

target_sources(GuitarStrumSequencer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/GuitarVoicer.cpp
        Source/StepSequencer.cpp
        Source/StrumEngine.cpp
        Source/UI/StepSequencerComponent.cpp
        Source/UI/ControlPanelComponent.cpp
        Source/UI/CustomLookAndFeel.cpp
)

target_compile_definitions(GuitarStrumSequencer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(GuitarStrumSequencer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
