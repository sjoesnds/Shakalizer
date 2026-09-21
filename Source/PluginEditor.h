#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <vector>

class ShakalizerAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit ShakalizerAudioProcessorEditor(ShakalizerAudioProcessor&);
    ~ShakalizerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class ShakalLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        ShakalLookAndFeel();

        void drawRotarySlider(juce::Graphics&, int, int, int, int,
                              float, float, float, juce::Slider&) override;

        void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int,
                          juce::ComboBox&) override;

        void drawButtonBackground(juce::Graphics&, juce::Button&,
                                  const juce::Colour&, bool, bool) override;
    };

    class Meter final : public juce::Component
    {
    public:
        void setLevel(float v)
        {
            level = juce::jlimit(0.0f, 1.0f, v);
            repaint();
        }

        void paint(juce::Graphics&) override;

    private:
        float level = 0.0f;
    };

    static constexpr int sliderCount = 30;

    void configureSlider(juce::Slider&, const juce::String&,
                         double min, double max, double step);
    void addAttachment(const juce::String&, juce::Slider&);

    void timerCallback() override;
    void saveA();
    void swapAB();
    void toggleAutoMatch();
    void randomize();
    void updateAutoMatchButton();

    void captureState(std::vector<float>& destination);
    void applyState(const std::vector<float>& state);

    ShakalizerAudioProcessor& processor;
    ShakalLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::ComboBox modeBox;
    juce::ComboBox resampleBox;
    juce::ComboBox filterBox;
    juce::ComboBox movementBox;
    juce::ComboBox qualityBox;
    juce::ComboBox syncBox;
    juce::ComboBox glitchGridBox;

    juce::TextButton saveAButton { "SAVE A" };
    juce::TextButton abButton { "A / B" };
    juce::TextButton autoMatchButton { "AUTO MATCH" };
    juce::TextButton randomButton { "RANDOMIZE ALL" };

    juce::Label meterLabel { {}, "OUT" };
    Meter meter;

    juce::Slider shakalSlider;
    juce::Slider destroySlider;
    juce::Slider crushSlider;
    juce::Slider decimateSlider;
    juce::Slider driveSlider;
    juce::Slider clipSlider;
    juce::Slider glitchSlider;
    juce::Slider jitterSlider;
    juce::Slider splitSlider;
    juce::Slider transientSlider;
    juce::Slider bodySlider;
    juce::Slider stereoSlider;
    juce::Slider movementSlider;
    juce::Slider unstableSlider;
    juce::Slider alienSlider;
    juce::Slider filterFreqSlider;
    juce::Slider filterResSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;

    juce::Slider shatterSlider;
    juce::Slider foldSlider;
    juce::Slider shiftSlider;
    juce::Slider resonanceSlider;
    juce::Slider envFollowSlider;
    juce::Slider bandLowSlider;
    juce::Slider bandMidSlider;
    juce::Slider bandHighSlider;
    juce::Slider bandAirSlider;
    juce::Slider characterSlider;
    juce::Slider preGainSlider;

    std::array<juce::Slider*, sliderCount> sliders {
        &shakalSlider, &destroySlider, &crushSlider, &decimateSlider, &driveSlider,
        &clipSlider, &glitchSlider, &jitterSlider, &splitSlider, &transientSlider,
        &bodySlider, &stereoSlider, &movementSlider, &unstableSlider, &alienSlider,
        &filterFreqSlider, &filterResSlider, &mixSlider, &outputSlider,
        &shatterSlider, &foldSlider, &shiftSlider, &resonanceSlider, &envFollowSlider,
        &bandLowSlider, &bandMidSlider, &bandHighSlider, &bandAirSlider,
        &characterSlider, &preGainSlider
    };

    std::array<juce::String, sliderCount> sliderNames {
        "SHAKAL", "DESTROY", "CRUSH", "DECIMATE", "DRIVE", "CLIP",
        "GLITCH", "JITTER", "SPLIT", "TRANSIENT", "BODY", "STEREO",
        "MOVEMENT", "UNSTABLE", "ALIEN", "FILTER FREQ", "RESONANCE",
        "MIX", "OUTPUT", "SHATTER", "FOLD", "SHIFT", "RESONATOR",
        "ENVELOPE", "LOW SHATTER", "MID SHATTER", "HIGH SHATTER",
        "AIR SHATTER", "CHARACTER", "PRE GAIN"
    };

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    std::vector<float> abState;
    bool hasAState = false;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> resampleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> movementAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> qualityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> glitchGridAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShakalizerAudioProcessorEditor)
};
