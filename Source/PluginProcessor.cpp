#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

float clamp01(float v)
{
    return juce::jlimit(0.0f, 1.0f, v);
}

float onePoleAlpha(float cutoff, double sampleRate)
{
    return std::exp(
        -2.0f * juce::MathConstants<float>::pi
        * cutoff / static_cast<float>(sampleRate));
}

int choiceIndex(juce::AudioProcessorValueTreeState& state,
                const juce::String& id,
                int fallback)
{
    if (auto* parameter =
            dynamic_cast<const juce::AudioParameterChoice*>(
                state.getParameter(id)))
        return parameter->getIndex();

    return fallback;
}

float softCeiling(float x, float amount)
{
    const float drive = 1.0f + amount * 0.8f;
    return std::tanh(x * drive)
        / std::tanh(drive);
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

    // Core.
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shakal", "Shakal", FloatRange(0.0f, 1.0f, 0.001f), 0.50f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "destroy", "Destroy", FloatRange(0.0f, 1.0f, 0.001f), 0.42f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "crush", "Crush", FloatRange(0.0f, 1.0f, 0.001f), 0.34f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "decimate", "Decimate", FloatRange(0.0f, 1.0f, 0.001f), 0.28f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive", FloatRange(0.0f, 1.0f, 0.001f), 0.30f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "clip", "Clip", FloatRange(0.0f, 1.0f, 0.001f), 0.20f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "glitch", "Glitch", FloatRange(0.0f, 1.0f, 0.001f), 0.08f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "jitter", "Jitter", FloatRange(0.0f, 1.0f, 0.001f), 0.08f));

    // Dynamics / stereo.
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "split", "Split", FloatRange(0.0f, 1.0f, 0.001f), 0.56f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "transient", "Transient", FloatRange(0.0f, 1.0f, 0.001f), 0.74f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "body", "Body", FloatRange(0.0f, 1.0f, 0.001f), 0.55f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stereo", "Stereo", FloatRange(0.0f, 1.0f, 0.001f), 0.18f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "movement", "Movement", FloatRange(0.0f, 1.0f, 0.001f), 0.14f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "unstable", "Unstable", FloatRange(0.0f, 1.0f, 0.001f), 0.06f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "alien", "Alien", FloatRange(0.0f, 1.0f, 0.001f), 0.02f));

    // Output/filter.
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterFreq", "Filter Frequency",
        FloatRange(80.0f, 18000.0f, 1.0f, 0.35f), 14500.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterRes", "Filter Resonance",
        FloatRange(0.05f, 0.95f, 0.001f), 0.30f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix", FloatRange(0.0f, 1.0f, 0.001f), 0.76f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", FloatRange(-12.0f, 6.0f, 0.01f), -1.5f));

    // Shatter engine.
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shatter", "Spectral Shatter",
        FloatRange(0.0f, 1.0f, 0.001f), 0.24f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fold", "Wave Fold",
        FloatRange(0.0f, 1.0f, 0.001f), 0.06f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "shift", "Shift",
        FloatRange(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "resonance", "Resonator",
        FloatRange(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "envFollow", "Envelope",
        FloatRange(0.0f, 1.0f, 0.001f), 0.28f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandLow", "Low Shatter",
        FloatRange(0.0f, 1.0f, 0.001f), 0.10f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandMid", "Mid Shatter",
        FloatRange(0.0f, 1.0f, 0.001f), 0.52f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandHigh", "High Shatter",
        FloatRange(0.0f, 1.0f, 0.001f), 0.68f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bandAir", "Air Shatter",
        FloatRange(0.0f, 1.0f, 0.001f), 0.42f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "character", "Character",
        FloatRange(0.0f, 1.0f, 0.001f), 0.52f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "preGain", "Pre Gain",
        FloatRange(-24.0f, 12.0f, 0.01f), 0.0f));

    // Final anti-harsh control.
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "smooth", "Smooth",
        FloatRange(0.0f, 1.0f, 0.001f), 0.34f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode",
        juce::StringArray {
            "Clean", "Crunch", "Shakal", "Destroy", "Fried",
            "Pixel", "Alien", "Melt", "Shatter"
        }, 2));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "resampleMode", "Resample",
        juce::StringArray {
            "Hold", "Linear", "Stair", "Smear", "Random"
        }, 1));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "filterType", "Filter",
        juce::StringArray {
            "Low Pass", "Band Pass", "High Pass"
        }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "movementShape", "Movement Shape",
        juce::StringArray {
            "Sine", "Triangle", "Sample+Hold", "Stepped"
        }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "quality", "Quality",
        juce::StringArray { "1x", "2x", "4x" }, 2));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "syncRate", "Sync",
        juce::StringArray {
            "Free", "1/4", "1/8", "1/16", "1/32"
        }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "glitchGrid", "Glitch Grid",
        juce::StringArray {
            "Free", "1/8", "1/16", "1/32"
        }, 1));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        "autoMatch", "Auto Match", false));

    return { p.begin(), p.end() };
}

void ShakalizerAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate =
        sampleRate > 0.0 ? sampleRate : 44100.0;

    maxBlockSize =
        juce::jmax(1, samplesPerBlock);

    oversampler2x.reset();
    oversampler4x.reset();

    oversampler2x.initProcessing(
        static_cast<size_t>(maxBlockSize));

    oversampler4x.initProcessing(
        static_cast<size_t>(maxBlockSize));

    dryBuffer.setSize(
        2,
        maxBlockSize,
        false,
        false,
        true);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize =
        static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    for (auto& f : postFilter)
    {
        f.prepare(spec);
        f.reset();
        f.setType(
            juce::dsp::StateVariableTPTFilterType::lowpass);
        f.setCutoffFrequency(14500.0f);
        f.setResonance(0.30f);
    }

    for (auto& f : safetyFilter)
    {
        f.prepare(spec);
        f.reset();
        f.setType(
            juce::dsp::StateVariableTPTFilterType::lowpass);
        f.setCutoffFrequency(18000.0f);
        f.setResonance(0.10f);
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

    resonatorBuffer = {};
    resonatorWriteIndex = 0;

    movementPhase = 0.0f;
    syncPhase = 0.0f;
    movementHoldValue = 0.0f;
    movementHoldCounter = 0;
    lastGlitchGridSlot = -1;

    unstableValue = 0.0f;
    unstableRemaining = 0;

    alienPhase.fill(0.0f);

    autoMatchGain = 1.0f;
    meterLevel.store(0.0f);
}

void ShakalizerAudioProcessor::releaseResources()
{
    oversampler2x.reset();
    oversampler4x.reset();

    for (auto& f : postFilter)
        f.reset();

    for (auto& f : safetyFilter)
        f.reset();
}

bool ShakalizerAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto input =
        layouts.getMainInputChannelSet();

    const auto output =
        layouts.getMainOutputChannelSet();

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
         / static_cast<float>(
             std::numeric_limits<std::uint32_t>::max());
}

float ShakalizerAudioProcessor::tpdfDither(float step) noexcept
{
    return (nextRandom() - nextRandom()) * step;
}

float ShakalizerAudioProcessor::shapedSample(
    float x,
    float drive,
    float clip) const noexcept
{
    const float gain =
        juce::Decibels::decibelsToGain(
            juce::jmap(drive, 0.0f, 24.0f));

    const float pushed = x * gain;

    // Keep the main nonlinearity smooth. Hard clipping is only a small
    // character component, which prevents the high end becoming abrasive.
    const float soft =
        std::tanh(
            pushed * (0.75f + 1.25f * drive));

    const float threshold =
        juce::jmap(clip, 1.05f, 0.48f);

    const float limited =
        juce::jlimit(
            -threshold,
            threshold,
            pushed);

    const float hard =
        std::tanh(
            (limited
             / juce::jmax(0.001f, threshold))
            * 1.9f);

    const float shaped =
        juce::jmap(
            clip * clip * 0.65f,
            soft,
            hard);

    return shaped
        * juce::jmap(drive, 1.0f, 0.68f);
}

float ShakalizerAudioProcessor::waveFold(
    float x,
    float amount) const noexcept
{
    if (amount <= 0.0001f)
        return x;

    const float gain =
        1.0f + amount * 5.0f;

    const float v = x * gain;

    float folded =
        std::fmod(v + 2.0f, 4.0f);

    if (folded < 0.0f)
        folded += 4.0f;

    folded =
        std::abs(folded - 2.0f) - 1.0f;

    return juce::jmap(
        amount * 0.72f,
        x,
        folded);
}

float ShakalizerAudioProcessor::getMovementValue(
    int shape,
    float phase) noexcept
{
    const float cycles =
        phase / (2.0f * pi);

    const float wrapped =
        cycles - std::floor(cycles);

    switch (shape)
    {
        case 1:
        {
            const float t =
                wrapped < 0.5f
                    ? wrapped * 2.0f
                    : 2.0f - wrapped * 2.0f;

            return t * 2.0f - 1.0f;
        }

        case 2:
            return movementHoldValue;

        case 3:
            return std::floor(wrapped * 8.0f)
                 / 3.5f - 1.0f;

        default:
            return std::sin(phase);
    }
}

void ShakalizerAudioProcessor::setFilterFromParameters(
    int type,
    float cutoff,
    float resonance)
{
    auto filterType =
        juce::dsp::StateVariableTPTFilterType::lowpass;

    if (type == 1)
        filterType =
            juce::dsp::StateVariableTPTFilterType::bandpass;
    else if (type == 2)
        filterType =
            juce::dsp::StateVariableTPTFilterType::highpass;

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

    const int channels =
        juce::jmin(2, buffer.getNumChannels());

    const int samples =
        buffer.getNumSamples();

    if (channels == 0 || samples == 0)
        return;

    for (int ch = 0; ch < channels; ++ch)
    {
        dryBuffer.copyFrom(
            ch,
            0,
            buffer,
            ch,
            0,
            samples);
    }

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

    const float filterFreq =
        value("filterFreq");

    const float filterRes =
        clamp01(value("filterRes"));

    const float mix =
        clamp01(value("mix"));

    const float outputDb =
        value("output");

    const float shatter =
        clamp01(value("shatter"));

    const float fold =
        clamp01(value("fold"));

    const float shift =
        clamp01(value("shift"));

    const float resonance =
        clamp01(value("resonance"));

    const float envFollow =
        clamp01(value("envFollow"));

    const float bandLow =
        clamp01(value("bandLow"));

    const float bandMid =
        clamp01(value("bandMid"));

    const float bandHigh =
        clamp01(value("bandHigh"));

    const float bandAir =
        clamp01(value("bandAir"));

    const float character =
        clamp01(value("character"));

    const float preGain =
        juce::jlimit(
            -24.0f,
            12.0f,
            value("preGain"));

    const float smooth =
        clamp01(value("smooth"));

    const bool autoMatch =
        value("autoMatch") > 0.5f;

    const int mode =
        choiceIndex(apvts, "mode", 2);

    const int resampleMode =
        choiceIndex(apvts, "resampleMode", 1);

    const int filterType =
        choiceIndex(apvts, "filterType", 0);

    const int movementShape =
        choiceIndex(apvts, "movementShape", 0);

    const int quality =
        choiceIndex(apvts, "quality", 2);

    const int syncRate =
        choiceIndex(apvts, "syncRate", 0);

    const int glitchGrid =
        choiceIndex(apvts, "glitchGrid", 1);

    const float modeScale = [mode]
    {
        switch (mode)
        {
            case 1: return 0.46f; // Crunch
            case 2: return 0.78f; // Shakal
            case 3: return 1.00f; // Destroy
            case 4: return 1.08f; // Fried
            case 5: return 0.72f; // Pixel
            case 6: return 0.68f; // Alien
            case 7: return 0.90f; // Melt
            case 8: return 0.98f; // Shatter
            default: return 0.18f; // Clean
        }
    }();

    // True SHAKAL macro: it gently pushes several stages at once instead
    // of merely multiplying DESTROY.
    const float macro =
        shakal * modeScale;

    const float intensity =
        clamp01(
            (destroy * (0.55f + 0.55f * macro)
             + shakal * 0.22f)
            * juce::jmap(character, 0.75f, 1.08f));

    const float effectiveDrive =
        clamp01(
            drive * 0.72f
            + shakal * 0.28f
            + intensity * 0.20f);

    const float effectiveClip =
        clamp01(
            clip * 0.76f
            + shakal * 0.20f
            + character * 0.08f);

    const float effectiveCrush =
        clamp01(
            crush * (0.70f + 0.35f * macro)
            + shakal * 0.08f);

    const float effectiveDecimate =
        clamp01(
            decimate * (0.72f + 0.40f * macro)
            + shakal * 0.06f);

    const float preGainAmount =
        juce::Decibels::decibelsToGain(preGain);

    const float driveBase =
        clamp01(
            effectiveDrive
            * (0.50f + intensity * 0.72f));

    const float clipBase =
        clamp01(
            effectiveClip
            * (0.35f + intensity * 0.70f));

    const float crushBase =
        clamp01(
            effectiveCrush
            * (0.20f + intensity * 0.88f));

    const float decimateBase =
        clamp01(
            effectiveDecimate
            * (0.08f + intensity * 0.84f));

    auto nonlinearStage =
        [this, driveBase, clipBase, fold,
         preGainAmount, mode](juce::dsp::AudioBlock<float>& block)
    {
        for (size_t sample = 0;
             sample < block.getNumSamples();
             ++sample)
        {
            for (int ch = 0;
                 ch < static_cast<int>(
                     block.getNumChannels());
                 ++ch)
            {
                float* data =
                    block.getChannelPointer(
                        static_cast<size_t>(ch));

                float x =
                    data[sample] * preGainAmount;

                x =
                    shapedSample(
                        x,
                        driveBase,
                        clipBase);

                const float foldAmount =
                    fold * (mode == 7 ? 0.90f : 0.58f);

                x =
                    waveFold(
                        x,
                        foldAmount);

                data[sample] =
                    softCeiling(
                        x,
                        0.20f + driveBase * 0.20f);
            }
        }
    };

    if (quality == 1)
    {
        const auto input =
            juce::dsp::AudioBlock<const float>(buffer);

        auto up =
            oversampler2x.processSamplesUp(input);

        nonlinearStage(up);

        auto output =
            juce::dsp::AudioBlock<float>(buffer);

        oversampler2x.processSamplesDown(output);
    }
    else if (quality >= 2)
    {
        const auto input =
            juce::dsp::AudioBlock<const float>(buffer);

        auto up =
            oversampler4x.processSamplesUp(input);

        nonlinearStage(up);

        auto output =
            juce::dsp::AudioBlock<float>(buffer);

        oversampler4x.processSamplesDown(output);
    }
    else
    {
        auto block =
            juce::dsp::AudioBlock<float>(buffer);

        nonlinearStage(block);
    }

    const int baseBits =
        juce::jlimit(
            4,
            16,
            static_cast<int>(
                std::round(
                    16.0f
                    - crushBase * 10.0f)));

    const int baseHold =
        1
        + static_cast<int>(
            std::round(
                decimateBase * 72.0f));

    double bpm = 120.0;

    if (syncRate > 0)
    {
        if (auto* currentPlayHead =
                getPlayHead())
        {
            if (auto position =
                    currentPlayHead->getPosition())
            {
                if (auto hostBpm =
                        position->getBpm())
                    bpm = *hostBpm;
            }
        }
    }

    float movementRateHz =
        0.16f
        + 4.0f * movement;

    if (syncRate > 0)
    {
        const float divisions[] {
            1.0f, 2.0f, 4.0f, 8.0f
        };

        movementRateHz =
            static_cast<float>(bpm / 60.0)
            * divisions[
                juce::jlimit(
                    0,
                    3,
                    syncRate - 1)];
    }

    const float movementIncrement =
        2.0f * pi * movementRateHz
        / static_cast<float>(currentSampleRate);

    const float splitAlpha1 =
        onePoleAlpha(
            110.0f + split * 140.0f,
            currentSampleRate);

    const float splitAlpha2 =
        onePoleAlpha(
            850.0f,
            currentSampleRate);

    const float splitAlpha3 =
        onePoleAlpha(
            3000.0f,
            currentSampleRate);

    const float filterCutoff =
        juce::jlimit(
            100.0f,
            static_cast<float>(
                currentSampleRate) * 0.44f,
            filterFreq);

    setFilterFromParameters(
        filterType,
        filterCutoff,
        juce::jlimit(
            0.05f,
            0.82f,
            filterRes));

    // Smooth is deliberately a real safety control: it reduces high-band
    // aggression and softens the final edge without killing the character.
    const float safetyCutoff =
        juce::jlimit(
            4500.0f,
            static_cast<float>(
                currentSampleRate) * 0.46f,
            19500.0f
            - smooth * 10500.0f);

    for (auto& f : safetyFilter)
    {
        f.setType(
            juce::dsp::StateVariableTPTFilterType::lowpass);
        f.setCutoffFrequency(
            safetyCutoff);
        f.setResonance(
            0.08f + smooth * 0.16f);
    }

    float inputEnergy = 0.0f;
    float processedEnergy = 0.0f;
    float blockPeak = 0.0f;

    for (int sample = 0;
         sample < samples;
         ++sample)
    {
        movementPhase += movementIncrement;

        if (movementPhase > 2.0f * pi)
            movementPhase -= 2.0f * pi;

        syncPhase += movementIncrement;

        if (syncPhase > 2.0f * pi)
            syncPhase -= 2.0f * pi;

        if (movementShape == 2)
        {
            if (--movementHoldCounter <= 0)
            {
                movementHoldCounter =
                    2 + static_cast<int>(
                        nextRandom() * 28.0f);

                movementHoldValue =
                    nextRandom() * 2.0f - 1.0f;
            }
        }

        if (unstable > 0.001f
            && unstableRemaining <= 0)
        {
            unstableRemaining =
                240 + static_cast<int>(
                    nextRandom() * 1100.0f);

            unstableValue =
                nextRandom() * 2.0f - 1.0f;
        }

        if (unstableRemaining > 0)
            --unstableRemaining;

        const float movementValue =
            movement > 0.001f
                ? getMovementValue(
                    movementShape,
                    movementPhase)
                : 0.0f;

        const float move =
            movement * movementValue * 0.26f
            + unstableValue
              * unstable * 0.12f;

        int gridSlots = 0;

        if (glitchGrid == 1) gridSlots = 8;
        if (glitchGrid == 2) gridSlots = 16;
        if (glitchGrid == 3) gridSlots = 32;

        bool gridBoundary = true;

        if (gridSlots > 0)
        {
            const float wrapped =
                syncPhase / (2.0f * pi)
                - std::floor(
                    syncPhase / (2.0f * pi));

            const int slot =
                static_cast<int>(
                    wrapped
                    * static_cast<float>(
                        gridSlots));

            gridBoundary =
                slot != lastGlitchGridSlot;

            if (gridBoundary)
                lastGlitchGridSlot = slot;
        }

        for (int ch = 0;
             ch < channels;
             ++ch)
        {
            const size_t index =
                static_cast<size_t>(ch);

            const float dry =
                dryBuffer.getSample(ch, sample);

            inputEnergy += dry * dry;

            splitLow1[index] =
                (1.0f - splitAlpha1)
                    * dry
                + splitAlpha1
                    * splitLow1[index];

            splitLow2[index] =
                (1.0f - splitAlpha2)
                    * dry
                + splitAlpha2
                    * splitLow2[index];

            splitLow3[index] =
                (1.0f - splitAlpha3)
                    * dry
                + splitAlpha3
                    * splitLow3[index];

            const float low =
                splitLow1[index];

            const float mid =
                splitLow2[index] - low;

            const float high =
                splitLow3[index] - splitLow2[index];

            const float air =
                dry - splitLow3[index];

            const float inputAbs =
                std::abs(dry);

            const float fastCoeff =
                inputAbs
                    > fastEnvelope[index]
                    ? 0.026f
                    : 0.0011f;

            const float slowCoeff =
                inputAbs
                    > slowEnvelope[index]
                    ? 0.0055f
                    : 0.00035f;

            fastEnvelope[index] +=
                fastCoeff
                * (inputAbs
                   - fastEnvelope[index]);

            slowEnvelope[index] +=
                slowCoeff
                * (inputAbs
                   - slowEnvelope[index]);

            const float transientAmount =
                clamp01(
                    (fastEnvelope[index]
                     - slowEnvelope[index])
                    * 7.0f);

            const float bodyAmount =
                clamp01(
                    slowEnvelope[index]
                    * 3.2f);

            const float envAmount =
                juce::jmap(
                    envFollow,
                    1.0f,
                    clamp01(
                        0.35f
                        + transientAmount * 0.75f
                        + bodyAmount * 0.30f));

            if (holdRemaining[index] <= 0)
            {
                previousHeldSample[index] =
                    heldSample[index];

                heldSample[index] =
                    dry;

                int hold =
                    baseHold;

                if (jitter > 0.001f)
                {
                    const int jitterAmount =
                        static_cast<int>(
                            std::round(
                                hold
                                * jitter
                                * 0.50f));

                    hold +=
                        static_cast<int>(
                            (nextRandom() * 2.0f
                             - 1.0f)
                            * jitterAmount);
                }

                holdRemaining[index] =
                    juce::jmax(
                        1,
                        hold);

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

            float resampled =
                heldSample[index];

            switch (resampleMode)
            {
                case 0:
                    resampled =
                        heldSample[index];
                    break;

                case 1:
                    resampled =
                        juce::jmap(
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
                        0.5f
                        + 0.5f
                          * std::sin(
                              progress * pi * 2.0f
                              - pi * 0.5f);

                    resampled =
                        juce::jmap(
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

            const float baseShatter =
                shatter
                * intensity
                * envAmount
                * (0.82f + move * 0.18f);

            // Low band stays deliberately calmer.
            const float lowAmt =
                clamp01(
                    baseShatter
                    * bandLow
                    * 0.40f);

            const float midAmt =
                clamp01(
                    baseShatter
                    * bandMid
                    * 0.78f);

            const float highAmt =
                clamp01(
                    baseShatter
                    * bandHigh
                    * 0.90f);

            const float airAmt =
                clamp01(
                    baseShatter
                    * bandAir
                    * (0.48f + 0.34f
                       * (1.0f - smooth)));

            auto processBand =
                [this, crushBase, decimateBase,
                 fold, character, resampled,
                 baseBits](float band, float amount)
            {
                if (amount <= 0.0001f)
                    return band;

                const float crushAmount =
                    clamp01(
                        crushBase
                        * (0.45f + amount * 0.78f));

                const int bits =
                    juce::jlimit(
                        4,
                        16,
                        static_cast<int>(
                            std::round(
                                16.0f
                                - crushAmount
                                  * static_cast<float>(
                                      16 - baseBits))));

                const float levels =
                    static_cast<float>(
                        (1u << bits) - 1u);

                float x =
                    juce::jmap(
                        amount * 0.72f,
                        band,
                        resampled);

                if (decimateBase > 0.001f)
                {
                    x =
                        juce::jmap(
                            amount
                            * decimateBase
                            * 0.55f,
                            x,
                            std::round(x
                                       * levels)
                            / levels);
                }

                const float step =
                    2.0f
                    / juce::jmax(
                        2.0f,
                        levels);

                const float quantized =
                    std::round(
                        (x
                         + tpdfDither(
                             step
                             * 0.35f
                             * crushAmount))
                        * levels)
                    / levels;

                x =
                    juce::jmap(
                        amount * 0.62f,
                        x,
                        quantized);

                x =
                    waveFold(
                        x,
                        fold
                        * amount
                        * juce::jmap(
                            character,
                            0.55f,
                            1.10f));

                return softCeiling(
                    x,
                    0.10f + amount * 0.18f);
            };

            float processed =
                processBand(low, lowAmt)
                + processBand(mid, midAmt)
                + processBand(high, highAmt)
                + processBand(air, airAmt);

            // Protect the lows and keep the sum energy sane.
            const float lowProtection =
                1.0f - lowAmt * 0.72f;

            processed =
                processed
                + low
                  * (1.0f - lowProtection);

            const float movementGain =
                1.0f
                + move
                  * (0.10f
                     + shatter * 0.16f);

            processed *= movementGain;

            if (shift > 0.001f)
            {
                const float carrierHz =
                    20.0f
                    + 1500.0f
                      * shift * shift;

                alienPhase[index] +=
                    2.0f * pi
                    * carrierHz
                    / static_cast<float>(
                        currentSampleRate);

                if (alienPhase[index] > 2.0f * pi)
                    alienPhase[index] -=
                        2.0f * pi;

                const float carrier =
                    std::sin(
                        alienPhase[index]);

                const float shifted =
                    processed
                    * (0.65f
                       + 0.35f
                         * carrier);

                processed =
                    juce::jmap(
                        shift * 0.48f,
                        processed,
                        shifted);
            }

            if (mode == 5)
            {
                processed =
                    juce::jmap(
                        0.35f,
                        processed,
                        std::round(
                            processed * 31.0f)
                        / 31.0f);
            }

            if (mode == 6
                && alien > 0.001f)
            {
                processed +=
                    processed
                    * std::sin(
                        alienPhase[index]
                        * 0.73f)
                    * alien
                    * 0.45f;
            }

            if (resonance > 0.001f)
            {
                const int delaySamples =
                    juce::jlimit(
                        24,
                        7600,
                        static_cast<int>(
                            (0.009f
                             + resonance
                               * 0.075f)
                            * currentSampleRate));

                int readIndex =
                    resonatorWriteIndex
                    - delaySamples;

                const int bufferSize =
                    static_cast<int>(
                        resonatorBuffer[index].size());

                while (readIndex < 0)
                    readIndex += bufferSize;

                const float delayed =
                    resonatorBuffer[index]
                        [static_cast<size_t>(
                            readIndex)];

                const float feedback =
                    0.12f
                    + resonance * 0.42f;

                processed +=
                    delayed
                    * resonance
                    * 0.28f;

                resonatorBuffer[index]
                    [static_cast<size_t>(
                        resonatorWriteIndex)] =
                    processed
                    + delayed * feedback;
            }

            const float glitchChance =
                glitch
                * (0.012f
                   + intensity * 0.05f)
                * (gridSlots > 0
                    ? (gridBoundary ? 0.55f : 0.0f)
                    : 0.00008f);

            if (glitchCooldown[index] > 0)
                --glitchCooldown[index];

            if (glitchChance > 0.0f
                && glitchRemaining[index] <= 0
                && glitchCooldown[index] <= 0
                && nextRandom() < glitchChance)
            {
                glitchRemaining[index] =
                    18 + static_cast<int>(
                        nextRandom() * 220.0f);

                glitchCooldown[index] =
                    800 + static_cast<int>(
                        nextRandom() * 5000.0f);

                glitchValue[index] =
                    processed;
            }

            if (glitchRemaining[index] > 0)
            {
                const float glitchMix =
                    glitchRemaining[index] < 24
                        ? static_cast<float>(
                            glitchRemaining[index])
                          / 24.0f
                        : 1.0f;

                processed =
                    juce::jmap(
                        glitchMix,
                        processed,
                        glitchValue[index]);

                --glitchRemaining[index];
            }

            const float cutoffMod =
                move * 0.22f
                + unstableValue
                  * unstable
                  * 0.08f;

            const float modCutoff =
                juce::jlimit(
                    100.0f,
                    static_cast<float>(
                        currentSampleRate)
                    * 0.44f,
                    filterCutoff
                    * std::pow(
                        2.0f,
                        cutoffMod));

            postFilter[index]
                .setCutoffFrequency(
                    modCutoff);

            float wet =
                postFilter[index]
                    .processSample(
                        0,
                        processed);

            const float transientProtection =
                juce::jmap(
                    transient,
                    0.0f,
                    1.0f,
                    0.52f,
                    0.16f);

            const float bodyAmountFinal =
                juce::jmap(
                    body,
                    0.0f,
                    1.0f,
                    0.28f,
                    0.86f);

            const float dynamicAmount =
                juce::jlimit(
                    0.0f,
                    1.0f,
                    bodyAmountFinal
                    - transientAmount
                      * transientProtection);

            wet =
                juce::jmap(
                    dynamicAmount
                    * envAmount,
                    dry,
                    wet);

            if (channels == 2
                && stereo > 0.001f)
            {
                const float decor =
                    stereo
                    * (0.006f
                       + shatter * 0.012f);

                wet +=
                    (index == 0
                        ? -decor
                        : decor)
                    * move;
            }

            // Smooth is also a character control: the more it is turned up,
            // the more of the original transient is retained.
            const float smoothBlend =
                juce::jmap(
                    smooth,
                    1.0f,
                    0.72f);

            wet =
                juce::jmap(
                    smooth * 0.22f,
                    wet,
                    dry);

            const float destroyed =
                juce::jmap(
                    intensity
                    * dynamicAmount
                    * smoothBlend,
                    dry,
                    wet);

            const float out =
                juce::jmap(
                    mix,
                    dry,
                    destroyed);

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

    // A restrained stereo field; no per-sample random pan noise.
    if (channels == 2
        && stereo > 0.001f)
    {
        const float width =
            1.0f + stereo * 0.38f;

        for (int sample = 0;
             sample < samples;
             ++sample)
        {
            const float left =
                buffer.getSample(
                    0,
                    sample);

            const float right =
                buffer.getSample(
                    1,
                    sample);

            const float mid =
                (left + right) * 0.5f;

            const float side =
                (left - right) * 0.5f;

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
        const float inputRms =
            std::sqrt(
                inputEnergy
                / static_cast<float>(
                    samples * channels)
                + 1.0e-12f);

        const float outputRms =
            std::sqrt(
                processedEnergy
                / static_cast<float>(
                    samples * channels)
                + 1.0e-12f);

        const float target =
            juce::jlimit(
                0.58f,
                1.65f,
                inputRms
                / outputRms);

        autoMatchGain +=
            0.045f
            * (target
               - autoMatchGain);
    }
    else
    {
        autoMatchGain +=
            0.035f
            * (1.0f
               - autoMatchGain);
    }

    const float finalGain =
        juce::Decibels::decibelsToGain(
            outputDb)
        * autoMatchGain;

    buffer.applyGain(finalGain);

    // Final safety stage: tame sharp peaks and ultrasonic-ish edge without
    // turning the entire effect into a brickwall limiter.
    for (int ch = 0;
         ch < channels;
         ++ch)
    {
        for (int sample = 0;
             sample < samples;
             ++sample)
        {
            float x =
                buffer.getSample(
                    ch,
                    sample);

            x =
                safetyFilter[
                    static_cast<size_t>(ch)]
                    .processSample(0, x);

            x =
                softCeiling(
                    x,
                    0.32f
                    + smooth * 0.55f);

            x *= 0.96f;

            buffer.setSample(
                ch,
                sample,
                x);
        }
    }

    meterLevel.store(
        juce::jlimit(
            0.0f,
            1.0f,
            juce::jmax(
                blockPeak * finalGain
                * 0.96f,
                meterLevel.load()
                * 0.92f)));
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
