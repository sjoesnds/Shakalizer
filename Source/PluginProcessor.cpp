#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

float map01(float value, float min, float max)
{
    return min + value * (max - min);
}

float clamp01(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float safeQuantise(float x, float levels, float dither) noexcept
{
    if (levels <= 1.0f)
        return x;

    x += dither;
    return std::round(x * levels) / levels;
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
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("destroy", "Destroy", Range(0.0f, 1.0f, 0.001f), 0.72f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crush", "Crush", Range(0.0f, 1.0f, 0.001f), 0.68f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("decimate", "Decimate", Range(0.0f, 1.0f, 0.001f), 0.62f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Drive", Range(0.0f, 1.0f, 0.001f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("clip", "Clip", Range(0.0f, 1.0f, 0.001f), 0.48f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("glitch", "Glitch", Range(0.0f, 1.0f, 0.001f), 0.22f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("jitter", "Jitter", Range(0.0f, 1.0f, 0.001f), 0.16f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tone", "Tone", Range(0.0f, 1.0f, 0.001f), 0.72f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", Range(0.0f, 1.0f, 0.001f), 0.86f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", Range(-12.0f, 6.0f, 0.01f), -1.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("mode", "Mode",
        juce::StringArray { "Clean", "Crunch", "Shakal", "Destroy", "Fried" }, 2));

    return { params.begin(), params.end() };
}

void ShakalizerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxBlockSize = juce::jmax(1, samplesPerBlock);

    oversampler.reset();
    oversampler.initProcessing(static_cast<size_t>(maxBlockSize));

    holdRemaining = 0;
    currentHoldLength = 1;
    heldSample.fill(0.0f);
    previousHeldSample.fill(0.0f);
    glitchValue.fill(0.0f);
    glitchRemaining.fill(0);
    glitchCooldown.fill(0);
    toneState.fill(0.0f);
    dcState.fill(0.0f);
    meterLevel.store(0.0f);
}

void ShakalizerAudioProcessor::releaseResources()
{
    oversampler.reset();
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
    // Triangular-PDF dither keeps the crushed signal from collapsing into
    // unpleasant deterministic quantisation patterns at medium settings.
    const float a = nextRandom();
    const float b = nextRandom();
    return (a - b) * step;
}

float ShakalizerAudioProcessor::shapedSample(float x, float drive, float clip) const noexcept
{
    const float driveGain = juce::Decibels::decibelsToGain(map01(drive, 0.0f, 32.0f));
    const float pushed = x * driveGain;

    // Soft stage keeps transients coherent while the hard stage adds the
    // unmistakable digital edge as CLIP is increased.
    const float soft = std::tanh(pushed * (0.95f + drive * 1.65f));

    const float threshold = map01(clamp01(clip), 0.95f, 0.22f);
    const float limited = juce::jlimit(-threshold, threshold, pushed);
    const float hard = std::tanh((limited / juce::jmax(0.001f, threshold)) * 3.0f)
                     * (0.82f + threshold * 0.18f);

    const float hardMix = clip * clip;
    const float shaped = juce::jmap(hardMix, soft, hard);

    // Mild compensation keeps high DRIVE from just becoming louder.
    const float compensation = juce::jmap(drive, 1.0f, 0.50f);
    return shaped * compensation;
}

float ShakalizerAudioProcessor::applyWaveshaper(float x, float drive, float clip) const noexcept
{
    return shapedSample(x, drive, clip);
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

    const float destroy = clamp01(apvts.getRawParameterValue("destroy")->load());
    const float crush = clamp01(apvts.getRawParameterValue("crush")->load());
    const float decimate = clamp01(apvts.getRawParameterValue("decimate")->load());
    const float drive = clamp01(apvts.getRawParameterValue("drive")->load());
    const float clip = clamp01(apvts.getRawParameterValue("clip")->load());
    const float glitch = clamp01(apvts.getRawParameterValue("glitch")->load());
    const float jitter = clamp01(apvts.getRawParameterValue("jitter")->load());
    const float tone = clamp01(apvts.getRawParameterValue("tone")->load());
    const float mix = clamp01(apvts.getRawParameterValue("mix")->load());
    const float outputDb = apvts.getRawParameterValue("output")->load();
    const int mode = static_cast<int>(apvts.getRawParameterValue("mode")->load());

    const float modeScale = juce::jmap(static_cast<float>(mode), 0.0f, 4.0f, 0.25f, 1.35f);
    const float intensity = clamp01(destroy * modeScale);

    const float effectiveCrush = clamp01(crush * (0.25f + 0.95f * intensity));
    const float effectiveDecimate = clamp01(decimate * (0.10f + 0.90f * intensity));
    const float effectiveDrive = clamp01(drive * (0.35f + 0.90f * intensity));
    const float effectiveClip = clamp01(clip * (0.30f + 0.95f * intensity));
    const float effectiveGlitch = clamp01(glitch * (0.10f + 0.95f * intensity));
    const float effectiveJitter = clamp01(jitter * (0.15f + 0.95f * intensity));

    // Drive is oversampled. This is the part that most benefits from higher
    // internal sample rate because nonlinear processing generates harmonics.
    const auto inputBlock = juce::dsp::AudioBlock<const float>(buffer);
    auto oversampledBlock = oversampler.processSamplesUp(inputBlock);

    const auto oversampledSamples = oversampledBlock.getNumSamples();
    for (size_t sample = 0; sample < oversampledSamples; ++sample)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
            const float driveInput = data[sample];

            // Only the nonlinear stage is oversampled. Decimation/quantisation
            // happens after returning to the host rate on purpose.
            data[sample] = applyWaveshaper(driveInput, effectiveDrive, effectiveClip);
        }
    }

    auto outputBlock = juce::dsp::AudioBlock<float>(buffer);
    oversampler.processSamplesDown(outputBlock);

    const int bits = juce::jlimit(
        3, 16,
        static_cast<int>(std::round(16.0f - effectiveCrush * 13.0f)));

    const float quantLevels = static_cast<float>((1u << bits) - 1u);

    // A little less extreme than the original 1..64 sample staircase.
    // At maximum DECIMATE this reaches roughly 2 ms at 44.1 kHz.
    const int baseHold = 1 + static_cast<int>(
        std::round(effectiveDecimate * 88.0f));

    const int jitterRange = static_cast<int>(
        std::round(effectiveJitter * baseHold * 0.55f));

    const float cutoff = map01(tone * tone, 700.0f, 19000.0f);
    const float alpha = std::exp(
        -2.0f * pi * cutoff / static_cast<float>(currentSampleRate));

    const float outputGain = juce::Decibels::decibelsToGain(outputDb);

    // At lower destruction the stair-step is interpolated; at high destruction
    // it becomes progressively more sample-and-hold-like.
    const float holdStyle = effectiveDecimate * effectiveDecimate;

    float blockPeak = 0.0f;

    for (int sample = 0; sample < samples; ++sample)
    {
        if (holdRemaining <= 0)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                const auto index = static_cast<size_t>(ch);
                previousHeldSample[index] = heldSample[index];
                heldSample[index] = buffer.getSample(ch, sample);
            }

            int nextHold = baseHold;
            if (jitterRange > 0)
            {
                nextHold += static_cast<int>(
                    (nextRandom() * 2.0f - 1.0f) * static_cast<float>(jitterRange));
            }

            holdRemaining = juce::jmax(1, nextHold);
            currentHoldLength = holdRemaining;
        }

        const float progress = 1.0f
            - static_cast<float>(holdRemaining)
            / static_cast<float>(juce::jmax(1, currentHoldLength));

        --holdRemaining;

        for (int ch = 0; ch < channels; ++ch)
        {
            const auto index = static_cast<size_t>(ch);
            const float dry = buffer.getSample(ch, sample);

            const float interpolated = juce::jmap(
                progress, previousHeldSample[index], heldSample[index]);

            float x = juce::jmap(
                holdStyle, interpolated, heldSample[index]);

            const float quantStep = 2.0f / quantLevels;
            x = safeQuantise(x, quantLevels,
                             tpdfDither(quantStep * 0.55f * effectiveCrush));

            if (glitchCooldown[index] > 0)
                --glitchCooldown[index];

            if (effectiveGlitch > 0.001f
                && glitchRemaining[index] <= 0
                && glitchCooldown[index] <= 0
                && nextRandom() < effectiveGlitch * 0.00012f)
            {
                glitchRemaining[index] =
                    32 + static_cast<int>(nextRandom() * 520.0f);
                glitchCooldown[index] =
                    700 + static_cast<int>(nextRandom() * 3800.0f);
                glitchValue[index] = x;
            }

            if (glitchRemaining[index] > 0)
            {
                const int total = 32; // short attack/release window
                const int remaining = glitchRemaining[index];

                float glitchMix = 1.0f;
                if (remaining < total)
                    glitchMix = static_cast<float>(remaining) / static_cast<float>(total);

                if (remaining > 0 && remaining <= 520)
                {
                    const float edge = juce::jlimit(0.0f, 1.0f, glitchMix);
                    x = juce::jmap(edge, x, glitchValue[index]);
                }

                --glitchRemaining[index];
            }

            // Gentle post filter keeps the high-frequency spray under control.
            toneState[index] =
                (1.0f - alpha) * x + alpha * toneState[index];

            x = toneState[index];

            // Very small DC protection. This only reacts to the accumulated
            // offset; normal audio is left effectively untouched.
            const float corrected = x - dcState[index] * 0.995f;
            dcState[index] = x;

            const float destroyed =
                juce::jmap(intensity, 0.0f, 1.0f, dry, corrected);

            const float out =
                ((1.0f - mix) * dry + mix * destroyed) * outputGain;

            buffer.setSample(ch, sample, out);
            blockPeak = juce::jmax(blockPeak, std::abs(out));
        }
    }

    meterLevel.store(
        juce::jmax(blockPeak, meterLevel.load() * meterRelease));
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
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// JUCE VST3 factory entry point.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShakalizerAudioProcessor();
}
