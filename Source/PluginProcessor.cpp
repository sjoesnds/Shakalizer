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

void ShakalizerAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    holdRemaining = 0;
    heldSample.fill(0.0f);
    glitchValue.fill(0.0f);
    glitchRemaining.fill(0);
    toneState.fill(0.0f);
    meterLevel.store(0.0f);
}

void ShakalizerAudioProcessor::releaseResources() {}

bool ShakalizerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (input != output) return false;
    return input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
}

float ShakalizerAudioProcessor::nextRandom()
{
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return static_cast<float>(rngState) / static_cast<float>(std::numeric_limits<std::uint32_t>::max());
}

float ShakalizerAudioProcessor::applyWaveshaper(float x, float drive, float clip) const noexcept
{
    const float driveGain = juce::Decibels::decibelsToGain(map01(drive, 0.0f, 30.0f));
    const float driven = x * driveGain;
    const float threshold = map01(clamp01(clip), 1.0f, 0.16f);
    const float clipped = juce::jlimit(-threshold, threshold, driven);
    const float normalized = threshold > 0.0001f ? clipped / threshold : clipped;
    const float shaped = std::tanh(normalized * (1.0f + drive * 4.0f));
    return shaped * (0.85f + 0.15f * threshold);
}

void ShakalizerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels == 0 || samples == 0) return;

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
    const float effectiveCrush = clamp01(crush * (0.35f + 0.85f * intensity));
    const float effectiveDecimate = clamp01(decimate * (0.25f + 0.95f * intensity));
    const float effectiveDrive = clamp01(drive * (0.30f + 0.90f * intensity));
    const float effectiveClip = clamp01(clip * (0.25f + 0.95f * intensity));
    const float effectiveGlitch = clamp01(glitch * (0.15f + 1.10f * intensity));
    const float effectiveJitter = clamp01(jitter * (0.20f + 1.10f * intensity));

    const int bits = juce::jlimit(2, 16, static_cast<int>(std::round(16.0f - effectiveCrush * 14.0f)));
    const float quantLevels = static_cast<float>((1u << bits) - 1u);
    const int baseHold = 1 + static_cast<int>(std::round(effectiveDecimate * 63.0f));
    const int jitterRange = static_cast<int>(std::round(effectiveJitter * baseHold * 0.65f));
    const float cutoff = map01(tone * tone, 500.0f, 19000.0f);
    const float alpha = std::exp(-2.0f * pi * cutoff / static_cast<float>(currentSampleRate));
    const float outputGain = juce::Decibels::decibelsToGain(outputDb);

    float blockPeak = 0.0f;

    for (int sample = 0; sample < samples; ++sample)
    {
        if (holdRemaining <= 0)
        {
            int nextHold = baseHold;
            if (jitterRange > 0)
                nextHold += static_cast<int>((nextRandom() * 2.0f - 1.0f) * static_cast<float>(jitterRange));
            holdRemaining = juce::jmax(1, nextHold);
            for (int ch = 0; ch < channels; ++ch)
                heldSample[static_cast<size_t>(ch)] = buffer.getSample(ch, sample);
        }

        --holdRemaining;

        for (int ch = 0; ch < channels; ++ch)
        {
            const size_t index = static_cast<size_t>(ch);
            const float dry = buffer.getSample(ch, sample);
            float x = heldSample[index];
            x = applyWaveshaper(x, effectiveDrive, effectiveClip);
            x = std::round(x * quantLevels) / quantLevels;

            if (effectiveGlitch > 0.001f)
            {
                auto& remaining = glitchRemaining[index];
                if (remaining <= 0 && nextRandom() < effectiveGlitch * 0.0045f)
                {
                    remaining = 16 + static_cast<int>(nextRandom() * 900.0f);
                    glitchValue[index] = x;
                }
                if (remaining > 0)
                {
                    x = glitchValue[index];
                    if (nextRandom() < 0.15f * effectiveGlitch) x = -x;
                    --remaining;
                }
            }

            toneState[index] = (1.0f - alpha) * x + alpha * toneState[index];
            x = toneState[index];

            const float destroyed = juce::jmap(intensity, 0.0f, 1.0f, dry, x);
            const float out = ((1.0f - mix) * dry + mix * destroyed) * outputGain;
            buffer.setSample(ch, sample, out);
            blockPeak = juce::jmax(blockPeak, std::abs(out));
        }
    }

    meterLevel.store(juce::jmax(blockPeak, meterLevel.load() * meterRelease));
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
