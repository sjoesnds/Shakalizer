#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ShakalizerAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
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
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
        void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
        void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    };

    class Meter final : public juce::Component
    {
    public:
        void setLevel(float v) { level = juce::jlimit(0.0f, 1.0f, v); repaint(); }
        void paint(juce::Graphics&) override;
    private:
        float level = 0.0f;
    };

    void timerCallback() override;
    void randomize();
    void configureSlider(juce::Slider&, const juce::String&);
    void addAttachment(const juce::String&, juce::Slider&);

    ShakalizerAudioProcessor& processor;
    ShakalLookAndFeel lookAndFeel;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label modeLabel;
    juce::ComboBox modeBox;
    juce::TextButton randomButton { "RANDOMIZE" };
    juce::Label meterLabel { {}, "OUTPUT" };
    Meter meter;

    juce::Slider destroySlider, crushSlider, decimateSlider, driveSlider, clipSlider;
    juce::Slider glitchSlider, jitterSlider, toneSlider, mixSlider, outputSlider;

    std::array<juce::Slider*, 10> sliders {
        &destroySlider, &crushSlider, &decimateSlider, &driveSlider, &clipSlider,
        &glitchSlider, &jitterSlider, &toneSlider, &mixSlider, &outputSlider };

    std::array<juce::String, 10> sliderNames {
        "DESTROY", "CRUSH", "DECIMATE", "DRIVE", "CLIP",
        "GLITCH", "JITTER", "TONE", "MIX", "OUTPUT" };

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShakalizerAudioProcessorEditor)
};
