cmake_minimum_required(VERSION 3.22)

project(Shakalizer VERSION 3.0.0)

include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Shakalizer
    COMPANY_NAME "MidiForge Labs"
    PRODUCT_NAME "Shakalizer"
    PLUGIN_MANUFACTURER_CODE Mflb
    PLUGIN_CODE Shak
    FORMATS VST3
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Distortion
)

target_sources(Shakalizer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

juce_generate_juce_header(Shakalizer)

target_compile_features(Shakalizer PRIVATE cxx_std_17)

target_compile_definitions(Shakalizer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if (MSVC)
    target_compile_options(Shakalizer PRIVATE /utf-8)
endif()

target_link_libraries(Shakalizer PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
