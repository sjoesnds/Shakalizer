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
    float getScopeSample(int index) const noexcept { return scopeBuffer[static_cast<size_t>(juce::jlimit(0, 255, index))].load(); }
    int getScopeWriteIndex() const noexcept { return scopeWriteIndex.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float nextRandom();
    float tpdfDither(float step) noexcept;
    float shapedSample(float x, float drive, float clip) const noexcept;
    float waveFold(float x, float amount) const noexcept;
    float movementValue(int shape, float phase) const noexcept;
    void setFilterFromParameters(int type, float cutoff, float resonance);

    juce::AudioProcessorValueTreeState apvts;

    double currentSampleRate = 44100.0;
    int maxBlockSize = 0;

    juce::dsp::Oversampling<float> oversampler2x {
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true
    };

    juce::dsp::Oversampling<float> oversampler4x {
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true
    };

    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> postFilter;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> safetyFilter;

    juce::AudioBuffer<float> dryBuffer;

    std::array<int, 2> holdRemaining { 0, 0 };
    std::array<int, 2> currentHoldLength { 1, 1 };
    std::array<float, 2> heldSample { 0.0f, 0.0f };
    std::array<float, 2> previousHeldSample { 0.0f, 0.0f };

    std::array<float, 2> splitLow1 { 0.0f, 0.0f };
    std::array<float, 2> splitLow2 { 0.0f, 0.0f };
    std::array<float, 2> splitLow3 { 0.0f, 0.0f };

    std::array<float, 2> fastEnvelope { 0.0f, 0.0f };
    std::array<float, 2> slowEnvelope { 0.0f, 0.0f };

    std::array<float, 2> glitchValue { 0.0f, 0.0f };
    std::array<int, 2> glitchRemaining { 0, 0 };
    std::array<int, 2> glitchCooldown { 0, 0 };
    std::array<int, 2> glitchEventLength { 1, 1 };
    std::array<int, 2> glitchEventAge { 0, 0 };
    std::array<std::array<float, 16384>, 2> glitchBuffer {};
    int glitchWriteIndex = 0;

    std::array<float, 2> spectralFreeze { 0.0f, 0.0f };

    std::array<float, 4> modSmoothState { 0.0f, 0.0f, 0.0f, 0.0f };
    float modPhase = 0.0f;

    std::array<std::array<float, 8192>, 2> resonatorBuffer {};
    int resonatorWriteIndex = 0;

    float movementPhase = 0.0f;
    float syncPhase = 0.0f;
    float movementHoldValue = 0.0f;
    int movementHoldCounter = 0;
    int lastGlitchGridSlot = -1;

    float unstableValue = 0.0f;
    int unstableRemaining = 0;

    std::array<float, 2> alienPhase { 0.0f, 0.0f };

    float autoMatchGain = 1.0f;
    float morphPhase = 0.0f;

    std::uint32_t rngState = 0xA341316Cu;
    std::array<std::atomic<float>, 256> scopeBuffer {};
    std::atomic<int> scopeWriteIndex { 0 };
    std::atomic<float> meterLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShakalizerAudioProcessor)
};
