#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

float clamp01(float v)
{
    return juce::jlimit(0.0f, 1.0f, v);
}

float map01(float v, float lo, float hi)
{
    return lo + clamp01(v) * (hi - lo);
}

int choiceIndex(juce::AudioProcessorValueTreeState& state,
                const juce::String& id, int fallback)
{
    if (auto* parameter = dynamic_cast<const juce::AudioParameterChoice*>(
            state.getParameter(id)))
        return parameter->getIndex();

    return fallback;
}

float onePoleAlpha(float cutoff, double sampleRate)
{
    return std::exp(-2.0f * juce::MathConstants<float>::pi
                    * cutoff / static_cast<float>(sampleRate));
}
}

ShakalizerAudioProcessor::ShakalizerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
ShakalizerAudioProcessor::createParameterLayout()
{
    using FloatRange = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shakal", "Shakal", FloatRange(0.0f, 1.0f, 0.001f), 0.62f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "destroy", "Destroy", FloatRange(0.0f, 1.0f, 0.001f), 0.58f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "crush", "Crush", FloatRange(0.0f, 1.0f, 0.001f), 0.48f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "decimate", "Decimate", FloatRange(0.0f, 1.0f, 0.001f), 0.42f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive", FloatRange(0.0f, 1.0f, 0.001f), 0.42f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "clip", "Clip", FloatRange(0.0f, 1.0f, 0.001f), 0.36f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "glitch", "Glitch", FloatRange(0.0f, 1.0f, 0.001f), 0.12f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "jitter", "Jitter", FloatRange(0.0f, 1.0f, 0.001f), 0.10f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "split", "Split", FloatRange(0.0f, 1.0f, 0.001f), 0.58f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "transient", "Transient", FloatRange(0.0f, 1.0f, 0.001f), 0.70f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "body", "Body", FloatRange(0.0f, 1.0f, 0.001f), 0.64f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stereo", "Stereo", FloatRange(0.0f, 1.0f, 0.001f), 0.28f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "movement", "Movement", FloatRange(0.0f, 1.0f, 0.001f), 0.20f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "unstable", "Unstable", FloatRange(0.0f, 1.0f, 0.001f), 0.10f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "alien", "Alien", FloatRange(0.0f, 1.0f, 0.001f), 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterFreq", "Filter Frequency",
        FloatRange(80.0f, 18000.0f, 1.0f, 0.35f), 14500.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterRes", "Filter Resonance",
        FloatRange(0.05f, 0.95f, 0.001f), 0.38f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix", FloatRange(0.0f, 1.0f, 0.001f), 0.84f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", FloatRange(-12.0f, 6.0f, 0.01f), -1.0f));

    // New v0.3 sound-design controls.
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shatter", "Spectral Shatter", FloatRange(0.0f, 1.0f, 0.001f), 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fold", "Wave Fold", FloatRange(0.0f, 1.0f, 0.001f), 0.12f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shift", "Shift", FloatRange(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "resonance", "Resonator", FloatRange(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "envFollow", "Envelope", FloatRange(0.0f, 1.0f, 0.001f), 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandLow", "Low Shatter", FloatRange(0.0f, 1.0f, 0.001f), 0.18f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandMid", "Mid Shatter", FloatRange(0.0f, 1.0f, 0.001f), 0.62f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandHigh", "High Shatter", FloatRange(0.0f, 1.0f, 0.001f), 0.82f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandAir", "Air Shatter", FloatRange(0.0f, 1.0f, 0.001f), 0.58f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "character", "Character", FloatRange(0.0f, 1.0f, 0.001f), 0.62f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "preGain", "Pre Gain", FloatRange(-24.0f, 12.0f, 0.01f), 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode",
        juce::StringArray {
            "Clean", "Crunch", "Shakal", "Destroy", "Fried",
            "Pixel", "Alien", "Melt", "Shatter"
        }, 2));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "resampleMode", "Resample",
        juce::StringArray { "Hold", "Linear", "Stair", "Smear", "Random" }, 1));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "filterType", "Filter",
        juce::StringArray { "Low Pass", "Band Pass", "High Pass" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "movementShape", "Movement Shape",
        juce::StringArray { "Sine", "Triangle", "Sample+Hold", "Stepped" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "quality", "Quality",
        juce::StringArray { "1x", "2x", "4x" }, 2));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "syncRate", "Sync",
        juce::StringArray { "Free", "1/4", "1/8", "1/16", "1/32" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "glitchGrid", "Glitch Grid",
        juce::StringArray { "Free", "1/8", "1/16", "1/32" }, 1));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        "autoMatch", "Auto Match", false));

    return { p.begin(), p.end() };
}

void ShakalizerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxBlockSize = juce::jmax(1, samplesPerBlock);

    oversampler2x.reset();
    oversampler4x.reset();

    oversampler2x.initProcessing(static_cast<size_t>(maxBlockSize));
    oversampler4x.initProcessing(static_cast<size_t>(maxBlockSize));

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    for (auto& f : postFilter)
    {
        f.prepare(spec);
        f.reset();
        f.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        f.setCutoffFrequency(14500.0f);
        f.setResonance(0.38f);
    }

    holdRemaining.fill(0);
    currentHoldLength.fill(1);
    heldSample.fill(0.0f);
    previousHeldSample.fill(0.0f);
    splitLow1.fill(0.0f);
    splitLow2.fill(0.0f);
    splitLow3.fill(0.0f);
    fastEnvelope.fill(0.0f);
    slowEnvelope.fill(0.0f);
    glitchValue.fill(0.0f);
    glitchRemaining.fill(0);
    glitchCooldown.fill(0);
    alienPhase.fill(0.0f);
    resonatorBuffer = {};
    resonatorWriteIndex = 0;

    movementPhase = 0.0f;
    syncPhase = 0.0f;
    lastGlitchGridSlot = -1;
    unstableValue = 0.0f;
    unstableRemaining = 0;
    autoMatchGain = 1.0f;
    meterLevel.store(0.0f);
}

void ShakalizerAudioProcessor::releaseResources()
{
    oversampler2x.reset();
    oversampler4x.reset();

    for (auto& f : postFilter)
        f.reset();
}

bool ShakalizerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return input == juce::AudioChannelSet::mono()
        || input == juce::AudioChannelSet::stereo();
}

float ShakalizerAudioProcessor::nextRandom()
{
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;

    return static_cast<float>(rngState)
         / static_cast<float>(std::numeric_limits<std::uint32_t>::max());
}

float ShakalizerAudioProcessor::tpdfDither(float step) noexcept
{
    return (nextRandom() - nextRandom()) * step;
}

float ShakalizerAudioProcessor::shapedSample(
    float x, float drive, float clip) const noexcept
{
    const float gain =
        juce::Decibels::decibelsToGain(juce::jmap(drive, 0.0f, 30.0f));

    const float pushed = x * gain;

    const float soft =
        std::tanh(pushed * (0.90f + 1.9f * drive));

    const float threshold =
        juce::jmap(clip, 0.98f, 0.18f);

    const float clipped =
        juce::jlimit(-threshold, threshold, pushed);

    const float hard =
        std::tanh((clipped / juce::jmax(0.001f, threshold)) * 2.6f);

    const float shaped =
        juce::jmap(clip * clip, soft, hard);

    return shaped * juce::jmap(drive, 1.0f, 0.56f);
}

float ShakalizerAudioProcessor::waveFold(float x, float amount) const noexcept
{
    if (amount <= 0.0001f)
        return x;

    const float gain = 1.0f + amount * 8.0f;
    const float v = x * gain;

    float folded = std::fmod(v + 2.0f, 4.0f);
    if (folded < 0.0f)
        folded += 4.0f;

    folded = std::abs(folded - 2.0f) - 1.0f;

    return juce::jmap(amount, x, folded);
}

float ShakalizerAudioProcessor::getMovementValue(int shape, float phase) noexcept
{
    const float wrapped =
        phase / (2.0f * pi)
        - std::floor(phase / (2.0f * pi));

    switch (shape)
    {
        case 1:
        {
            const float t =
                wrapped < 0.5f ? wrapped * 2.0f : 2.0f - wrapped * 2.0f;
            return t * 2.0f - 1.0f;
        }

        case 2:
            return unstableValue;

        case 3:
            return std::floor(wrapped * 8.0f) / 3.5f - 1.0f;

        default:
            return std::sin(phase);
    }
}

void ShakalizerAudioProcessor::setFilterFromParameters(
    int type, float cutoff, float resonance)
{
    auto filterType = juce::dsp::StateVariableTPTFilterType::lowpass;

    if (type == 1)
        filterType = juce::dsp::StateVariableTPTFilterType::bandpass;
    else if (type == 2)
        filterType = juce::dsp::StateVariableTPTFilterType::highpass;

    for (auto& f : postFilter)
    {
        f.setType(filterType);
        f.setCutoffFrequency(cutoff);
        f.setResonance(resonance);
    }
}

void ShakalizerAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();

    if (channels == 0 || samples == 0)
        return;

    auto value = [this](const char* id)
    {
        return apvts.getRawParameterValue(id)->load();
    };

    const float shakal = clamp01(value("shakal"));
    const float destroy = clamp01(value("destroy"));
    const float crush = clamp01(value("crush"));
    const float decimate = clamp01(value("decimate"));
    const float drive = clamp01(value("drive"));
    const float clip = clamp01(value("clip"));
    const float glitch = clamp01(value("glitch"));
    const float jitter = clamp01(value("jitter"));

    const float split = clamp01(value("split"));
    const float transient = clamp01(value("transient"));
    const float body = clamp01(value("body"));
    const float stereo = clamp01(value("stereo"));
    const float movement = clamp01(value("movement"));
    const float unstable = clamp01(value("unstable"));
    const float alien = clamp01(value("alien"));

    const float filterFreq = value("filterFreq");
    const float filterRes = clamp01(value("filterRes"));
    const float mix = clamp01(value("mix"));
    const float outputDb = value("output");

    const float shatter = clamp01(value("shatter"));
    const float fold = clamp01(value("fold"));
    const float shift = clamp01(value("shift"));
    const float resonance = clamp01(value("resonance"));
    const float envFollow = clamp01(value("envFollow"));
    const float bandLow = clamp01(value("bandLow"));
    const float bandMid = clamp01(value("bandMid"));
    const float bandHigh = clamp01(value("bandHigh"));
    const float bandAir = clamp01(value("bandAir"));
    const float character = clamp01(value("character"));
    const float preGain = value("preGain");
    const bool autoMatch = value("autoMatch") > 0.5f;

    const int mode = choiceIndex(apvts, "mode", 2);
    const int resampleMode = choiceIndex(apvts, "resampleMode", 1);
    const int filterType = choiceIndex(apvts, "filterType", 0);
    const int movementShape = choiceIndex(apvts, "movementShape", 0);
    const int quality = choiceIndex(apvts, "quality", 2);
    const int syncRate = choiceIndex(apvts, "syncRate", 0);
    const int glitchGrid = choiceIndex(apvts, "glitchGrid", 1);

    const float modeScale =
        juce::jmap(static_cast<float>(mode), 0.0f, 8.0f, 0.18f, 1.50f);

    const float macro =
        juce::jmap(shakal, 0.0f, 1.0f, 0.45f, 1.0f);

    float intensity =
        clamp01(destroy * macro * modeScale
                * juce::jmap(character, 0.0f, 1.0f, 0.72f, 1.18f));

    if (mode == 5) intensity = clamp01(intensity * 1.12f);
    if (mode == 6) intensity = clamp01(intensity * 1.05f);
    if (mode == 7) intensity = clamp01(intensity * 1.20f);
    if (mode == 8) intensity = clamp01(intensity * 1.10f);

    const float driveBase =
        clamp01(drive * (0.30f + intensity * 0.92f)
                * juce::jmap(character, 0.65f, 1.20f));

    const float clipBase =
        clamp01(clip * (0.24f + intensity * 0.92f)
                * juce::jmap(character, 0.70f, 1.18f));

    const float crushBase =
        clamp01(crush * (0.18f + intensity * 0.95f));

    const float decimateBase =
        clamp01(decimate * (0.06f + intensity * 0.94f));

    // Pre gain is deliberately before the nonlinear/quantisation path.
    buffer.applyGain(juce::Decibels::decibelsToGain(
        juce::jlimit(-24.0f, 12.0f, preGain)));

    auto nonlinearStage = [this, driveBase, clipBase, fold, mode]
        (juce::dsp::AudioBlock<float>& block)
    {
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            for (int ch = 0; ch < static_cast<int>(block.getNumChannels()); ++ch)
            {
                float* data =
                    block.getChannelPointer(static_cast<size_t>(ch));

                float x = shapedSample(data[sample], driveBase, clipBase);

                if (mode == 6)
                    x = waveFold(x, fold * 0.65f);
                else if (mode == 7)
                    x = waveFold(x, fold);
                else
                    x = waveFold(x, fold * 0.70f);

                data[sample] = x;
            }
        }
    };

    if (quality == 1)
    {
        const auto in =
            juce::dsp::AudioBlock<const float>(buffer);
        auto up = oversampler2x.processSamplesUp(in);
        nonlinearStage(up);
        auto out =
            juce::dsp::AudioBlock<float>(buffer);
        oversampler2x.processSamplesDown(out);
    }
    else if (quality >= 2)
    {
        const auto in =
            juce::dsp::AudioBlock<const float>(buffer);
        auto up = oversampler4x.processSamplesUp(in);
        nonlinearStage(up);
        auto out =
            juce::dsp::AudioBlock<float>(buffer);
        oversampler4x.processSamplesDown(out);
    }
    else
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        nonlinearStage(block);
    }

    const int baseBits =
        juce::jlimit(3, 16,
            static_cast<int>(std::round(
                16.0f - crushBase * 13.0f)));

    const float baseLevels =
        static_cast<float>((1u << baseBits) - 1u);

    const int baseHold =
        1 + static_cast<int>(std::round(decimateBase * 96.0f));

    // Free movement or host-tempo-synchronised movement.
    double bpm = 120.0;

    if (syncRate > 0)
    {
        if (auto* currentPlayHead = getPlayHead())
        {
            if (auto position = currentPlayHead->getPosition())
            {
                if (auto hostBpm = position->getBpm())
                    bpm = *hostBpm;
            }
        }
    }

    float movementRateHz = 0.18f + 5.0f * movement;

    if (syncRate > 0)
    {
        const float divisions[] { 1.0f, 2.0f, 4.0f, 8.0f };
        movementRateHz =
            static_cast<float>(bpm / 60.0)
            * divisions[juce::jlimit(0, 3, syncRate - 1)];
    }

    const float movementIncrement =
        2.0f * pi * movementRateHz
        / static_cast<float>(currentSampleRate);

    const float split1Alpha =
        onePoleAlpha(map01(split, 100.0f, 230.0f), currentSampleRate);

    const float split2Alpha =
        onePoleAlpha(900.0f, currentSampleRate);

    const float split3Alpha =
        onePoleAlpha(3200.0f, currentSampleRate);

    const float filterCutoff =
        juce::jlimit(
            80.0f,
            static_cast<float>(currentSampleRate) * 0.45f,
            filterFreq);

    setFilterFromParameters(
        filterType,
        filterCutoff,
        juce::jlimit(0.05f, 0.95f, filterRes));

    float inputEnergy = 0.0f;
    float processedEnergy = 0.0f;
    float blockPeak = 0.0f;

    for (int sample = 0; sample < samples; ++sample)
    {
        movementPhase += movementIncrement;
        syncPhase += movementIncrement;

        if (movementPhase > 2.0f * pi)
            movementPhase -= 2.0f * pi;

        if (syncPhase > 2.0f * pi)
            syncPhase -= 2.0f * pi;

        if (unstable > 0.001f && unstableRemaining <= 0)
        {
            unstableRemaining =
                220 + static_cast<int>(nextRandom() * 1200.0f);

            unstableValue =
                nextRandom() * 2.0f - 1.0f;
        }

        if (unstableRemaining > 0)
            --unstableRemaining;

        const float move =
            movement > 0.001f
                ? getMovementValue(movementShape, movementPhase)
                : 0.0f;

        const float slowMovement =
            movement * move * 0.32f
            + unstableValue * unstable * 0.16f;

        int gridSlots = 0;

        if (glitchGrid == 1) gridSlots = 8;
        if (glitchGrid == 2) gridSlots = 16;
        if (glitchGrid == 3) gridSlots = 32;

        bool gridBoundary = true;

        if (gridSlots > 0)
        {
            const float wrapped =
                syncPhase / (2.0f * pi)
                - std::floor(syncPhase / (2.0f * pi));

            const int slot =
                static_cast<int>(wrapped * static_cast<float>(gridSlots));

            gridBoundary = slot != lastGlitchGridSlot;

            if (gridBoundary)
                lastGlitchGridSlot = slot;
        }

        for (int ch = 0; ch < channels; ++ch)
        {
            const size_t index = static_cast<size_t>(ch);
            const float dry = buffer.getSample(ch, sample);

            inputEnergy += dry * dry;

            // Four broad bands from inexpensive one-pole crossovers.
            splitLow1[index] =
                (1.0f - split1Alpha) * dry
                + split1Alpha * splitLow1[index];

            splitLow2[index] =
                (1.0f - split2Alpha) * dry
                + split2Alpha * splitLow2[index];

            splitLow3[index] =
                (1.0f - split3Alpha) * dry
                + split3Alpha * splitLow3[index];

            const float low = splitLow1[index];
            const float mid = splitLow2[index] - low;
            const float high = splitLow3[index] - splitLow2[index];
            const float air = dry - splitLow3[index];

            const float envelopeIn = std::abs(dry);

            const float fastCoeff =
                envelopeIn > fastEnvelope[index] ? 0.025f : 0.0012f;

            const float slowCoeff =
                envelopeIn > slowEnvelope[index] ? 0.006f : 0.0004f;

            fastEnvelope[index] +=
                fastCoeff * (envelopeIn - fastEnvelope[index]);

            slowEnvelope[index] +=
                slowCoeff * (envelopeIn - slowEnvelope[index]);

            const float transientAmount =
                clamp01((fastEnvelope[index]
                         - slowEnvelope[index]) * 7.0f);

            const float bodyAmount =
                clamp01(slowEnvelope[index] * 3.5f);

            const float envAmount =
                juce::jmap(envFollow,
                           1.0f,
                           clamp01(0.25f
                                   + transientAmount * 0.85f
                                   + bodyAmount * 0.35f));

            if (holdRemaining[index] <= 0)
            {
                previousHeldSample[index] =
                    heldSample[index];

                heldSample[index] = dry;

                int hold =
                    baseHold;

                if (jitter > 0.001f)
                {
                    const int jitterAmount =
                        static_cast<int>(
                            std::round(
                                hold * jitter * 0.60f));

                    hold += static_cast<int>(
                        (nextRandom() * 2.0f - 1.0f)
                        * static_cast<float>(jitterAmount));
                }

                holdRemaining[index] =
                    juce::jmax(1, hold);

                currentHoldLength[index] =
                    holdRemaining[index];
            }

            const float progress =
                1.0f
                - static_cast<float>(
                    holdRemaining[index])
                / static_cast<float>(
                    juce::jmax(
                        1,
                        currentHoldLength[index]));

            --holdRemaining[index];

            float resampled = heldSample[index];

            switch (resampleMode)
            {
                case 0:
                    resampled = heldSample[index];
                    break;

                case 1:
                    resampled = juce::jmap(
                        progress,
                        previousHeldSample[index],
                        heldSample[index]);
                    break;

                case 2:
                    resampled =
                        progress < 0.5f
                            ? previousHeldSample[index]
                            : heldSample[index];
                    break;

                case 3:
                {
                    const float smear =
                        0.5f + 0.5f
                        * std::sin(
                            progress * pi * 2.0f
                            - pi * 0.5f);

                    resampled = juce::jmap(
                        smear,
                        previousHeldSample[index],
                        heldSample[index]);
                    break;
                }

                default:
                    resampled =
                        nextRandom() < progress
                            ? heldSample[index]
                            : previousHeldSample[index];
                    break;
            }

            // Spectral Shatter: preserve the low band by default and push
            // progressively more processing into mids/highs/air.
            const float baseShatter =
                shatter * intensity * envAmount;

            const float lowAmt =
                clamp01(baseShatter * bandLow);

            const float midAmt =
                clamp01(baseShatter * bandMid);

            const float highAmt =
                clamp01(baseShatter * bandHigh);

            const float airAmt =
                clamp01(baseShatter * bandAir);

            auto destroyBand =
                [this, baseCrush = crushBase, decimateBase,
                 fold, character, resampled, baseLevels]
                (float band, float amount)
            {
                const float localCrush =
                    clamp01(
                        baseCrush
                        * juce::jmap(amount, 0.55f, 1.25f));

                const int bits =
                    juce::jlimit(
                        3, 16,
                        static_cast<int>(
                            std::round(
                                16.0f - localCrush * 13.0f)));

                const float levels =
                    static_cast<float>((1u << bits) - 1u);

                float x =
                    juce::jmap(amount, band, resampled);

                if (decimateBase > 0.001f)
                    x = juce::jmap(
                        amount * decimateBase,
                        x,
                        resampled);

                const float ditherStep =
                    2.0f / juce::jmax(
                        levels, baseLevels);

                x =
                    std::round(
                        (x
                         + tpdfDither(
                             ditherStep
                             * 0.45f
                             * localCrush))
                        * levels)
                    / levels;

                return waveFold(
                    x,
                    fold * amount
                    * juce::jmap(character, 0.7f, 1.35f));
            };

            float x =
                destroyBand(low, lowAmt)
                + destroyBand(mid, midAmt)
                + destroyBand(high, highAmt)
                + destroyBand(air, airAmt);

            // Re-add a controlled amount of the original low end.
            x =
                juce::jmap(
                    lowAmt,
                    low + mid + high + air,
                    x);

            // Global movement and unstable drift.
            x *=
                1.0f + slowMovement
                * (0.15f + shatter * 0.40f);

            // Shift is a sideband-style ring modulation stage. It is deliberately
            // blended rather than replacing the signal.
            if (shift > 0.001f)
            {
                const float carrierHz =
                    25.0f + 1800.0f
                    * shift * shift;

                alienPhase[index] +=
                    2.0f * pi * carrierHz
                    / static_cast<float>(currentSampleRate);

                if (alienPhase[index] > 2.0f * pi)
                    alienPhase[index] -= 2.0f * pi;

                const float carrier =
                    std::sin(alienPhase[index]);

                const float shifted =
                    x * carrier;

                x =
                    juce::jmap(
                        shift * 0.72f,
                        x,
                        shifted);
            }

            if (mode == 5)
                x = std::round(x * 31.0f) / 31.0f;

            if (mode == 6 && alien > 0.001f)
            {
                const float carrier =
                    std::sin(alienPhase[index] * 0.73f);

                x +=
                    x * carrier
                    * alien * 0.90f;
            }

            // Resonator / comb stage.
            if (resonance > 0.001f)
            {
                const int delaySamples =
                    juce::jlimit(
                        16,
                        8000,
                        static_cast<int>(
                            map01(
                                resonance,
                                0.004f,
                                0.095f)
                            * currentSampleRate));

                int readIndex =
                    resonatorWriteIndex
                    - delaySamples;

                while (readIndex < 0)
                    readIndex +=
                        static_cast<int>(
                            resonatorBuffer[index].size());

                const float delayed =
                    resonatorBuffer[index]
                        [static_cast<size_t>(
                            readIndex)];

                const float feedback =
                    0.20f + resonance * 0.70f;

                x +=
                    delayed
                    * resonance
                    * 0.48f;

                resonatorBuffer[index]
                    [static_cast<size_t>(
                        resonatorWriteIndex)] =
                    x
                    + delayed * feedback;
            }

            const float glitchChance =
                glitch
                * (0.03f + intensity * 0.16f)
                * (gridSlots > 0
                    ? (gridBoundary ? 0.018f : 0.0f)
                    : 0.00011f);

            if (glitchCooldown[index] > 0)
                --glitchCooldown[index];

            if (glitchChance > 0.0f
                && glitchRemaining[index] <= 0
                && glitchCooldown[index] <= 0
                && nextRandom() < glitchChance)
            {
                glitchRemaining[index] =
                    24
                    + static_cast<int>(
                        nextRandom() * 400.0f);

                glitchCooldown[index] =
                    700
                    + static_cast<int>(
                        nextRandom() * 4800.0f);

                glitchValue[index] = x;
            }

            if (glitchRemaining[index] > 0)
            {
                const int fadeWindow = 28;

                if (glitchRemaining[index]
                    > fadeWindow)
                {
                    x = glitchValue[index];
                }
                else
                {
                    const float fade =
                        static_cast<float>(
                            glitchRemaining[index])
                        / static_cast<float>(
                            fadeWindow);

                    x =
                        juce::jmap(
                            fade,
                            x,
                            glitchValue[index]);
                }

                --glitchRemaining[index];
            }

            const float cutoffMod =
                movement * move * 0.30f
                + unstableValue * unstable * 0.12f;

            const float modCutoff =
                juce::jlimit(
                    80.0f,
                    static_cast<float>(
                        currentSampleRate) * 0.45f,
                    filterCutoff
                    * std::pow(2.0f, cutoffMod));

            postFilter[index].setCutoffFrequency(
                modCutoff);

            x = postFilter[index]
                    .processSample(0, x);

            // Transients can stay cleaner than the body.
            const float transientDry =
                juce::jmap(
                    transient,
                    0.0f,
                    1.0f,
                    1.0f,
                    0.25f);

            const float bodyDirty =
                juce::jmap(
                    body,
                    0.0f,
                    1.0f,
                    0.35f,
                    1.0f);

            const float dynamicDirty =
                juce::jlimit(
                    0.0f,
                    1.0f,
                    bodyDirty
                    - transientAmount * transientDry);

            x =
                juce::jmap(
                    dynamicDirty * envAmount,
                    dry,
                    x);

            if (channels == 2 && stereo > 0.001f)
            {
                const float sideOffset =
                    stereo
                    * (0.018f + shatter * 0.035f)
                    * (index == 0 ? -1.0f : 1.0f);

                x += sideOffset * move;
            }

            const float destroyed =
                juce::jmap(intensity, dry, x);

            const float out =
                juce::jmap(mix, dry, destroyed);

            buffer.setSample(
                ch,
                sample,
                out);

            processedEnergy += out * out;
            blockPeak =
                juce::jmax(
                    blockPeak,
                    std::abs(out));
        }

        ++resonatorWriteIndex;

        if (resonatorWriteIndex
            >= static_cast<int>(
                resonatorBuffer[0].size()))
        {
            resonatorWriteIndex = 0;
        }
    }

    if (channels == 2 && stereo > 0.001f)
    {
        const float width =
            1.0f + stereo * 0.60f;

        for (int sample = 0; sample < samples; ++sample)
        {
            const float l =
                buffer.getSample(0, sample);

            const float r =
                buffer.getSample(1, sample);

            const float mid =
                (l + r) * 0.5f;

            const float side =
                (l - r) * 0.5f;

            buffer.setSample(
                0,
                sample,
                mid + side * width);

            buffer.setSample(
                1,
                sample,
                mid - side * width);
        }
    }

    if (autoMatch)
    {
        const float inRms =
            std::sqrt(
                inputEnergy
                / static_cast<float>(
                    samples * channels)
                + 1.0e-12f);

        const float outRms =
            std::sqrt(
                processedEnergy
                / static_cast<float>(
                    samples * channels)
                + 1.0e-12f);

        const float target =
            juce::jlimit(
                0.50f,
                2.0f,
                inRms / outRms);

        autoMatchGain +=
            0.08f * (target - autoMatchGain);
    }
    else
    {
        autoMatchGain +=
            0.06f * (1.0f - autoMatchGain);
    }

    const float finalGain =
        juce::Decibels::decibelsToGain(outputDb)
        * autoMatchGain;

    buffer.applyGain(finalGain);

    meterLevel.store(
        juce::jmax(
            blockPeak * finalGain,
            meterLevel.load() * meterRelease));
}

juce::AudioProcessorEditor*
ShakalizerAudioProcessor::createEditor()
{
    return new ShakalizerAudioProcessorEditor(*this);
}

void ShakalizerAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    if (auto xml =
            apvts.copyState().createXml())
    {
        copyXmlToBinary(
            *xml,
            destData);
    }
}

void ShakalizerAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    if (auto xml =
            getXmlFromBinary(
                data,
                sizeInBytes))
    {
        if (xml->hasTagName(
                apvts.state.getType()))
        {
            apvts.replaceState(
                juce::ValueTree::fromXml(*xml));
        }
    }
}

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new ShakalizerAudioProcessor();
}
