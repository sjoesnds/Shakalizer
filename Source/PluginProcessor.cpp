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

int choiceIndex(juce::AudioProcessorValueTreeState& state, const juce::String& id, int fallback)
{
    if (auto* parameter = dynamic_cast<const juce::AudioParameterChoice*>(state.getParameter(id)))
        return parameter->getIndex();
    return fallback;
}
}

ShakalizerAudioProcessor::ShakalizerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ShakalizerAudioProcessor::createParameterLayout()
{
    using FloatRange = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterFloat>("shakal", "Shakal", FloatRange(0.0f, 1.0f, 0.001f), 0.65f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("destroy", "Destroy", FloatRange(0.0f, 1.0f, 0.001f), 0.68f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("crush", "Crush", FloatRange(0.0f, 1.0f, 0.001f), 0.58f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("decimate", "Decimate", FloatRange(0.0f, 1.0f, 0.001f), 0.48f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Drive", FloatRange(0.0f, 1.0f, 0.001f), 0.46f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("clip", "Clip", FloatRange(0.0f, 1.0f, 0.001f), 0.42f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("glitch", "Glitch", FloatRange(0.0f, 1.0f, 0.001f), 0.14f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("jitter", "Jitter", FloatRange(0.0f, 1.0f, 0.001f), 0.12f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("split", "Split", FloatRange(0.0f, 1.0f, 0.001f), 0.60f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("transient", "Transient", FloatRange(0.0f, 1.0f, 0.001f), 0.72f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("body", "Body", FloatRange(0.0f, 1.0f, 0.001f), 0.68f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("stereo", "Stereo", FloatRange(0.0f, 1.0f, 0.001f), 0.35f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("movement", "Movement", FloatRange(0.0f, 1.0f, 0.001f), 0.22f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("unstable", "Unstable", FloatRange(0.0f, 1.0f, 0.001f), 0.12f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("alien", "Alien", FloatRange(0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("filterFreq", "Filter Frequency", FloatRange(80.0f, 18000.0f, 1.0f, 0.35f), 14500.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("filterRes", "Filter Resonance", FloatRange(0.05f, 0.95f, 0.001f), 0.42f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", FloatRange(0.0f, 1.0f, 0.001f), 0.86f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", FloatRange(-12.0f, 6.0f, 0.01f), -1.0f));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode",
        juce::StringArray { "Clean", "Crunch", "Shakal", "Destroy", "Fried", "Pixel", "Alien", "Melt" }, 2));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "resampleMode", "Resample",
        juce::StringArray { "Hold", "Linear", "Stair", "Smear", "Random" }, 1));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "filterType", "Filter",
        juce::StringArray { "Low Pass", "Band Pass", "High Pass" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        "movementShape", "Movement Shape",
        juce::StringArray { "Sine", "Triangle", "Sample+Hold", "Stepped" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        "autoMatch", "Auto Match", false));

    return { p.begin(), p.end() };
}

void ShakalizerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxBlockSize = juce::jmax(1, samplesPerBlock);

    oversampler.reset();
    oversampler.initProcessing(static_cast<size_t>(maxBlockSize));

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    for (auto& filter : postFilter)
    {
        filter.prepare(spec);
        filter.reset();
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setCutoffFrequency(14500.0f);
        filter.setResonance(0.42f);
    }

    holdRemaining.fill(0);
    currentHoldLength.fill(1);
    heldSample.fill(0.0f);
    previousHeldSample.fill(0.0f);
    splitLowState.fill(0.0f);
    fastEnvelope.fill(0.0f);
    slowEnvelope.fill(0.0f);
    glitchValue.fill(0.0f);
    glitchRemaining.fill(0);
    glitchCooldown.fill(0);
    alienPhase.fill(0.0f);

    movementPhase = 0.0f;
    unstableValue = 0.0f;
    unstableRemaining = 0;
    autoMatchGain = 1.0f;
    meterLevel.store(0.0f);
}

void ShakalizerAudioProcessor::releaseResources()
{
    oversampler.reset();
    for (auto& filter : postFilter)
        filter.reset();
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

float ShakalizerAudioProcessor::shapedSample(float x, float drive, float clip) const noexcept
{
    const float gain = juce::Decibels::decibelsToGain(juce::jmap(drive, 0.0f, 30.0f));
    const float pushed = x * gain;

    const float soft = std::tanh(pushed * (0.95f + 1.8f * drive));
    const float threshold = juce::jmap(clip, 0.98f, 0.20f);
    const float hardInput = juce::jlimit(-threshold, threshold, pushed);
    const float hard = std::tanh((hardInput / juce::jmax(0.001f, threshold)) * 2.8f);

    const float shape = juce::jmap(clip * clip, soft, hard);
    const float compensation = juce::jmap(drive, 1.0f, 0.56f);
    return shape * compensation;
}

float ShakalizerAudioProcessor::getMovementValue(int shape, float phase) noexcept
{
    const float p = phase / (2.0f * pi);
    const float wrapped = p - std::floor(p);

    switch (shape)
    {
        case 1:
        {
            const float t = wrapped < 0.5f ? wrapped * 2.0f : 2.0f - wrapped * 2.0f;
            return t * 2.0f - 1.0f;
        }
        case 2:
        {
            if (static_cast<int>(phase * 8.0f) != static_cast<int>((phase - 0.002f) * 8.0f))
                unstableValue = nextRandom() * 2.0f - 1.0f;
            return unstableValue;
        }
        case 3:
        {
            return std::floor(wrapped * 8.0f) / 3.5f - 1.0f;
        }
        default:
            return std::sin(phase);
    }
}

void ShakalizerAudioProcessor::setFilterFromParameters(int type, float cutoff, float resonance)
{
    juce::dsp::StateVariableTPTFilterType filterTypeValue =
        juce::dsp::StateVariableTPTFilterType::lowpass;

    if (type == 1)
        filterTypeValue = juce::dsp::StateVariableTPTFilterType::bandpass;
    else if (type == 2)
        filterTypeValue = juce::dsp::StateVariableTPTFilterType::highpass;

    for (auto& filter : postFilter)
    {
        filter.setType(filterTypeValue);
        filter.setCutoffFrequency(cutoff);
        filter.setResonance(resonance);
    }
}

void ShakalizerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();

    if (channels == 0 || samples == 0)
        return;

    const float shakal = clamp01(apvts.getRawParameterValue("shakal")->load());
    const float destroy = clamp01(apvts.getRawParameterValue("destroy")->load());
    const float crush = clamp01(apvts.getRawParameterValue("crush")->load());
    const float decimate = clamp01(apvts.getRawParameterValue("decimate")->load());
    const float drive = clamp01(apvts.getRawParameterValue("drive")->load());
    const float clip = clamp01(apvts.getRawParameterValue("clip")->load());
    const float glitch = clamp01(apvts.getRawParameterValue("glitch")->load());
    const float jitter = clamp01(apvts.getRawParameterValue("jitter")->load());
    const float split = clamp01(apvts.getRawParameterValue("split")->load());
    const float transient = clamp01(apvts.getRawParameterValue("transient")->load());
    const float body = clamp01(apvts.getRawParameterValue("body")->load());
    const float stereo = clamp01(apvts.getRawParameterValue("stereo")->load());
    const float movement = clamp01(apvts.getRawParameterValue("movement")->load());
    const float unstable = clamp01(apvts.getRawParameterValue("unstable")->load());
    const float alien = clamp01(apvts.getRawParameterValue("alien")->load());
    const float filterFreq = apvts.getRawParameterValue("filterFreq")->load();
    const float filterRes = clamp01(apvts.getRawParameterValue("filterRes")->load());
    const float mix = clamp01(apvts.getRawParameterValue("mix")->load());
    const float outputDb = apvts.getRawParameterValue("output")->load();
    const bool autoMatch = apvts.getRawParameterValue("autoMatch")->load() > 0.5f;

    const int mode = choiceIndex(apvts, "mode", 2);
    const int resampleMode = choiceIndex(apvts, "resampleMode", 1);
    const int filterType = choiceIndex(apvts, "filterType", 0);
    const int movementShape = choiceIndex(apvts, "movementShape", 0);

    const float modeScale =
        juce::jmap(static_cast<float>(mode), 0.0f, 7.0f, 0.20f, 1.45f);

    const float macro = 0.55f + 0.85f * shakal;

    float intensityBase = clamp01(destroy * macro * modeScale);

    if (mode == 5) intensityBase = clamp01(intensityBase * 1.15f);
    if (mode == 6) intensityBase = clamp01(intensityBase * 1.05f);
    if (mode == 7) intensityBase = clamp01(intensityBase * 1.25f);

    const float driveBase = clamp01(drive * (0.35f + intensityBase * 0.95f));
    const float clipBase = clamp01(clip * (0.25f + intensityBase));
    const float crushBase = clamp01(crush * (0.20f + 0.95f * intensityBase));
    const float decimateBase = clamp01(decimate * (0.08f + 0.95f * intensityBase));

    const auto inputBlock = juce::dsp::AudioBlock<const float>(buffer);
    auto oversampledBlock = oversampler.processSamplesUp(inputBlock);

    for (size_t sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
            float x = data[sample];

            if (mode == 6 && alien > 0.001f)
            {
                const float carrier =
                    std::sin(alienPhase[static_cast<size_t>(ch)]) * alien;
                x *= (1.0f - 0.65f * alien) + 0.65f * carrier;
            }

            data[sample] = shapedSample(x, driveBase, clipBase);
        }
    }

    auto outputBlock = juce::dsp::AudioBlock<float>(buffer);
    oversampler.processSamplesDown(outputBlock);

    const int bits = juce::jlimit(
        3, 16, static_cast<int>(std::round(16.0f - crushBase * 13.0f)));

    const float quantLevels = static_cast<float>((1u << bits) - 1u);
    const int baseHold = 1 + static_cast<int>(std::round(decimateBase * 96.0f));

    const float filterMotion = 1.0f + movement * 0.45f;
    float previousInputEnergy = 0.0f;
    float processedEnergy = 0.0f;

    const float movementRateHz = 0.18f + 4.6f * movement * filterMotion;
    const float movementIncrement =
        2.0f * pi * movementRateHz / static_cast<float>(currentSampleRate);

    const float filterCutoff = juce::jlimit(
        80.0f,
        static_cast<float>(currentSampleRate) * 0.45f,
        filterFreq);

    setFilterFromParameters(filterType, filterCutoff, juce::jlimit(0.05f, 0.95f, filterRes));

    for (int sample = 0; sample < samples; ++sample)
    {
        movementPhase += movementIncrement;
        if (movementPhase > 2.0f * pi)
            movementPhase -= 2.0f * pi;

        float movementValue = 0.0f;
        if (movement > 0.001f)
            movementValue = getMovementValue(movementShape, movementPhase);

        if (unstable > 0.001f)
        {
            if (unstableRemaining <= 0)
            {
                unstableRemaining =
                    200 + static_cast<int>(nextRandom() * 1100.0f);
                unstableValue = nextRandom() * 2.0f - 1.0f;
            }

            --unstableRemaining;
        }

        const float unstableMod = unstable * unstableValue;
        const float modulation =
            1.0f + movement * movementValue * 0.42f + unstableMod * 0.20f;

        for (int ch = 0; ch < channels; ++ch)
        {
            const size_t index = static_cast<size_t>(ch);
            const float dry = buffer.getSample(ch, sample);

            previousInputEnergy += dry * dry;

            const float splitCutoff = map01(split, 140.0f, 900.0f);
            const float splitAlpha =
                std::exp(-2.0f * pi * splitCutoff
                         / static_cast<float>(currentSampleRate));

            splitLowState[index] =
                (1.0f - splitAlpha) * dry
                + splitAlpha * splitLowState[index];

            const float low = splitLowState[index];
            const float high = dry - low;

            const float inEnvelope = std::abs(dry);
            const float fastCoeff = inEnvelope > fastEnvelope[index] ? 0.025f : 0.0015f;
            const float slowCoeff = inEnvelope > slowEnvelope[index] ? 0.006f : 0.0005f;

            fastEnvelope[index] +=
                fastCoeff * (inEnvelope - fastEnvelope[index]);

            slowEnvelope[index] +=
                slowCoeff * (inEnvelope - slowEnvelope[index]);

            const float transientAmount =
                clamp01((fastEnvelope[index] - slowEnvelope[index]) * 7.0f);

            const float bodyAmount =
                clamp01(slowEnvelope[index] * 3.5f);

            const float region = clamp01(
                transient * transientAmount + body * bodyAmount + 0.20f);

            const float localIntensity =
                clamp01(intensityBase * region * modulation);

            const int localBits = juce::jlimit(
                3, 16,
                static_cast<int>(
                    std::round(16.0f - clamp01(crushBase * (0.75f + 0.35f * localIntensity)) * 13.0f)));

            const float localLevels =
                static_cast<float>((1u << localBits) - 1u);

            float lowProcess = low;
            float highProcess = high;

            if (holdRemaining[index] <= 0)
            {
                previousHeldSample[index] = heldSample[index];
                heldSample[index] = dry;

                int hold = baseHold;

                if (jitter > 0.001f)
                {
                    const int jitterRange =
                        static_cast<int>(std::round(
                            hold * jitter * stereo * 0.55f
                            + hold * jitter * 0.35f));

                    hold += static_cast<int>(
                        (nextRandom() * 2.0f - 1.0f)
                        * static_cast<float>(jitterRange));
                }

                holdRemaining[index] = juce::jmax(1, hold);
                currentHoldLength[index] = holdRemaining[index];
            }

            const float progress =
                1.0f - static_cast<float>(holdRemaining[index])
                       / static_cast<float>(juce::jmax(1, currentHoldLength[index]));

            --holdRemaining[index];

            float resampled = heldSample[index];

            switch (resampleMode)
            {
                case 0:
                    resampled = heldSample[index];
                    break;

                case 1:
                    resampled = juce::jmap(
                        progress, previousHeldSample[index], heldSample[index]);
                    break;

                case 2:
                    resampled = progress < 0.5f
                        ? previousHeldSample[index]
                        : heldSample[index];
                    break;

                case 3:
                {
                    const float smear =
                        0.5f + 0.5f
                        * std::sin(progress * pi * 2.0f - pi * 0.5f);
                    resampled = juce::jmap(
                        smear, previousHeldSample[index], heldSample[index]);
                    break;
                }

                default:
                    resampled = nextRandom() < progress
                        ? heldSample[index]
                        : previousHeldSample[index];
                    break;
            }

            float x =
                lowProcess
                + (resampled - lowProcess) * juce::jmap(split, 1.0f, 0.05f);

            if (mode == 7)
            {
                const float melt =
                    0.5f + 0.5f * std::sin(movementPhase * 0.33f);
                x = juce::jmap(melt * 0.55f, x, heldSample[index]);
            }

            const float ditherStep = 2.0f / localLevels;
            x = std::round(
                (x + tpdfDither(ditherStep * 0.50f * crushBase))
                * localLevels) / localLevels;

            if (mode == 5)
            {
                x = std::round(x * 31.0f) / 31.0f;
            }

            if (mode == 6 && alien > 0.001f)
            {
                const float carrier =
                    std::sin(alienPhase[index])
                    * (0.25f + 0.75f * alien);

                x *= 1.0f - 0.55f * alien;
                x += x * carrier * 0.85f;
            }

            const float glitchProbability =
                effectiveGlitch > 0.0f
                    ? (glitch * (0.05f + intensityBase) * 0.00010f)
                    : 0.0f;

            if (glitchCooldown[index] > 0)
                --glitchCooldown[index];

            if (glitchProbability > 0.0f
                && glitchRemaining[index] <= 0
                && glitchCooldown[index] <= 0
                && nextRandom() < glitchProbability)
            {
                glitchRemaining[index] =
                    24 + static_cast<int>(nextRandom() * 420.0f);

                glitchCooldown[index] =
                    900 + static_cast<int>(nextRandom() * 4200.0f);

                glitchValue[index] = x;
            }

            if (glitchRemaining[index] > 0)
            {
                const int fadeWindow = 28;
                if (glitchRemaining[index] > fadeWindow)
                {
                    x = glitchValue[index];
                }
                else
                {
                    const float fade =
                        static_cast<float>(glitchRemaining[index])
                        / static_cast<float>(fadeWindow);

                    x = juce::jmap(fade, x, glitchValue[index]);
                }

                --glitchRemaining[index];
            }

            const float filterMod =
                movement * movementValue * 0.30f
                + unstableMod * 0.14f;

            const float modCutoff =
                juce::jlimit(
                    80.0f,
                    static_cast<float>(currentSampleRate) * 0.45f,
                    filterCutoff
                    * std::pow(2.0f, filterMod));

            postFilter[index].setCutoffFrequency(modCutoff);
            postFilter[index].setResonance(
                juce::jlimit(0.05f, 0.95f, filterRes));

            x = postFilter[index].processSample(0, x);

            const float destroyed =
                juce::jmap(localIntensity, dry,
                           low * (1.0f - split)
                           + x);

            const float mixed =
                juce::jmap(mix, dry, destroyed);

            buffer.setSample(ch, sample, mixed);
            processedEnergy += mixed * mixed;

            if (alien > 0.001f)
            {
                const float alienFreq =
                    35.0f + 1600.0f * alien * alien;
                alienPhase[index] +=
                    2.0f * pi * alienFreq
                    / static_cast<float>(currentSampleRate);

                if (alienPhase[index] > 2.0f * pi)
                    alienPhase[index] -= 2.0f * pi;
            }
        }
    }

    if (channels == 2 && stereo > 0.001f)
    {
        const float width =
            1.0f + stereo * 0.55f;

        for (int sample = 0; sample < samples; ++sample)
        {
            const float left = buffer.getSample(0, sample);
            const float right = buffer.getSample(1, sample);

            const float mid = (left + right) * 0.5f;
            const float side = (left - right) * 0.5f;

            const float divergence =
                stereo * 0.06f
                * (nextRandom() * 2.0f - 1.0f);

            buffer.setSample(
                0, sample, mid + side * width + divergence);

            buffer.setSample(
                1, sample, mid - side * width - divergence);
        }
    }

    if (autoMatch)
    {
        const float inputRms =
            std::sqrt(previousInputEnergy
                       / static_cast<float>(samples * channels) + 1.0e-12f);

        const float outputRms =
            std::sqrt(processedEnergy
                       / static_cast<float>(samples * channels) + 1.0e-12f);

        const float target =
            juce::jlimit(0.50f, 2.0f, inputRms / outputRms);

        autoMatchGain += 0.08f * (target - autoMatchGain);
    }
    else
    {
        autoMatchGain += 0.06f * (1.0f - autoMatchGain);
    }

    const float outputGain =
        juce::Decibels::decibelsToGain(outputDb) * autoMatchGain;

    for (int ch = 0; ch < channels; ++ch)
        buffer.applyGain(ch, 0, samples, outputGain);

    float peak = 0.0f;
    for (int ch = 0; ch < channels; ++ch)
        peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, samples));

    meterLevel.store(juce::jmax(peak, meterLevel.load() * meterRelease));
}

juce::AudioProcessorEditor* ShakalizerAudioProcessor::createEditor()
{
    return new ShakalizerAudioProcessorEditor(*this);
}

void ShakalizerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void ShakalizerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShakalizerAudioProcessor();
}
