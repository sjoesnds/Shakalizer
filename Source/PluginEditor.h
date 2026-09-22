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

    static constexpr int sliderCount = 101;

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
    void setPage(int page);
    bool sliderBelongsToPage(int sliderIndex, int page) const noexcept;

    void captureState(std::vector<float>& destination);
    void applyState(const std::vector<float>& state);

    ShakalizerAudioProcessor& processor;
    ShakalLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label pageLabel;

    std::array<juce::TextButton, 7> pageButtons {
        juce::TextButton { "CORE" },
        juce::TextButton { "GLITCH" },
        juce::TextButton { "SPECTRAL" },
        juce::TextButton { "MOD" },
        juce::TextButton { "GRANULAR" },
        juce::TextButton { "FEEDBACK" },
        juce::TextButton { "REACTIVE" }
    };

    int currentPage = 0;

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
    juce::ComboBox modWaveBox;
    juce::ComboBox modSyncBox;
    juce::ComboBox routingBox;
    juce::ComboBox msModeBox;
    juce::ComboBox liveSceneBox;
    juce::ComboBox characterModeBox;
    juce::ComboBox fftWindowBox;
    juce::ComboBox pitchModeBox;
    juce::ComboBox routingTopologyBox;

    std::array<juce::ComboBox*, 8> modSourceBoxes {
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr
    };

    std::array<juce::ComboBox*, 8> modDestBoxes {
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr
    };

    juce::ComboBox modSource1Box;
    juce::ComboBox modSource2Box;
    juce::ComboBox modSource3Box;
    juce::ComboBox modSource4Box;
    juce::ComboBox modSource5Box;
    juce::ComboBox modSource6Box;
    juce::ComboBox modSource7Box;
    juce::ComboBox modSource8Box;

    juce::ComboBox modDest1Box;
    juce::ComboBox modDest2Box;
    juce::ComboBox modDest3Box;
    juce::ComboBox modDest4Box;
    juce::ComboBox modDest5Box;
    juce::ComboBox modDest6Box;
    juce::ComboBox modDest7Box;
    juce::ComboBox modDest8Box;

    juce::TextButton saveAButton { "SAVE A" };
    juce::TextButton abButton { "A / B" };
    juce::TextButton autoMatchButton { "AUTO" };
    juce::TextButton smartButton { "SMART" };

    juce::TextButton randomAllButton { "RANDOM ALL" };
    juce::TextButton randomCoreButton { "CORE" };
    juce::TextButton randomShatterButton { "SHATTER" };
    juce::TextButton randomGlitchButton { "GLITCH" };
    juce::TextButton randomModButton { "MOD" };
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton loadPresetButton { "LOAD" };
    juce::TextButton favoritePresetButton { "FAV" };

    juce::Label meterLabel { {}, "OUT" };
    juce::Label cpuLabel { {}, "CPU 0%" };
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

    juce::Slider smartAmountSlider;
    juce::Slider smartBassProtectSlider;
    juce::Slider smartTransientProtectSlider;
    juce::Slider smartHighControlSlider;

    juce::Slider mod5AmountSlider;
    juce::Slider mod6AmountSlider;
    juce::Slider mod7AmountSlider;
    juce::Slider mod8AmountSlider;
    juce::Slider fftMixSlider;
    juce::Slider fftShatterSlider;
    juce::Slider fftFreezeSlider;
    juce::Slider fftBitsSlider;
    juce::Slider fftShiftSlider;
    juce::Slider grainMixSlider;
    juce::Slider grainSizeSlider;
    juce::Slider grainPitchSlider;
    juce::Slider grainJitterSlider;
    juce::Slider feedbackSlider;
    juce::Slider feedbackToneSlider;
    juce::Slider feedbackDriveSlider;
    juce::Slider pitchChaosSlider;
    juce::Slider timelineMixSlider;
    juce::Slider timelineStep1Slider;
    juce::Slider timelineStep2Slider;
    juce::Slider timelineStep3Slider;
    juce::Slider timelineStep4Slider;
    juce::Slider timelineStep5Slider;
    juce::Slider timelineStep6Slider;
    juce::Slider timelineStep7Slider;
    juce::Slider timelineStep8Slider;

    juce::Slider fftSpreadSlider;
    juce::Slider fftThresholdSlider;
    juce::Slider fftWarpSlider;
    juce::Slider grainDensitySlider;
    juce::Slider grainPositionSlider;
    juce::Slider grainSpraySlider;
    juce::Slider grainReverseSlider;
    juce::Slider grainPanSlider;
    juce::Slider feedbackTimeSlider;
    juce::Slider feedbackDiffusionSlider;
    juce::Slider feedbackFreezeSlider;
    juce::Slider feedbackSpreadSlider;
    juce::Slider feedbackPitchSlider;
    juce::Slider pitchDamageSlider;
    juce::Slider pitchRangeSlider;
    juce::Slider pitchDriftSlider;
    juce::Slider reactiveAmountSlider;
    juce::Slider reactiveTransientSlider;
    juce::Slider reactiveSpectralSlider;
    juce::Slider reactiveBassSlider;
    juce::Slider reactiveHighSlider;
    juce::Slider macroCurveSlider;
    juce::Slider sceneMorphTimeSlider;

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
        &modRateSlider, &modDepthSlider, &modSmoothSlider,
        &smartAmountSlider, &smartBassProtectSlider,
        &smartTransientProtectSlider, &smartHighControlSlider,
        &mod5AmountSlider, &mod6AmountSlider, &mod7AmountSlider,
        &mod8AmountSlider, &fftMixSlider, &fftShatterSlider,
        &fftFreezeSlider, &fftBitsSlider, &fftShiftSlider,
        &grainMixSlider, &grainSizeSlider, &grainPitchSlider,
        &grainJitterSlider, &feedbackSlider, &feedbackToneSlider,
        &feedbackDriveSlider, &pitchChaosSlider, &timelineMixSlider,
        &timelineStep1Slider, &timelineStep2Slider, &timelineStep3Slider,
        &timelineStep4Slider, &timelineStep5Slider, &timelineStep6Slider,
        &timelineStep7Slider, &timelineStep8Slider,
        &fftSpreadSlider, &fftThresholdSlider, &fftWarpSlider,
        &grainDensitySlider, &grainPositionSlider, &grainSpraySlider,
        &grainReverseSlider, &grainPanSlider, &feedbackTimeSlider,
        &feedbackDiffusionSlider, &feedbackFreezeSlider, &feedbackSpreadSlider,
        &feedbackPitchSlider, &pitchDamageSlider, &pitchRangeSlider,
        &pitchDriftSlider, &reactiveAmountSlider, &reactiveTransientSlider,
        &reactiveSpectralSlider, &reactiveBassSlider, &reactiveHighSlider,
        &macroCurveSlider, &sceneMorphTimeSlider
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
        "MOD RATE", "MOD DEPTH", "MOD SMOOTH",
        "SMART AMOUNT", "SMART BASS", "SMART TRANSIENT",
        "SMART HIGH", "MOD 5", "MOD 6", "MOD 7", "MOD 8",
        "FFT MIX", "FFT SHATTER", "FFT FREEZE", "FFT BITS",
        "FFT SHIFT", "GRAIN MIX", "GRAIN SIZE", "GRAIN PITCH",
        "GRAIN JITTER", "FEEDBACK", "FEEDBACK TONE", "FEEDBACK DRIVE",
        "PITCH CHAOS", "TIMELINE MIX", "STEP 1", "STEP 2", "STEP 3",
        "STEP 4", "STEP 5", "STEP 6", "STEP 7", "STEP 8",
        "FFT SPREAD", "FFT THRESHOLD", "FFT WARP",
        "GRAIN DENSITY", "GRAIN POSITION", "GRAIN SPRAY",
        "GRAIN REVERSE", "GRAIN PAN", "FEEDBACK TIME",
        "FEEDBACK DIFFUSION", "FEEDBACK FREEZE", "FEEDBACK SPREAD",
        "FEEDBACK PITCH", "PITCH DAMAGE", "PITCH RANGE", "PITCH DRIFT",
        "REACTIVE AMOUNT", "REACTIVE TRANSIENT", "REACTIVE SPECTRAL",
        "REACTIVE BASS", "REACTIVE HIGH", "MACRO CURVE", "SCENE MORPH"
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
        modWaveAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        modSyncAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        routingAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        msModeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        liveSceneAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        characterModeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        fftWindowAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        pitchModeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        routingTopologyAttachment;

    std::array<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 8>
        modSourceAttachments;

    std::array<std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 8>
        modDestAttachments;

    std::vector<float> abState;
    bool hasAState = false;

    std::array<bool, 16> favoritePresets {};
    std::unique_ptr<juce::FileChooser> presetFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        ShakalizerAudioProcessorEditor)
};
