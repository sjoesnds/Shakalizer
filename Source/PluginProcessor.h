#pragma once

#include <JuceHeader.h>
#include <array>
#include <limits>
#include <cstdint>

class ShakalizerAudioProcessor final : public juce::AudioProcessor
{
public:
    ShakalizerAudioProcessor();
    ~ShakalizerAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Shakalizer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    float getMeterLevel() const noexcept { return meterLevel.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float nextRandom();
    float tpdfDither(float step) noexcept;
    float shapedSample(float x, float drive, float clip) const noexcept;
    float getMovementValue(int shape, float phase) noexcept;
    void setFilterFromParameters(int type, float cutoff, float resonance);

    juce::AudioProcessorValueTreeState apvts;

    double currentSampleRate = 44100.0;
    int maxBlockSize = 0;

    juce::dsp::Oversampling<float> oversampler {
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true
    };

    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> postFilter;

    std::array<int, 2> holdRemaining { 0, 0 };
    std::array<int, 2> currentHoldLength { 1, 1 };

    std::array<float, 2> heldSample { 0.0f, 0.0f };
    std::array<float, 2> previousHeldSample { 0.0f, 0.0f };

    std::array<float, 2> splitLowState { 0.0f, 0.0f };
    std::array<float, 2> fastEnvelope { 0.0f, 0.0f };
    std::array<float, 2> slowEnvelope { 0.0f, 0.0f };

    std::array<float, 2> glitchValue { 0.0f, 0.0f };
    std::array<int, 2> glitchRemaining { 0, 0 };
    std::array<int, 2> glitchCooldown { 0, 0 };

    float movementPhase = 0.0f;
    float unstableValue = 0.0f;
    int unstableRemaining = 0;

    std::array<float, 2> alienPhase { 0.0f, 0.0f };

    float autoMatchGain = 1.0f;
    float meterRelease = 0.92f;

    std::uint32_t rngState = 0xA341316Cu;
    std::atomic<float> meterLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShakalizerAudioProcessor)
};
