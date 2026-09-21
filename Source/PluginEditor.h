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

    static constexpr int sliderCount = 48;

    void configureSlider(juce::Slider&, const juce::String&,
                         double min, double max, double step);
    void addAttachment(const juce::String&, juce::Slider&);

    void timerCallback() override;

    void saveA();
    void swapAB();
    void toggleAutoMatch();
    void toggleSmart();

    void randomizeAll();
    void randomizeScope(int scope);

    void savePresetToFile();
    void loadPresetFromFile();

    void loadPreset(int index);

    void captureState(std::vector<float>& destination);
    void applyState(const std::vector<float>& state);

    ShakalizerAudioProcessor& processor;
    ShakalLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::ComboBox presetBox;
    juce::ComboBox modeBox;
    juce::ComboBox resampleBox;
    juce::ComboBox filterBox;
    juce::ComboBox movementBox;
    juce::ComboBox qualityBox;
    juce::ComboBox syncBox;
    juce::ComboBox glitchGridBox;
    juce::ComboBox glitchModeBox;
    juce::ComboBox glitchLengthBox;
    juce::ComboBox spectralModeBox;
    juce::ComboBox routingBox;
    juce::ComboBox msModeBox;
    juce::ComboBox liveSceneBox;

    std::array<juce::ComboBox*, 4> modSourceBoxes {
        nullptr, nullptr, nullptr, nullptr
    };

    std::array<juce::ComboBox*, 4> modDestBoxes {
        nullptr, nullptr, nullptr, nullptr
    };

    juce::ComboBox modSource1Box;
    juce::ComboBox modSource2Box;
    juce::ComboBox modSource3Box;
    juce::ComboBox modSource4Box;

    juce::ComboBox modDest1Box;
    juce::ComboBox modDest2Box;
    juce::ComboBox modDest3Box;
    juce::ComboBox modDest4Box;

    juce::TextButton saveAButton { "SAVE A" };
    juce::TextButton abButton { "A / B" };
    juce::TextButton autoMatchButton { "AUTO" };
    juce::TextButton smartButton { "SMART" };

    juce::TextButton randomAllButton { "RANDOM ALL" };
    juce::TextButton randomCoreButton { "CORE" };
    juce::TextButton randomShatterButton { "SHATTER" };
    juce::TextButton randomGlitchButton { "GLITCH" };
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton loadPresetButton { "LOAD" };

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
    juce::Slider smoothSlider;

    juce::Slider mod1AmountSlider;
    juce::Slider mod2AmountSlider;
    juce::Slider mod3AmountSlider;
    juce::Slider mod4AmountSlider;
    juce::Slider morphSlider;

    juce::Slider glitchDensitySlider;
    juce::Slider glitchProbabilitySlider;
    juce::Slider glitchFadeSlider;
    juce::Slider glitchVariationSlider;

    juce::Slider spectralMixSlider;
    juce::Slider spectralSmearSlider;
    juce::Slider spectralFreezeAmountSlider;
    juce::Slider spectralBitsSlider;
    juce::Slider spectralRingSlider;

    juce::Slider modRateSlider;
    juce::Slider modDepthSlider;
    juce::Slider modSmoothSlider;

    std::array<juce::Slider*, sliderCount> sliders {
        &shakalSlider, &destroySlider, &crushSlider, &decimateSlider,
        &driveSlider, &clipSlider, &glitchSlider, &jitterSlider,
        &splitSlider, &transientSlider, &bodySlider, &stereoSlider,
        &movementSlider, &unstableSlider, &alienSlider,
        &filterFreqSlider, &filterResSlider, &mixSlider, &outputSlider,
        &shatterSlider, &foldSlider, &shiftSlider, &resonanceSlider,
        &envFollowSlider, &bandLowSlider, &bandMidSlider,
        &bandHighSlider, &bandAirSlider, &characterSlider,
        &preGainSlider, &smoothSlider,
        &mod1AmountSlider, &mod2AmountSlider,
        &mod3AmountSlider, &mod4AmountSlider, &morphSlider,
        &glitchDensitySlider, &glitchProbabilitySlider,
        &glitchFadeSlider, &glitchVariationSlider,
        &spectralMixSlider, &spectralSmearSlider,
        &spectralFreezeAmountSlider, &spectralBitsSlider,
        &spectralRingSlider,
        &modRateSlider, &modDepthSlider, &modSmoothSlider
    };

    std::array<juce::String, sliderCount> sliderNames {
        "SHAKAL", "DESTROY", "CRUSH", "DECIMATE",
        "DRIVE", "CLIP", "GLITCH", "JITTER",
        "SPLIT", "TRANSIENT", "BODY", "STEREO",
        "MOVEMENT", "UNSTABLE", "ALIEN", "FILTER FREQ",
        "RESONANCE", "MIX", "OUTPUT", "SHATTER",
        "FOLD", "SHIFT", "RESONATOR", "ENVELOPE",
        "LOW SHATTER", "MID SHATTER", "HIGH SHATTER",
        "AIR SHATTER", "CHARACTER", "PRE GAIN", "SMOOTH",
        "MOD 1", "MOD 2", "MOD 3", "MOD 4", "MORPH",
        "GLITCH DENSITY", "GLITCH PROB", "GLITCH FADE",
        "GLITCH VAR", "SPECTRAL MIX", "SPECTRAL SMEAR",
        "SPECTRAL FREEZE", "SPECTRAL BITS", "SPECTRAL RING",
        "MOD RATE", "MOD DEPTH", "MOD SMOOTH"
    };

    std::vector<std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>>
        attachments;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        modeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        resampleAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        filterAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        movementAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        qualityAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        syncAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        glitchGridAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        glitchModeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        glitchLengthAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        spectralModeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        routingAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        msModeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        liveSceneAttachment;

    std::array<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 4>
        modSourceAttachments;

    std::array<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 4>
        modDestAttachments;

    std::vector<float> abState;
    bool hasAState = false;

    std::unique_ptr<juce::FileChooser> presetFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        ShakalizerAudioProcessorEditor)
};
