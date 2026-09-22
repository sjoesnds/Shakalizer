#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

float clamp01(float v)
{
    return juce::jlimit(0.0f, 1.0f, v);
}

int choiceIndex(juce::AudioProcessorValueTreeState& state,
                const juce::String& id,
                int fallback)
{
    if (auto* parameter =
            dynamic_cast<const juce::AudioParameterChoice*>(
                state.getParameter(id)))
    {
        return parameter->getIndex();
    }

    return fallback;
}

float onePoleAlpha(float cutoff, double sampleRate)
{
    return std::exp(
        -2.0f * pi
        * cutoff
        / static_cast<float>(sampleRate));
}

float softCeiling(float x, float amount)
{
    const float strength = 1.0f + amount * 1.2f;
    return std::tanh(x * strength) / std::tanh(strength);
}

float bipolarToUnit(float x)
{
    return 0.5f + 0.5f * juce::jlimit(-1.0f, 1.0f, x);
}
}

ShakalizerAudioProcessor::ShakalizerAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput(
                "Input",
                juce::AudioChannelSet::stereo(),
                true)
            .withOutput(
                "Output",
                juce::AudioChannelSet::stereo(),
                true)),
      apvts(
          *this,
          nullptr,
          "PARAMETERS",
          createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
ShakalizerAudioProcessor::createParameterLayout()
{
    using FloatRange =
        juce::NormalisableRange<float>;

    std::vector<
        std::unique_ptr<
            juce::RangedAudioParameter>> p;

    // Core.
    const auto addFloat =
        [&p](const char* id,
             const char* name,
             float min,
             float max,
             float step,
             float value)
    {
        p.push_back(
            std::make_unique<
                juce::AudioParameterFloat>(
                    id,
                    name,
                    FloatRange(
                        min,
                        max,
                        step),
                    value));
    };

    addFloat("shakal", "Shakal", 0, 1, 0.001f, 0.50f);
    addFloat("destroy", "Destroy", 0, 1, 0.001f, 0.42f);
    addFloat("crush", "Crush", 0, 1, 0.001f, 0.34f);
    addFloat("decimate", "Decimate", 0, 1, 0.001f, 0.28f);
    addFloat("drive", "Drive", 0, 1, 0.001f, 0.30f);
    addFloat("clip", "Clip", 0, 1, 0.001f, 0.20f);
    addFloat("glitch", "Glitch", 0, 1, 0.001f, 0.08f);
    addFloat("jitter", "Jitter", 0, 1, 0.001f, 0.08f);
    addFloat("glitchDensity", "Glitch Density", 0, 1, 0.001f, 0.32f);
    addFloat("glitchProbability", "Glitch Probability", 0, 1, 0.001f, 0.42f);
    addFloat("glitchFade", "Glitch Fade", 0, 1, 0.001f, 0.56f);
    addFloat("glitchVariation", "Glitch Variation", 0, 1, 0.001f, 0.28f);

    addFloat("split", "Split", 0, 1, 0.001f, 0.56f);
    addFloat("transient", "Transient", 0, 1, 0.001f, 0.74f);
    addFloat("body", "Body", 0, 1, 0.001f, 0.55f);
    addFloat("stereo", "Stereo", 0, 1, 0.001f, 0.18f);
    addFloat("movement", "Movement", 0, 1, 0.001f, 0.14f);
    addFloat("unstable", "Unstable", 0, 1, 0.001f, 0.06f);
    addFloat("alien", "Alien", 0, 1, 0.001f, 0.02f);

    addFloat("filterFreq", "Filter Frequency",
             80.0f, 18000.0f, 1.0f, 14500.0f);
    addFloat("filterRes", "Filter Resonance",
             0.05f, 0.95f, 0.001f, 0.30f);
    addFloat("mix", "Mix", 0, 1, 0.001f, 0.76f);
    addFloat("output", "Output", -12, 6, 0.01f, -1.5f);

    // Spectral / character.
    addFloat("shatter", "Spectral Shatter", 0, 1, 0.001f, 0.24f);
    addFloat("fold", "Wave Fold", 0, 1, 0.001f, 0.06f);
    addFloat("shift", "Shift", 0, 1, 0.001f, 0.0f);
    addFloat("resonance", "Resonator", 0, 1, 0.001f, 0.0f);
    addFloat("envFollow", "Envelope", 0, 1, 0.001f, 0.28f);
    addFloat("bandLow", "Low Shatter", 0, 1, 0.001f, 0.10f);
    addFloat("bandMid", "Mid Shatter", 0, 1, 0.001f, 0.52f);
    addFloat("bandHigh", "High Shatter", 0, 1, 0.001f, 0.68f);
    addFloat("bandAir", "Air Shatter", 0, 1, 0.001f, 0.42f);
    addFloat("spectralMix", "Spectral Mix", 0, 1, 0.001f, 0.72f);
    addFloat("spectralSmear", "Spectral Smear", 0, 1, 0.001f, 0.16f);
    addFloat("spectralFreezeAmount", "Spectral Freeze", 0, 1, 0.001f, 0.0f);
    addFloat("spectralBits", "Spectral Bits", 0, 1, 0.001f, 0.08f);
    addFloat("spectralRing", "Spectral Ring", 0, 1, 0.001f, 0.0f);
    addFloat("character", "Character", 0, 1, 0.001f, 0.52f);
    addFloat("preGain", "Pre Gain", -24, 12, 0.01f, 0.0f);
    addFloat("smooth", "Smooth", 0, 1, 0.001f, 0.34f);

    // Modulation matrix.
    for (int i = 1; i <= 4; ++i)
        addFloat(
            ("mod" + juce::String(i) + "Amount").toRawUTF8(),
            ("Mod " + juce::String(i) + " Amount").toRawUTF8(),
            -1.0f, 1.0f, 0.001f, 0.0f);

    addFloat("morph", "Morph", 0, 1, 0.001f, 0.0f);
    addFloat("modRate", "Mod Rate", 0.05f, 20.0f, 0.01f, 1.25f);
    addFloat("modDepth", "Mod Depth", 0, 1, 0.001f, 0.70f);
    addFloat("modSmooth", "Mod Smooth", 0, 1, 0.001f, 0.54f);
    addFloat("smartAmount", "Smart Amount", 0, 1, 0.001f, 0.72f);
    addFloat("smartBassProtect", "Smart Bass Protect", 0, 1, 0.001f, 0.76f);
    addFloat("smartTransientProtect", "Smart Transient Protect", 0, 1, 0.001f, 0.72f);
    addFloat("smartHighControl", "Smart High Control", 0, 1, 0.001f, 0.68f);

    // v3 destruction workstation.
    addFloat("fftMix", "FFT Mix", 0, 1, 0.001f, 0.0f);
    addFloat("fftShatter", "FFT Shatter", 0, 1, 0.001f, 0.0f);
    addFloat("fftFreeze", "FFT Freeze", 0, 1, 0.001f, 0.0f);
    addFloat("fftBits", "FFT Bits", 0, 1, 0.001f, 0.0f);
    addFloat("fftShift", "FFT Shift", -1, 1, 0.001f, 0.0f);
    addFloat("grainMix", "Grain Mix", 0, 1, 0.001f, 0.0f);
    addFloat("grainSize", "Grain Size", 0, 1, 0.001f, 0.28f);
    addFloat("grainPitch", "Grain Pitch", 0, 1, 0.001f, 0.50f);
    addFloat("grainJitter", "Grain Jitter", 0, 1, 0.001f, 0.10f);
    addFloat("feedback", "Feedback", 0, 1, 0.001f, 0.0f);
    addFloat("feedbackTone", "Feedback Tone", 0, 1, 0.001f, 0.48f);
    addFloat("feedbackDrive", "Feedback Drive", 0, 1, 0.001f, 0.16f);
    addFloat("pitchChaos", "Pitch Chaos", 0, 1, 0.001f, 0.0f);
    addFloat("timelineMix", "Timeline Mix", 0, 1, 0.001f, 0.0f);

    for (int i = 1; i <= 8; ++i)
        addFloat(
            ("timelineStep" + juce::String(i)).toRawUTF8(),
            ("Timeline " + juce::String(i)).toRawUTF8(),
            0.0f, 1.0f, 0.001f,
            i == 1 || i == 5 ? 0.85f : 0.0f);

    for (int i = 5; i <= 8; ++i)
        addFloat(
            ("mod" + juce::String(i) + "Amount").toRawUTF8(),
            ("Mod " + juce::String(i) + " Amount").toRawUTF8(),
            -1.0f, 1.0f, 0.001f, 0.0f);

    // v4 adaptive, granular, feedback and routing controls.
    addFloat("fftSpread", "FFT Spread", 0, 1, 0.001f, 0.18f);
    addFloat("fftThreshold", "FFT Threshold", 0, 1, 0.001f, 0.0f);
    addFloat("fftWarp", "FFT Warp", -1, 1, 0.001f, 0.0f);

    addFloat("grainDensity", "Grain Density", 0, 1, 0.001f, 0.18f);
    addFloat("grainPosition", "Grain Position", 0, 1, 0.001f, 0.50f);
    addFloat("grainSpray", "Grain Spray", 0, 1, 0.001f, 0.08f);
    addFloat("grainReverse", "Grain Reverse", 0, 1, 0.001f, 0.0f);
    addFloat("grainPan", "Grain Pan", 0, 1, 0.001f, 0.50f);

    addFloat("feedbackTime", "Feedback Time", 0, 1, 0.001f, 0.22f);
    addFloat("feedbackDiffusion", "Feedback Diffusion", 0, 1, 0.001f, 0.12f);
    addFloat("feedbackFreeze", "Feedback Freeze", 0, 1, 0.001f, 0.0f);
    addFloat("feedbackSpread", "Feedback Spread", 0, 1, 0.001f, 0.18f);
    addFloat("feedbackPitch", "Feedback Pitch", 0, 1, 0.001f, 0.50f);

    addFloat("pitchDamage", "Pitch Damage", 0, 1, 0.001f, 0.0f);
    addFloat("pitchRange", "Pitch Range", 0, 1, 0.001f, 0.32f);
    addFloat("pitchDrift", "Pitch Drift", 0, 1, 0.001f, 0.0f);

    addFloat("reactiveAmount", "Reactive Amount", 0, 1, 0.001f, 0.0f);
    addFloat("reactiveTransient", "Reactive Transient", 0, 1, 0.001f, 0.55f);
    addFloat("reactiveSpectral", "Reactive Spectral", 0, 1, 0.001f, 0.38f);
    addFloat("reactiveBass", "Reactive Bass", 0, 1, 0.001f, 0.22f);
    addFloat("reactiveHigh", "Reactive High", 0, 1, 0.001f, 0.44f);

    addFloat("macroCurve", "Macro Curve", 0, 1, 0.001f, 0.50f);
    addFloat("sceneMorphTime", "Scene Morph Time", 0, 1, 0.001f, 0.50f);

    addFloat("damageMacro", "Damage", 0, 1, 0.001f, 0.0f);
    addFloat("motionMacro", "Motion", 0, 1, 0.001f, 0.0f);
    addFloat("chaosMacro", "Chaos", 0, 1, 0.001f, 0.0f);
    addFloat("spaceMacro", "Space", 0, 1, 0.001f, 0.0f);
    // v7 intelligent cleanup / performance layer.
    addFloat("antiNoise", "Anti Noise", 0, 1, 0.001f, 0.46f);
    addFloat("antiDc", "DC Guard", 0, 1, 0.001f, 0.70f);
    addFloat("antiAir", "Air Guard", 0, 1, 0.001f, 0.62f);
    addFloat("antiPeak", "Peak Guard", 0, 1, 0.001f, 0.72f);
    addFloat("humanRandom", "Human Random", 0, 1, 0.001f, 0.68f);
    addFloat("audioAware", "Audio Aware", 0, 1, 0.001f, 0.66f);
    addFloat("chaosShape", "Chaos Shape", 0, 1, 0.001f, 0.58f);

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "fftWindow",
                "FFT Window",
                juce::StringArray {
                    "Hann", "Blackman", "Triangle", "Rect"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "pitchMode",
                "Pitch Mode",
                juce::StringArray {
                    "Micro", "Semitone", "Octave", "Corrupt"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "routingTopology",
                "Routing Topology",
                juce::StringArray {
                    "Serial", "Parallel", "Split", "Crossfade",
                    "Feedback Loop", "Wide"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "characterMode",
                "Character Mode",
                juce::StringArray {
                    "Neutral", "Digital", "VHS", "Console",
                    "Radio", "Metallic", "Broken", "Alien",
                    "Cheap DAC", "Corrupt"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "modWave",
                "Mod Wave",
                juce::StringArray {
                    "Sine", "Triangle",
                    "Sample+Hold", "Stepped"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "modSync",
                "Mod Sync",
                juce::StringArray {
                    "Free", "1/4", "1/8",
                    "1/16", "1/32"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "mode",
                "Mode",
                juce::StringArray {
                    "Clean", "Crunch", "Shakal",
                    "Destroy", "Fried", "Pixel",
                    "Alien", "Melt", "Shatter"
                },
                2));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "resampleMode",
                "Resample",
                juce::StringArray {
                    "Hold", "Linear", "Stair",
                    "Smear", "Random"
                },
                1));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "filterType",
                "Filter",
                juce::StringArray {
                    "Low Pass", "Band Pass",
                    "High Pass"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "movementShape",
                "Movement Shape",
                juce::StringArray {
                    "Sine", "Triangle",
                    "Sample+Hold", "Stepped"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "quality",
                "Quality",
                juce::StringArray {
                    "1x", "2x", "4x"
                },
                2));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "syncRate",
                "Sync",
                juce::StringArray {
                    "Free", "1/4", "1/8",
                    "1/16", "1/32"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "glitchGrid",
                "Glitch Grid",
                juce::StringArray {
                    "Free", "1/8",
                    "1/16", "1/32"
                },
                1));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "glitchMode",
                "Glitch Mode",
                juce::StringArray {
                    "Freeze", "Stutter", "Repeat",
                    "Tape Stop", "Gate", "Reverse",
                    "Beat Chop"
                },
                1));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "glitchLength",
                "Glitch Length",
                juce::StringArray {
                    "1/64", "1/32", "1/16",
                    "1/8", "1/4", "1/2"
                },
                2));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "glitchPattern",
                "Glitch Pattern",
                juce::StringArray {
                    "Auto", "Straight", "Offbeat",
                    "Syncopated", "Sparse", "Dense", "Burst"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "spectralMode",
                "Spectral Mode",
                juce::StringArray {
                    "Smooth", "Shatter", "Blur",
                    "Freeze", "Bits", "Ring"
                },
                1));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "routing",
                "Routing",
                juce::StringArray {
                    "Standard",
                    "Damage > Shatter",
                    "Shatter > Damage",
                    "Parallel"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "msMode",
                "M/S",
                juce::StringArray {
                    "Stereo", "Mid", "Side", "Split"
                },
                0));

    p.push_back(
        std::make_unique<
            juce::AudioParameterChoice>(
                "liveScene",
                "Live Scene",
                juce::StringArray {
                    "Normal", "Impact", "Glitch",
                    "Melt", "Broken", "Chaos"
                },
                0));

    const juce::StringArray modSources {
        "Off", "LFO", "Envelope",
        "Random", "Step", "Beat"
    };

    const juce::StringArray modDestinations {
        "None", "Shakal", "Destroy",
        "Crush", "Decimate", "Shatter",
        "Fold", "Shift", "Glitch", "Filter"
    };

    for (int i = 1; i <= 8; ++i)
    {
        p.push_back(
            std::make_unique<
                juce::AudioParameterChoice>(
                    "mod" + juce::String(i) + "Source",
                    "Mod " + juce::String(i) + " Source",
                    modSources,
                    0));

        p.push_back(
            std::make_unique<
                juce::AudioParameterChoice>(
                    "mod" + juce::String(i) + "Dest",
                    "Mod " + juce::String(i) + " Dest",
                    modDestinations,
                    0));
    }

    p.push_back(
        std::make_unique<
            juce::AudioParameterBool>(
                "autoMatch",
                "Auto Match",
                false));

    p.push_back(
        std::make_unique<
            juce::AudioParameterBool>(
                "smart",
                "Smart",
                false));

    return {
        p.begin(),
        p.end()
    };
}

void ShakalizerAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate =
        sampleRate > 0.0
            ? sampleRate
            : 44100.0;

    maxBlockSize =
        juce::jmax(
            1,
            samplesPerBlock);

    oversampler2x.reset();
    oversampler4x.reset();

    oversampler2x.initProcessing(
        static_cast<size_t>(
            maxBlockSize));

    oversampler4x.initProcessing(
        static_cast<size_t>(
            maxBlockSize));

    dryBuffer.setSize(
        2,
        maxBlockSize,
        false,
        false,
        true);

    fftWetBuffer.setSize(
        2,
        maxBlockSize,
        false,
        false,
        true);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate =
        currentSampleRate;
    spec.maximumBlockSize =
        static_cast<juce::uint32>(
            maxBlockSize);
    spec.numChannels = 1;

    for (auto& filter : postFilter)
    {
        filter.prepare(spec);
        filter.reset();
        filter.setType(
            juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setCutoffFrequency(14500.0f);
        filter.setResonance(0.30f);
    }

    for (auto& filter : safetyFilter)
    {
        filter.prepare(spec);
        filter.reset();
        filter.setType(
            juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setCutoffFrequency(18000.0f);
        filter.setResonance(0.08f);
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
    glitchEventLength.fill(1);
    glitchEventAge.fill(0);
    glitchBuffer = {};
    glitchWriteIndex = 0;

    spectralFreeze.fill(0.0f);
    modSmoothState.fill(0.0f);
    modPhase = 0.0f;
    modHoldValue = 0.0f;
    modHoldCounter = 0;

    for (auto& sample : scopeBuffer)
        sample.store(0.0f);

    scopeWriteIndex.store(0);

    for (auto& band : bandLevels)
        band.store(0.0f);

    for (auto& bin : spectrumBuffer)
        bin.store(0.0f);

    glitchActivity.store(0.0f);
    modulationActivity.store(0.0f);
    cpuLoad.store(0.0f);

    resonatorBuffer = {};
    resonatorWriteIndex = 0;
    grainBuffer = {};
    grainPhase.fill(0.0f);
    grainWriteIndex = 0;
    feedbackState.fill(0.0f);
    feedbackToneState.fill(0.0f);
    feedbackBuffer = {};
    feedbackWriteIndex = 0;
    fftFrozenMagnitude = {};
    fftData.fill(0.0f);
    fftSource.fill(0.0f);

    movementPhase = 0.0f;
    syncPhase = 0.0f;
    movementHoldValue = 0.0f;
    movementHoldCounter = 0;
    lastGlitchGridSlot = -1;

    unstableValue = 0.0f;
    unstableRemaining = 0;
    alienPhase.fill(0.0f);

    autoMatchGain = 1.0f;
    morphPhase = 0.0f;
    dcBlockState.fill(0.0f);
    performHeldSample.fill(0.0f);
    performTrigger.store(0);
    performType = 0;
    performRemaining = 0;
    meterLevel.store(0.0f);
}

void ShakalizerAudioProcessor::releaseResources()
{
    oversampler2x.reset();
    oversampler4x.reset();

    for (auto& filter : postFilter)
        filter.reset();

    for (auto& filter : safetyFilter)
        filter.reset();

    grainBuffer = {};
    feedbackState.fill(0.0f);
    feedbackToneState.fill(0.0f);
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

float ShakalizerAudioProcessor::tpdfDither(
    float step) noexcept
{
    return (nextRandom()
            - nextRandom())
           * step;
}

float ShakalizerAudioProcessor::shapedSample(
    float x,
    float drive,
    float clip) const noexcept
{
    const float gain =
        juce::Decibels::decibelsToGain(
            juce::jmap(
                drive,
                0.0f,
                24.0f));

    const float pushed =
        x * gain;

    const float soft =
        std::tanh(
            pushed
            * (0.72f
               + 1.22f * drive));

    const float threshold =
        juce::jmap(
            clip,
            1.05f,
            0.50f);

    const float clipped =
        juce::jlimit(
            -threshold,
            threshold,
            pushed);

    const float hard =
        std::tanh(
            (clipped
             / juce::jmax(
                 0.001f,
                 threshold))
            * 1.85f);

    const float blend =
        clip * clip * 0.55f;

    const float shape =
        juce::jmap(
            blend,
            soft,
            hard);

    return shape
        * juce::jmap(
            drive,
            1.0f,
            0.70f);
}

float ShakalizerAudioProcessor::waveFold(
    float x,
    float amount) const noexcept
{
    if (amount <= 0.0001f)
        return x;

    const float gain =
        1.0f
        + amount * 4.2f;

    const float v =
        x * gain;

    float folded =
        std::fmod(
            v + 2.0f,
            4.0f);

    if (folded < 0.0f)
        folded += 4.0f;

    folded =
        std::abs(
            folded - 2.0f)
        - 1.0f;

    return juce::jmap(
        amount * 0.68f,
        x,
        folded);
}

float ShakalizerAudioProcessor::movementValue(
    int shape,
    float phase) const noexcept
{
    const float cycles =
        phase / (2.0f * pi);

    const float wrapped =
        cycles
        - std::floor(cycles);

    switch (shape)
    {
        case 1:
        {
            const float t =
                wrapped < 0.5f
                    ? wrapped * 2.0f
                    : 2.0f
                      - wrapped * 2.0f;

            return t * 2.0f - 1.0f;
        }

        case 2:
            return movementHoldValue;

        case 3:
            return std::floor(
                       wrapped * 8.0f)
                   / 3.5f
                   - 1.0f;

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

    for (auto& filter : postFilter)
    {
        filter.setType(filterType);
        filter.setCutoffFrequency(cutoff);
        filter.setResonance(resonance);
    }
}

void ShakalizerAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    const double blockStartMs =
        juce::Time::getMillisecondCounterHiRes();

    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int channels =
        juce::jmin(
            2,
            buffer.getNumChannels());

    const int samples =
        buffer.getNumSamples();

    if (channels <= 0
        || samples <= 0)
        return;

    for (int ch = 0;
         ch < channels;
         ++ch)
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
        if (auto* p =
                apvts.getRawParameterValue(id))
            return p->load();

        return 0.0f;
    };

    float shakal = clamp01(value("shakal"));
    float destroy = clamp01(value("destroy"));
    float crush = clamp01(value("crush"));
    float decimate = clamp01(value("decimate"));
    float drive = clamp01(value("drive"));
    float clip = clamp01(value("clip"));
    float glitch = clamp01(value("glitch"));
    float jitter = clamp01(value("jitter"));
    const float glitchDensity = clamp01(value("glitchDensity"));
    const float glitchProbability = clamp01(value("glitchProbability"));
    const float glitchFade = clamp01(value("glitchFade"));
    const float glitchVariation = clamp01(value("glitchVariation"));

    const float split =
        clamp01(value("split"));
    const float transient =
        clamp01(value("transient"));
    const float body =
        clamp01(value("body"));
    float stereo =
        clamp01(value("stereo"));
    float movement =
        clamp01(value("movement"));
    float unstable =
        clamp01(value("unstable"));
    const float alien =
        clamp01(value("alien"));

    const float filterFreq =
        value("filterFreq");
    const float filterRes =
        clamp01(value("filterRes"));
    const float mix =
        clamp01(value("mix"));
    const float outputDb =
        value("output");

    float shatter =
        clamp01(value("shatter"));
    float fold =
        clamp01(value("fold"));
    float shift =
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
    const float spectralMix = clamp01(value("spectralMix"));
    const float spectralSmear = clamp01(value("spectralSmear"));
    const float spectralFreezeAmount = clamp01(value("spectralFreezeAmount"));
    const float spectralBits = clamp01(value("spectralBits"));
    const float spectralRing = clamp01(value("spectralRing"));
    const float character =
        clamp01(value("character"));
    const float preGain =
        juce::jlimit(
            -24.0f,
            12.0f,
            value("preGain"));
    const float smooth =
        clamp01(value("smooth"));

    float morph =
        clamp01(value("morph"));

    const float modRate =
        juce::jlimit(0.05f, 20.0f, value("modRate"));
    const float modDepth =
        clamp01(value("modDepth"));
    const float modSmooth =
        clamp01(value("modSmooth"));

    const float smartAmount =
        clamp01(value("smartAmount"));
    const float smartBassProtect =
        clamp01(value("smartBassProtect"));
    const float smartTransientProtect =
        clamp01(value("smartTransientProtect"));
    const float smartHighControl =
        clamp01(value("smartHighControl"));

    const float fftMix = clamp01(value("fftMix"));
    const float fftShatter = clamp01(value("fftShatter"));
    const float fftFreeze = clamp01(value("fftFreeze"));
    const float fftBits = clamp01(value("fftBits"));
    const float fftShift = juce::jlimit(-1.0f, 1.0f, value("fftShift"));
    const float grainMix = clamp01(value("grainMix"));
    const float grainSize = clamp01(value("grainSize"));
    const float grainPitch = clamp01(value("grainPitch"));
    const float grainJitter = clamp01(value("grainJitter"));
    float feedback = clamp01(value("feedback"));
    const float feedbackTone = clamp01(value("feedbackTone"));
    const float feedbackDrive = clamp01(value("feedbackDrive"));
    float pitchChaos = clamp01(value("pitchChaos"));
    const float timelineMix = clamp01(value("timelineMix"));

    const float fftSpread = clamp01(value("fftSpread"));
    const float fftThreshold = clamp01(value("fftThreshold"));
    const float fftWarp = juce::jlimit(-1.0f, 1.0f, value("fftWarp"));
    const float grainDensity = clamp01(value("grainDensity"));
    const float grainPosition = clamp01(value("grainPosition"));
    const float grainSpray = clamp01(value("grainSpray"));
    const float grainReverse = clamp01(value("grainReverse"));
    const float feedbackTime = clamp01(value("feedbackTime"));
    float feedbackDiffusion = clamp01(value("feedbackDiffusion"));
    const float feedbackFreeze = clamp01(value("feedbackFreeze"));
    float feedbackSpread = clamp01(value("feedbackSpread"));
    const float feedbackPitch = clamp01(value("feedbackPitch"));
    const float pitchDamage = clamp01(value("pitchDamage"));
    const float pitchRange = clamp01(value("pitchRange"));
    const float pitchDrift = clamp01(value("pitchDrift"));
    const float reactiveAmount = clamp01(value("reactiveAmount"));
    const float reactiveTransient = clamp01(value("reactiveTransient"));
    const float reactiveSpectral = clamp01(value("reactiveSpectral"));
    const float reactiveBass = clamp01(value("reactiveBass"));
    const float reactiveHigh = clamp01(value("reactiveHigh"));
    const float macroCurve = clamp01(value("macroCurve"));
    const float antiNoise = clamp01(value("antiNoise"));
    const float antiDc = clamp01(value("antiDc"));
    const float antiAir = clamp01(value("antiAir"));
    const float antiPeak = clamp01(value("antiPeak"));
    const float humanRandom = clamp01(value("humanRandom"));
    const float audioAware = clamp01(value("audioAware"));
    const float chaosShape = clamp01(value("chaosShape"));
    float damageMacro = clamp01(value("damageMacro"));
    float motionMacro = clamp01(value("motionMacro"));
    float chaosMacro = clamp01(value("chaosMacro"));
    float spaceMacro = clamp01(value("spaceMacro"));
    const float timelineSteps[8] {
        clamp01(value("timelineStep1")), clamp01(value("timelineStep2")),
        clamp01(value("timelineStep3")), clamp01(value("timelineStep4")),
        clamp01(value("timelineStep5")), clamp01(value("timelineStep6")),
        clamp01(value("timelineStep7")), clamp01(value("timelineStep8"))
    };

    const int characterMode =
        choiceIndex(
            apvts,
            "characterMode",
            0);

    const int fftWindow =
        choiceIndex(apvts, "fftWindow", 0);

    const int pitchMode =
        choiceIndex(apvts, "pitchMode", 0);

    const int routingTopology =
        choiceIndex(apvts, "routingTopology", 0);

    const int modWave =
        choiceIndex(
            apvts,
            "modWave",
            0);

    const int modSync =
        choiceIndex(
            apvts,
            "modSync",
            0);

    const bool autoMatch =
        value("autoMatch") > 0.5f;
    const bool smart =
        value("smart") > 0.5f;

    const int mode =
        choiceIndex(
            apvts,
            "mode",
            2);

    const int resampleMode =
        choiceIndex(
            apvts,
            "resampleMode",
            1);

    const int filterType =
        choiceIndex(
            apvts,
            "filterType",
            0);

    const int movementShape =
        choiceIndex(
            apvts,
            "movementShape",
            0);

    const int quality =
        choiceIndex(
            apvts,
            "quality",
            2);

    const int syncRate =
        choiceIndex(
            apvts,
            "syncRate",
            0);

    const int glitchGrid =
        choiceIndex(
            apvts,
            "glitchGrid",
            1);

    const int glitchMode =
        choiceIndex(
            apvts,
            "glitchMode",
            1);

    const int glitchLength =
        choiceIndex(
            apvts,
            "glitchLength",
            2);

    const int glitchPattern =
        choiceIndex(
            apvts,
            "glitchPattern",
            0);

    const int spectralMode =
        choiceIndex(
            apvts,
            "spectralMode",
            1);

    const int routing =
        choiceIndex(
            apvts,
            "routing",
            0);

    const int msMode =
        choiceIndex(
            apvts,
            "msMode",
            0);

    const int liveScene =
        choiceIndex(
            apvts,
            "liveScene",
            0);

    // Macro mapping.
    const float modeScale =
        [mode]
        {
            switch (mode)
            {
                case 1: return 0.45f;
                case 2: return 0.78f;
                case 3: return 0.95f;
                case 4: return 1.06f;
                case 5: return 0.72f;
                case 6: return 0.66f;
                case 7: return 0.90f;
                case 8: return 0.98f;
                default: return 0.16f;
            }
        }();

    // SHAKAL is the master character macro. Its curve changes how
    // quickly the destruction ramps up and it also feeds several
    // secondary processors instead of controlling intensity only.
    const float macroExponent =
        juce::jmap(
            macroCurve,
            1.55f,
            0.62f);

    const float macroShaped =
        std::pow(
            juce::jlimit(
                0.0f,
                1.0f,
                shakal),
            macroExponent);

    float macro =
        macroShaped
        * (0.72f + 0.28f * modeScale);

    macro =
        juce::jmap(
            morph,
            macro,
            juce::jmin(
                1.0f,
                macro + 0.35f));

    // Macro coupling: higher SHAKAL progressively opens the destructive
    // path while keeping low values useful and relatively clean.
    destroy =
        juce::jlimit(
            0.0f,
            1.0f,
            destroy + macroShaped * 0.24f);

    crush =
        juce::jlimit(
            0.0f,
            1.0f,
            crush + macroShaped * macroShaped * 0.16f);

    decimate =
        juce::jlimit(
            0.0f,
            1.0f,
            decimate + macroShaped * 0.12f);

    drive =
        juce::jlimit(
            0.0f,
            1.0f,
            drive + macroShaped * 0.10f);

    clip =
        juce::jlimit(
            0.0f,
            1.0f,
            clip + macroShaped * 0.08f);

    shatter =
        juce::jlimit(
            0.0f,
            1.0f,
            shatter + macroShaped * 0.18f);

    glitch =
        juce::jlimit(
            0.0f,
            1.0f,
            glitch + macroShaped * 0.07f);

    fold =
        juce::jlimit(
            0.0f,
            1.0f,
            fold + macroShaped * 0.06f);

    shift =
        juce::jlimit(
            0.0f,
            1.0f,
            shift + macroShaped * 0.045f);

    const float damageBoost =
        std::pow(damageMacro, 0.78f);
    const float motionBoost =
        std::pow(motionMacro, 0.72f);
    const float chaosExponent =
        juce::jmap(chaosShape, 1.55f, 0.48f);
    const float chaosBoost =
        std::pow(chaosMacro, chaosExponent);
    const float spaceBoost =
        std::pow(spaceMacro, 0.80f);

    destroy = clamp01(destroy + damageBoost * 0.30f);
    crush = clamp01(crush + damageBoost * damageBoost * 0.20f);
    decimate = clamp01(decimate + damageBoost * 0.16f);
    drive = clamp01(drive + damageBoost * 0.13f);
    clip = clamp01(clip + damageBoost * 0.10f);
    shatter = clamp01(shatter + damageBoost * 0.22f);

    movement = clamp01(movement + motionBoost * 0.34f);
    jitter = clamp01(jitter + motionBoost * 0.22f);
    glitch = clamp01(glitch + motionBoost * 0.14f);

    unstable = clamp01(unstable + chaosBoost * 0.38f);
    pitchChaos = clamp01(pitchChaos + chaosBoost * 0.30f);
    glitch = clamp01(glitch + chaosBoost * 0.12f);

    feedback = clamp01(feedback + spaceBoost * 0.22f);
    feedbackSpread = clamp01(feedbackSpread + spaceBoost * 0.30f);
    feedbackDiffusion = clamp01(feedbackDiffusion + spaceBoost * 0.18f);
    stereo = clamp01(stereo + spaceBoost * 0.22f);

    float intensity =
        clamp01(
            destroy
            * (0.55f + 0.70f * macro)
            + shakal * 0.12f);

    if (liveScene == 1)
        intensity =
            clamp01(
                intensity * 1.08f);

    if (liveScene == 2)
        glitch =
            clamp01(
                glitch * 1.80f);

    if (liveScene == 3)
    {
        fold =
            clamp01(fold + 0.16f * morph);
        shift =
            clamp01(shift + 0.07f * morph);
    }

    if (liveScene == 4)
    {
        crush =
            clamp01(
                crush + 0.12f * macro);
        decimate =
            clamp01(
                decimate + 0.10f * macro);
    }

    if (liveScene == 5)
    {
        intensity =
            clamp01(
                intensity * 1.15f);
        shatter =
            clamp01(
                shatter + 0.18f * macro);
        glitch =
            clamp01(
                glitch + 0.14f);
        fold =
            clamp01(
                fold + 0.08f);
    }

    // Four-slot modulation matrix.
    const float modAmounts[8] {
        value("mod1Amount"), value("mod2Amount"),
        value("mod3Amount"), value("mod4Amount"),
        value("mod5Amount"), value("mod6Amount"),
        value("mod7Amount"), value("mod8Amount")
    };

    const int modSources[8] {
        choiceIndex(apvts, "mod1Source", 0), choiceIndex(apvts, "mod2Source", 0),
        choiceIndex(apvts, "mod3Source", 0), choiceIndex(apvts, "mod4Source", 0),
        choiceIndex(apvts, "mod5Source", 0), choiceIndex(apvts, "mod6Source", 0),
        choiceIndex(apvts, "mod7Source", 0), choiceIndex(apvts, "mod8Source", 0)
    };

    const int modDests[8] {
        choiceIndex(apvts, "mod1Dest", 0), choiceIndex(apvts, "mod2Dest", 0),
        choiceIndex(apvts, "mod3Dest", 0), choiceIndex(apvts, "mod4Dest", 0),
        choiceIndex(apvts, "mod5Dest", 0), choiceIndex(apvts, "mod6Dest", 0),
        choiceIndex(apvts, "mod7Dest", 0), choiceIndex(apvts, "mod8Dest", 0)
    };

    auto applyMod =
        [](int destination,
           float amount,
           float& a,
           float& b,
           float& c,
           float& d,
           float& e,
           float& f,
           float& g,
           float& h,
           float& i)
    {
        const float m =
            amount * 0.32f;

        switch (destination)
        {
            case 1: a += m; break;
            case 2: b += m; break;
            case 3: c += m; break;
            case 4: d += m; break;
            case 5: e += m; break;
            case 6: f += m; break;
            case 7: g += m; break;
            case 8: h += m; break;
            case 9: i += m; break;
            default: break;
        }
    };

    double bpm = 120.0;

    if (auto* currentPlayHead =
            getPlayHead())
    {
        if (auto position =
                currentPlayHead->getPosition())
        {
            if (auto hostBpm =
                    position->getBpm())
            {
                bpm = juce::jlimit(
                    30.0,
                    300.0,
                    *hostBpm);
            }
        }
    }

    float movementRateHz =
        0.16f
        + 4.0f * movement;

    if (syncRate > 0)
    {
        const float multipliers[] {
            1.0f, 2.0f, 4.0f, 8.0f
        };

        movementRateHz =
            static_cast<float>(
                bpm / 60.0)
            * multipliers[
                juce::jlimit(
                    0,
                    3,
                    syncRate - 1)];
    }

    const float movementIncrement =
        2.0f * pi * movementRateHz
        / static_cast<float>(
            currentSampleRate);

    float modRateHz = modRate;

    if (modSync > 0)
    {
        const float modMultipliers[] {
            1.0f, 2.0f, 4.0f, 8.0f
        };

        modRateHz =
            static_cast<float>(
                bpm / 60.0)
            * modMultipliers[
                juce::jlimit(
                    0,
                    3,
                    modSync - 1)];
    }

    const float modIncrement =
        2.0f * pi * modRateHz
        / static_cast<float>(
            currentSampleRate);

    const float splitAlpha1 =
        onePoleAlpha(
            100.0f
            + split * 130.0f,
            currentSampleRate);

    const float splitAlpha2 =
        onePoleAlpha(
            850.0f,
            currentSampleRate);

    const float splitAlpha3 =
        onePoleAlpha(
            3000.0f,
            currentSampleRate);

    const float baseCutoff =
        juce::jlimit(
            100.0f,
            static_cast<float>(
                currentSampleRate)
            * 0.44f,
            filterFreq);

    setFilterFromParameters(
        filterType,
        baseCutoff,
        juce::jlimit(
            0.05f,
            0.82f,
            filterRes));

    const float safetyCutoff =
        juce::jlimit(
            4800.0f,
            static_cast<float>(
                currentSampleRate)
            * 0.46f,
            19000.0f
            - smooth * 10500.0f
            - antiNoise * antiAir * 3600.0f);

    for (auto& filter : safetyFilter)
    {
        filter.setType(
            juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setCutoffFrequency(
            safetyCutoff);
        filter.setResonance(
            0.06f
            + smooth * 0.15f);
    }

    auto nonlinearStage =
        [this, drive, clip, preGain, modeScale]
        (juce::dsp::AudioBlock<float>& block)
    {
        const float pre =
            juce::Decibels::decibelsToGain(
                preGain);

        for (size_t sample = 0;
             sample < block.getNumSamples();
             ++sample)
        {
            for (size_t ch = 0;
                 ch < block.getNumChannels();
                 ++ch)
            {
                auto* data =
                    block.getChannelPointer(ch);

                const float x =
                    data[sample] * pre;

                if (modeScale < 0.2f)
                {
                    data[sample] = x;
                }
                else
                {
                    const float stageScale =
                        juce::jlimit(
                            0.0f,
                            1.0f,
                            modeScale);

                    data[sample] =
                        shapedSample(
                            x,
                            drive
                                * (0.45f
                                   + 0.55f
                                     * stageScale)
                                * 0.82f,
                            clip
                                * (0.35f
                                   + 0.65f
                                     * stageScale)
                                * 0.70f);
                }
            }
        }
    };

    const int requestedPerformTrigger = performTrigger.exchange(0);
    if (requestedPerformTrigger > 0)
    {
        performType = juce::jlimit(1, 4, requestedPerformTrigger);
        performRemaining = static_cast<int>(0.14 * currentSampleRate);
        for (int ch = 0; ch < channels; ++ch)
            performHeldSample[static_cast<size_t>(ch)] =
                dryBuffer.getSample(ch, 0);
    }

    if (quality == 1)
    {
        const auto input =
            juce::dsp::AudioBlock<const float>(
                buffer);

        auto up =
            oversampler2x.processSamplesUp(
                input);

        nonlinearStage(up);

        auto output =
            juce::dsp::AudioBlock<float>(
                buffer);

        oversampler2x.processSamplesDown(
            output);
    }
    else if (quality == 2)
    {
        const auto input =
            juce::dsp::AudioBlock<const float>(
                buffer);

        auto up =
            oversampler4x.processSamplesUp(
                input);

        nonlinearStage(up);

        auto output =
            juce::dsp::AudioBlock<float>(
                buffer);

        oversampler4x.processSamplesDown(
            output);
    }
    else
    {
        auto block =
            juce::dsp::AudioBlock<float>(
                buffer);

        nonlinearStage(block);
    }

    // v3 TRUE SPECTRAL ENGINE: responsive 128-sample FFT windows.
    if (fftMix > 0.0001f && fftShatter > 0.0001f)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            for (int start = 0; start < samples; start += fftSize)
            {
                fftData.fill(0.0f);

                for (int i = 0; i < fftSize; ++i)
                {
                    const int pos = start + i;
                    const float input =
                        pos < samples
                            ? buffer.getSample(ch, pos)
                            : 0.0f;

                    const float phase =
                        static_cast<float>(i)
                        / static_cast<float>(fftSize - 1);

                    float window = 0.5f;
                    switch (fftWindow)
                    {
                        case 1:
                            window =
                                0.42f
                                - 0.50f * std::cos(2.0f * pi * phase)
                                + 0.08f * std::cos(4.0f * pi * phase);
                            break;
                        case 2:
                            window =
                                phase < 0.5f
                                    ? phase * 2.0f
                                    : 2.0f - phase * 2.0f;
                            break;
                        case 3:
                            window = 1.0f;
                            break;
                        default:
                            window =
                                0.5f
                                - 0.5f * std::cos(2.0f * pi * phase);
                            break;
                    }

                    fftData[static_cast<size_t>(i)] =
                        input * window;
                }

                fft.performRealOnlyForwardTransform(
                    fftData.data());

                fftSource = fftData;

                const int shiftBins =
                    static_cast<int>(std::round(fftShift * 10.0f));

                for (int bin = 0; bin <= fftSize / 2; ++bin)
                {
                    const int sourceBin =
                        juce::jlimit(
                            0,
                            fftSize / 2,
                            bin - shiftBins);

                    float re = 0.0f;
                    float im = 0.0f;

                    if (sourceBin == 0)
                    {
                        re = fftSource[0];
                    }
                    else if (sourceBin == fftSize / 2)
                    {
                        re = fftSource[1];
                    }
                    else
                    {
                        re = fftSource[static_cast<size_t>(2 * sourceBin)];
                        im = fftSource[static_cast<size_t>(2 * sourceBin + 1)];
                    }

                    float magnitude =
                        std::sqrt(re * re + im * im);

                    float frozen =
                        fftFrozenMagnitude[
                            static_cast<size_t>(ch)][
                                static_cast<size_t>(bin)];

                    if (frozen < 1.0e-8f)
                        frozen = magnitude;

                    frozen +=
                        (0.008f + (1.0f - fftFreeze) * 0.10f)
                        * (magnitude - frozen);

                    fftFrozenMagnitude[
                        static_cast<size_t>(ch)][
                            static_cast<size_t>(bin)] = frozen;

                    magnitude =
                        juce::jmap(
                            fftFreeze,
                            magnitude,
                            frozen);

                    if (fftBits > 0.0001f
                        && magnitude > 1.0e-8f)
                    {
                        const float levels =
                            std::pow(
                                2.0f,
                                4.0f + (1.0f - fftBits) * 8.0f);

                        magnitude =
                            std::round(magnitude * levels)
                            / levels;
                    }

                    if (bin > 0 && bin < fftSize / 2)
                    {
                        float prevMag = magnitude;
                        float nextMag = magnitude;

                        const int prevBin = juce::jmax(0, sourceBin - 1);
                        const int nextBin =
                            juce::jmin(fftSize / 2, sourceBin + 1);

                        auto packedMagnitude =
                            [](const std::array<float, 256>& data, int k)
                        {
                            if (k == 0)
                                return std::abs(data[0]);
                            if (k == 64)
                                return std::abs(data[1]);

                            const float re =
                                data[static_cast<size_t>(2 * k)];
                            const float im =
                                data[static_cast<size_t>(2 * k + 1)];
                            return std::sqrt(re * re + im * im);
                        };

                        prevMag =
                            packedMagnitude(
                                fftSource,
                                prevBin);

                        nextMag =
                            packedMagnitude(
                                fftSource,
                                nextBin);

                        const float spread =
                            0.5f * (prevMag + nextMag);

                        magnitude =
                            juce::jmap(
                                fftShatter * 0.45f,
                                magnitude,
                                spread);
                    }

                    const float localThreshold =
                        fftThreshold * 0.28f;

                    if (magnitude < localThreshold)
                        magnitude =
                            juce::jmap(
                                fftThreshold,
                                magnitude,
                                0.0f);

                    const float spreadWarp =
                        std::sin(
                            static_cast<float>(bin) * 0.91f
                            + movementPhase * 0.17f
                            + fftWarp * 2.4f);

                    magnitude *=
                        1.0f
                        + spreadWarp
                          * fftSpread
                          * fftShatter
                          * 0.32f;

                    const float phase =
                        std::atan2(im, re)
                        + std::sin(
                            static_cast<float>(bin) * 1.731f
                            + morphPhase * 2.1f
                            + movementPhase * 0.31f)
                          * fftShatter * 1.35f;

                    magnitude *=
                        0.80f
                        + std::abs(
                            std::sin(
                                static_cast<float>(bin) * 1.731f
                                + movementPhase))
                          * fftShatter * 0.58f;

                    re = magnitude * std::cos(phase);
                    im = magnitude * std::sin(phase);

                    if (bin == 0)
                        fftData[0] = re;
                    else if (bin == fftSize / 2)
                        fftData[1] = re;
                    else
                    {
                        fftData[static_cast<size_t>(2 * bin)] = re;
                        fftData[static_cast<size_t>(2 * bin + 1)] = im;
                    }

                    if (ch == 0)
                    {
                        const float meter =
                            juce::jlimit(
                                0.0f,
                                1.0f,
                                std::log1p(magnitude * 10.0f)
                                / std::log(11.0f));

                        spectrumBuffer[
                            static_cast<size_t>(
                                juce::jmin(63, bin))]
                            .store(
                                0.78f
                                * spectrumBuffer[
                                    static_cast<size_t>(
                                        juce::jmin(63, bin))].load()
                                + 0.22f * meter);
                    }
                }

                fft.performRealOnlyInverseTransform(
                    fftData.data());

                const float spectralAmount =
                    juce::jlimit(
                        0.0f,
                        1.0f,
                        fftMix * fftShatter);

                for (int i = 0; i < fftSize; ++i)
                {
                    const int pos = start + i;
                    if (pos >= samples)
                        break;

                    const float spectral =
                        fftData[static_cast<size_t>(i)] * 2.0f;

                    const float original =
                        buffer.getSample(ch, pos);

                    fftWetBuffer.setSample(
                        ch,
                        pos,
                        juce::jmap(
                            spectralAmount,
                            original,
                            spectral));
                }
            }
        }
    }
    else
    {
        // Avoid a nested sample loop when FFT processing is bypassed.
        fftWetBuffer.makeCopyOf(
            buffer,
            true);
    }

    float inputEnergy = 0.0f;
    float processedEnergy = 0.0f;
    float blockPeak = 0.0f;
    std::array<float, 4> bandPeak {
        0.0f, 0.0f, 0.0f, 0.0f
    };

    for (int sample = 0;
         sample < samples;
         ++sample)
    {
        // One shared event roll keeps stereo glitch events coherent.
        const float glitchRoll =
            nextRandom();

        movementPhase +=
            movementIncrement;

        modPhase +=
            modIncrement;

        if (modPhase >= 2.0f * pi)
            modPhase -= 2.0f * pi;

        syncPhase +=
            movementIncrement;

        if (movementPhase >= 2.0f * pi)
            movementPhase -= 2.0f * pi;

        if (syncPhase >= 2.0f * pi)
            syncPhase -= 2.0f * pi;

        const bool performActive = performRemaining > 0;
        const float performProgress = performActive
            ? 1.0f - static_cast<float>(performRemaining)
              / static_cast<float>(juce::jmax(1, static_cast<int>(0.14 * currentSampleRate)))
            : 1.0f;
        const float performFade = performActive
            ? std::sin(juce::jlimit(0.0f, 1.0f, performProgress) * pi)
            : 0.0f;

        if (movementShape == 2)
        {
            if (--movementHoldCounter <= 0)
            {
                movementHoldCounter =
                    2 + static_cast<int>(
                        nextRandom() * 28.0f);

                movementHoldValue =
                    nextRandom()
                    * 2.0f
                    - 1.0f;
            }
        }

        if (modWave == 2
            && --modHoldCounter <= 0)
        {
            modHoldCounter =
                2 + static_cast<int>(
                    nextRandom() * 24.0f);

            modHoldValue =
                nextRandom()
                * 2.0f
                - 1.0f;
        }

        if (unstable > 0.001f
            && --unstableRemaining <= 0)
        {
            unstableRemaining =
                220 + static_cast<int>(
                    nextRandom() * 1100.0f);

            unstableValue =
                nextRandom()
                * 2.0f
                - 1.0f;
        }

        const float movementBipolar =
            movementValue(
                movementShape,
                movementPhase);

        const float beatValue =
            std::sin(syncPhase);

        for (int ch = 0;
             ch < channels;
             ++ch)
        {
            const size_t index =
                static_cast<size_t>(ch);

            const float dry =
                dryBuffer.getSample(
                    ch,
                    sample);

            inputEnergy +=
                dry * dry;

            const float fastAbs =
                std::abs(dry);

            const float fastCoeff =
                fastAbs > fastEnvelope[index]
                    ? 0.026f
                    : 0.0011f;

            const float slowCoeff =
                fastAbs > slowEnvelope[index]
                    ? 0.0055f
                    : 0.00035f;

            fastEnvelope[index] +=
                fastCoeff
                * (fastAbs
                   - fastEnvelope[index]);

            slowEnvelope[index] +=
                slowCoeff
                * (fastAbs
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

            float localShakal = shakal;
            float localDestroy = destroy;
            float localCrush = crush;
            float localDecimate = decimate;
            float localShatter = shatter;
            float localFold = fold;
            float localShift = shift;
            float localGlitch = glitch;
            float localFilter = 0.0f;

            for (int slot = 0;
                 slot < 8;
                 ++slot)
            {
                float source = 0.0f;

                switch (modSources[slot])
                {
                    case 1:
                    {
                        float lfo =
                            std::sin(modPhase);

                        if (modWave == 1)
                        {
                            const float wrapped =
                                modPhase
                                / (2.0f * pi)
                                - std::floor(
                                    modPhase
                                    / (2.0f * pi));

                            const float tri =
                                wrapped < 0.5f
                                    ? wrapped * 4.0f - 1.0f
                                    : 3.0f
                                      - wrapped * 4.0f;

                            lfo = tri;
                        }
                        else if (modWave == 2)
                        {
                            lfo = modHoldValue;
                        }
                        else if (modWave == 3)
                        {
                            const float wrapped =
                                modPhase
                                / (2.0f * pi)
                                - std::floor(
                                    modPhase
                                    / (2.0f * pi));

                            lfo =
                                wrapped < 0.5f
                                    ? 1.0f
                                    : -1.0f;
                        }

                        source =
                            bipolarToUnit(lfo);
                        break;
                    }

                    case 2:
                        source =
                            clamp01(
                                slowEnvelope[index]
                                * 3.0f);
                        break;

                    case 3:
                        source =
                            bipolarToUnit(
                                unstableValue);
                        break;

                    case 4:
                        source =
                            bipolarToUnit(
                                movementHoldValue);
                        break;

                    case 5:
                        source =
                            bipolarToUnit(
                                beatValue);
                        break;

                    default:
                        source = 0.5f;
                        break;
                }

                const float signedSource =
                    source * 2.0f - 1.0f;

                const float modTarget =
                    juce::jlimit(
                        -1.0f,
                        1.0f,
                        signedSource
                        * modAmounts[slot]
                        * modDepth);

                const float modSmoothing =
                    0.025f
                    + modSmooth * 0.45f;

                modSmoothState[slot] +=
                    modSmoothing
                    * (modTarget
                       - modSmoothState[slot]);

                applyMod(
                    modDests[slot],
                    modSmoothState[slot],
                    localShakal,
                    localDestroy,
                    localCrush,
                    localDecimate,
                    localShatter,
                    localFold,
                    localShift,
                    localGlitch,
                    localFilter);
            }

            localShakal =
                clamp01(localShakal);

            localDestroy =
                clamp01(localDestroy);

            localCrush =
                clamp01(localCrush);

            localDecimate =
                clamp01(localDecimate);

            localShatter =
                clamp01(localShatter);

            localFold =
                clamp01(localFold);

            localShift =
                clamp01(localShift);

            localGlitch =
                clamp01(localGlitch);

            if (audioAware > 0.0001f)
            {
                const float instantaneousLow = juce::jlimit(0.0f, 1.0f, std::abs(low) * 2.8f);
                const float instantaneousHigh = juce::jlimit(0.0f, 1.0f, (std::abs(high) + std::abs(air)) * 2.2f);
                const float presence = juce::jlimit(
                    0.0f,
                    1.0f,
                    instantaneousHigh * 0.68f
                    + transientAmount * 0.52f
                    + bodyAmount * 0.18f);
                dynamicIntensity = juce::jlimit(
                    0.0f,
                    1.0f,
                    dynamicIntensity
                    * (1.0f - audioAware * instantaneousLow * 0.22f)
                    + audioAware * presence * 0.20f);
            }

            if (performActive)
            {
                const float pulse = performFade * (1.0f - 0.25f * performFade);
                if (performType == 1)
                {
                    dynamicIntensity = juce::jlimit(0.0f, 1.0f, dynamicIntensity + pulse * 0.52f);
                    localDestroy = juce::jlimit(0.0f, 1.0f, localDestroy + pulse * 0.38f);
                    localCrush = juce::jlimit(0.0f, 1.0f, localCrush + pulse * 0.24f);
                }
                else if (performType == 2)
                {
                    localGlitch = 1.0f;
                    localDestroy = juce::jlimit(0.0f, 1.0f, localDestroy + pulse * 0.22f);
                }
                else if (performType == 4)
                {
                    dynamicIntensity = juce::jlimit(0.0f, 1.0f, dynamicIntensity + pulse * 0.44f);
                    localShatter = juce::jlimit(0.0f, 1.0f, localShatter + pulse * 0.34f);
                    localFold = juce::jlimit(0.0f, 1.0f, localFold + pulse * 0.18f);
                    localGlitch = juce::jlimit(0.0f, 1.0f, localGlitch + pulse * 0.42f);
                }
            }

            const float reactiveSignal =
                clamp01(
                    transientAmount * reactiveTransient
                    + (bandPeak[1] + bandPeak[2]) * 0.5f * reactiveSpectral
                    + bandPeak[0] * reactiveBass
                    + bandPeak[3] * reactiveHigh);

            float dynamicIntensity =
                intensity;

            if (reactiveAmount > 0.0001f)
            {
                const float curved =
                    std::pow(
                        reactiveSignal,
                        0.35f
                        + macroCurve * 1.65f);

                dynamicIntensity =
                    clamp01(
                        dynamicIntensity
                        + curved
                          * reactiveAmount
                          * 0.72f);
            }

            if (smart)
            {
                const float inputShape =
                    clamp01(
                        transientAmount * 0.70f
                        + bodyAmount * 0.30f);

                const float adaptive =
                    smartAmount
                    * (0.65f
                       + inputShape * 0.55f);

                dynamicIntensity =
                    clamp01(
                        dynamicIntensity
                        * juce::jmap(
                            adaptive,
                            1.0f,
                            0.72f));

                localShatter =
                    clamp01(
                        localShatter
                        * juce::jmap(
                            adaptive,
                            1.0f,
                            0.76f));

                localCrush =
                    clamp01(
                        localCrush
                        * juce::jmap(
                            smartBassProtect,
                            1.0f,
                            0.78f));

                if (transientAmount > 0.12f)
                {
                    localDestroy =
                        clamp01(
                            localDestroy
                            * juce::jmap(
                                smartTransientProtect,
                                1.0f,
                                0.58f));

                    localGlitch =
                        clamp01(
                            localGlitch
                            * juce::jmap(
                                smartTransientProtect,
                                1.0f,
                                0.62f));
                }

                localShatter =
                    clamp01(
                        localShatter
                        * juce::jmap(
                            smartHighControl
                            * bandHigh,
                            1.0f,
                            0.82f));
            }

            const float envAmount =
                juce::jmap(
                    envFollow,
                    1.0f,
                    clamp01(
                        0.35f
                        + transientAmount * 0.65f
                        + bodyAmount * 0.30f));

            if (holdRemaining[index] <= 0)
            {
                previousHeldSample[index] =
                    heldSample[index];

                heldSample[index] =
                    dry;

                int hold =
                    1
                    + static_cast<int>(
                        std::round(
                            localDecimate
                            * dynamicIntensity
                            * 72.0f));

                if (jitter > 0.001f)
                {
                    const int jitterAmount =
                        static_cast<int>(
                            std::round(
                                hold
                                * jitter
                                * 0.45f));

                    hold +=
                        static_cast<int>(
                            (nextRandom()
                             * 2.0f - 1.0f)
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
                              progress
                              * 2.0f
                              * pi
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
                        nextRandom()
                            < progress
                            ? heldSample[index]
                            : previousHeldSample[index];
                    break;
            }

            float source =
                routing == 2
                    ? dry
                    : buffer.getSample(
                        ch,
                        sample);

            if (routing == 3)
            {
                source =
                    0.5f
                    * (source + dry);
            }

            splitLow1[index] =
                (1.0f - splitAlpha1)
                * source
                + splitAlpha1
                * splitLow1[index];

            splitLow2[index] =
                (1.0f - splitAlpha2)
                * source
                + splitAlpha2
                * splitLow2[index];

            splitLow3[index] =
                (1.0f - splitAlpha3)
                * source
                + splitAlpha3
                * splitLow3[index];

            const float low =
                splitLow1[index];

            const float mid =
                splitLow2[index] - low;

            const float high =
                splitLow3[index] - splitLow2[index];

            const float air =
                source - splitLow3[index];

            bandPeak[0] =
                juce::jmax(
                    bandPeak[0],
                    std::abs(low));
            bandPeak[1] =
                juce::jmax(
                    bandPeak[1],
                    std::abs(mid));
            bandPeak[2] =
                juce::jmax(
                    bandPeak[2],
                    std::abs(high));
            bandPeak[3] =
                juce::jmax(
                    bandPeak[3],
                    std::abs(air));

            const float shatterBase =
                localShatter
                * dynamicIntensity
                * envAmount;

            const float lowAmt =
                clamp01(
                    shatterBase
                    * bandLow
                    * 0.34f);

            const float midAmt =
                clamp01(
                    shatterBase
                    * bandMid
                    * 0.72f);

            const float highAmt =
                clamp01(
                    shatterBase
                    * bandHigh
                    * 0.82f);

            const float airAmt =
                clamp01(
                    shatterBase
                    * bandAir
                    * (0.42f
                       + 0.32f
                         * (1.0f
                            - smooth)));

            auto destroyBand =
                [this, localCrush,
                 localDecimate, localFold,
                 character, resampled,
                 spectralMode, index,
                 movementBipolar,
                 spectralMix, spectralSmear,
                 spectralFreezeAmount, spectralBits,
                 spectralRing]
                (float band,
                 float amount)
            {
                if (amount <= 0.0001f)
                    return band;

                float effectiveAmount =
                    amount
                    * juce::jlimit(
                        0.0f,
                        1.0f,
                        spectralMix);

                if (spectralMode == 2)
                    effectiveAmount *= 0.58f;
                else if (spectralMode == 3)
                    effectiveAmount *= 0.82f;
                else if (spectralMode == 4)
                    effectiveAmount = juce::jmin(
                        1.0f,
                        amount * 1.18f);

                const float crushAmount =
                    clamp01(
                        localCrush
                        * (0.38f
                           + effectiveAmount * 0.82f));

                int bits =
                    juce::jlimit(
                        4,
                        16,
                        static_cast<int>(
                            std::round(
                                16.0f
                                - crushAmount
                                  * 10.0f)));

                if (spectralMode == 4)
                    bits = juce::jlimit(
                        3,
                        11,
                        bits - 2);

                if (spectralBits > 0.001f)
                    bits = juce::jlimit(
                        3,
                        16,
                        bits
                        - static_cast<int>(
                            std::round(
                                spectralBits * 9.0f)));

                const float levels =
                    static_cast<float>(
                        (1u << bits) - 1u);

                float x =
                    juce::jmap(
                        effectiveAmount
                        * (0.64f
                           + localDecimate
                             * 0.25f),
                        band,
                        resampled);

                if (spectralMode == 2)
                {
                    const float smearAmount =
                        juce::jlimit(
                            0.0f,
                            1.0f,
                            spectralSmear
                            + effectiveAmount * 0.55f);

                    x =
                        juce::jmap(
                            smearAmount,
                            x,
                            resampled);
                }
                else if (spectralMode == 3)
                {
                    const float freezeAmount =
                        juce::jlimit(
                            0.0f,
                            1.0f,
                            spectralFreezeAmount
                            + effectiveAmount * 0.72f);

                    spectralFreeze[index] =
                        juce::jmap(
                            0.006f
                            + freezeAmount * 0.028f,
                            spectralFreeze[index],
                            x);

                    x =
                        juce::jmap(
                            freezeAmount,
                            x,
                            spectralFreeze[index]);
                }
                else if (spectralMode == 5)
                {
                    const float ring =
                        std::sin(
                            alienPhase[index] * 0.47f
                            + movementPhase * 1.31f
                            + static_cast<float>(index) * 0.91f);

                    x *=
                        1.0f
                        + ring
                          * spectralRing
                          * 0.42f;

                    x =
                        juce::jlimit(
                            -1.25f,
                            1.25f,
                            x);
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
                             * 0.22f
                             * crushAmount))
                        * levels)
                    / levels;

                const float quantizeMix =
                    spectralMode == 0
                        ? 0.42f
                        : spectralMode == 1
                            ? 0.62f
                            : spectralMode == 2
                                ? 0.24f
                                : spectralMode == 4
                                    ? 0.82f
                                    : 0.48f;

                x =
                    juce::jmap(
                        effectiveAmount * quantizeMix,
                        x,
                        quantized);

                x =
                    waveFold(
                        x,
                        localFold
                        * effectiveAmount
                        * juce::jmap(
                            character,
                            0.45f,
                            1.08f));

                return softCeiling(
                    x,
                    0.07f
                    + effectiveAmount * 0.18f);
            };

                    const float shattered =
                destroyBand(low, lowAmt)
                + destroyBand(mid, midAmt)
                + destroyBand(high, highAmt)
                + destroyBand(air, airAmt);

            const float spectralBlend =
                juce::jlimit(
                    0.0f,
                    1.0f,
                    spectralMix
                    * (0.50f
                       + 0.50f * shatterBase));

            const float fftSample =
                fftWetBuffer.getSample(
                    ch,
                    sample);

            float wet =
                juce::jmap(
                    spectralBlend,
                    source,
                    shattered);

            wet =
                juce::jmap(
                    fftMix * fftShatter,
                    wet,
                    fftSample);

            if (routing == 3)
            {
                const float parallelDry =
                    juce::jmap(
                        shatterBase * 0.50f,
                        source,
                        shattered);

                wet =
                    0.5f
                    * (wet
                       + parallelDry);
            }

            wet *=
                1.0f
                + (movement
                   * movementBipolar
                   * 0.18f)
                + unstableValue
                  * unstable
                  * 0.08f;

            // v6 Character Engine 2.0.
            switch (characterMode)
            {
                case 1: // Digital.
                {
                    const float levels =
                        std::pow(2.0f, 17.0f - character * 8.0f);
                    wet =
                        std::round(wet * levels)
                        / juce::jmax(1.0f, levels);
                    wet =
                        std::tanh(wet * (1.0f + character * 1.8f));
                    break;
                }

                case 2: // VHS.
                {
                    const float flutter =
                        std::sin(alienPhase[index] * 0.21f
                                 + movementPhase * 0.071f)
                        * character * 0.10f;
                    wet =
                        juce::jmap(
                            character * 0.32f,
                            wet,
                            previousHeldSample[index]);
                    wet *= 0.88f + flutter;
                    break;
                }

                case 3: // Console.
                    wet =
                        shapedSample(
                            wet,
                            character * 9.0f + character * character * 4.0f,
                            character * 0.50f);
                    wet = std::tanh(wet * (1.0f + character));
                    break;

                case 4: // Radio.
                    wet =
                        0.45f * wet
                        + 0.55f * air * (0.30f + character * 0.52f);
                    wet = std::tanh(wet * (1.0f + character * 0.65f));
                    break;

                case 5: // Metallic.
                {
                    const float metal =
                        std::sin(alienPhase[index] * 1.7f
                                 + movementPhase * 2.3f);
                    wet += wet * metal * character * 0.34f;
                    wet = std::tanh(wet * (1.0f + character * 0.7f));
                    break;
                }

                case 6: // Broken.
                {
                    const float levels = 7.0f + character * 14.0f;
                    wet = std::round(wet * levels) / levels;
                    wet *=
                        0.78f
                        + 0.22f * std::sin(
                            alienPhase[index] * 0.37f
                            + movementBipolar * 2.0f);
                    break;
                }

                case 7: // Alien.
                {
                    const float carrier =
                        std::sin(alienPhase[index] * 0.83f
                                 + movementPhase * 1.71f);
                    wet =
                        wet * (0.70f + 0.30f * carrier)
                        + wet * carrier * character * 0.24f;
                    break;
                }

                case 8: // Cheap DAC.
                    wet =
                        std::round(
                            (wet + tpdfDither(0.0025f + character * 0.012f))
                            * (24.0f + (1.0f - character) * 80.0f))
                        / (24.0f + (1.0f - character) * 80.0f);
                    break;

                case 9: // Corrupt.
                {
                    const float phaseWarp =
                        std::sin(
                            (wet * 5.0f + movementBipolar * 1.7f)
                            * (1.0f + character * 7.0f));
                    wet *= 0.52f + 0.48f * phaseWarp;
                    wet +=
                        std::sin(alienPhase[index] * 3.1f)
                        * wet * character * 0.18f;
                    break;
                }

                default:
                    break;
            }

            if (feedback > 0.0001f)
            {
                const float toneCutoff =
                    350.0f + feedbackTone * 10500.0f;

                const float toneAlpha =
                    onePoleAlpha(
                        toneCutoff,
                        currentSampleRate);

                feedbackToneState[index] +=
                    (1.0f - toneAlpha)
                    * (feedbackState[index]
                       - feedbackToneState[index]);

                const int feedbackDelay =
                    juce::jlimit(
                        1,
                        8000,
                        static_cast<int>(
                            std::round(
                                (0.003f
                                 + feedbackTime * 0.42f)
                                * currentSampleRate)));

                int feedbackRead =
                    feedbackWriteIndex
                    - feedbackDelay;

                while (feedbackRead < 0)
                    feedbackRead += 8192;

                const float delayed =
                    feedbackBuffer[index][
                        static_cast<size_t>(
                            feedbackRead)];

                const float feedbackSource =
                    juce::jmap(
                        feedbackFreeze,
                        delayed,
                        feedbackState[index]);

                const float feedbackSample =
                    shapedSample(
                        feedbackSource,
                        feedbackDrive * 14.0f,
                        feedbackDrive * 0.72f);

                const float spread =
                    feedbackSpread
                    * stereo
                    * (index == 0 ? 1.0f : -1.0f);

                wet +=
                    feedbackSample
                    * feedback
                    * (0.18f + 0.58f * shakal);

                wet +=
                    spread
                    * feedbackSample
                    * feedback
                    * 0.12f;

                feedbackState[index] =
                    juce::jmap(
                        feedbackDiffusion,
                        wet,
                        0.5f
                        * (wet
                           + feedbackSample));

                // Keep recursive feedback bounded even at extreme settings.
                if (!std::isfinite(feedbackState[index]))
                    feedbackState[index] = 0.0f;

                feedbackState[index] =
                    juce::jlimit(-3.0f, 3.0f,
                                 feedbackState[index]);

                float& feedbackWrite =
                    feedbackBuffer[index][
                        static_cast<size_t>(
                            feedbackWriteIndex)];

                feedbackWrite =
                    juce::jmap(
                        feedbackPitch,
                        feedbackState[index],
                        delayed);

                if (!std::isfinite(feedbackWrite))
                    feedbackWrite = 0.0f;

                feedbackWrite =
                    juce::jlimit(-3.0f, 3.0f,
                                 feedbackWrite);
            }

            if (grainMix > 0.0001f)
            {
                const int history =
                    juce::jlimit(
                        32,
                        12000,
                        static_cast<int>(
                            std::round(
                                (0.006f
                                 + grainSize * 0.095f)
                                * currentSampleRate)));

                const float densityGate =
                    nextRandom();

                if (densityGate > juce::jlimit(
                        0.02f,
                        0.98f,
                        grainDensity))
                {
                    grainPhase[index] +=
                        1.0f
                        / static_cast<float>(history);
                }

                float pitchFactor =
                    juce::jmap(
                        grainPitch,
                        0.50f,
                        2.00f);

                if (pitchDamage > 0.0001f)
                {
                    float damageFactor = 1.0f;

                    switch (pitchMode)
                    {
                        case 1:
                        {
                            const float semitone =
                                std::round(
                                    ((nextRandom() * 2.0f - 1.0f)
                                     * pitchRange)
                                    * 12.0f);

                            damageFactor =
                                std::pow(
                                    2.0f,
                                    semitone / 12.0f);
                            break;
                        }

                        case 2:
                            damageFactor =
                                nextRandom() < 0.5f
                                    ? 0.5f
                                    : 2.0f;
                            break;

                        case 3:
                            damageFactor =
                                0.25f
                                + nextRandom() * 3.5f;
                            break;

                        default:
                            damageFactor =
                                std::pow(
                                    2.0f,
                                    (nextRandom() * 2.0f - 1.0f)
                                    * pitchRange);
                            break;
                    }

                    pitchFactor =
                        juce::jmap(
                            pitchDamage,
                            pitchFactor,
                            pitchFactor * damageFactor);
                }

                pitchFactor *=
                    1.0f
                    + std::sin(
                        movementPhase * 0.37f
                        + static_cast<float>(index))
                      * pitchDrift
                      * 0.22f
                    * (1.0f
                       + (nextRandom() * 2.0f - 1.0f)
                         * pitchChaos * 0.18f);

                grainPhase[index] +=
                    pitchFactor
                    / static_cast<float>(history);

                if (grainPhase[index] >= 1.0f)
                    grainPhase[index] -= 1.0f;

                const float window =
                    0.5f
                    - 0.5f
                      * std::cos(
                          2.0f * pi
                          * grainPhase[index]);

                const float spray =
                    (nextRandom() * 2.0f - 1.0f)
                    * grainSpray
                    * history;

                const float positionOffset =
                    (grainPosition - 0.5f)
                    * history
                    * 1.6f;

                float read =
                    static_cast<float>(grainWriteIndex)
                    - static_cast<float>(history)
                      * (0.15f + 0.70f * window)
                    + positionOffset
                    + spray;

                if (grainReverse > 0.001f
                    && nextRandom() < grainReverse)
                    read =
                        static_cast<float>(grainWriteIndex)
                        + static_cast<float>(history)
                          * (0.15f + 0.70f * window)
                        + positionOffset
                        + spray;

                read +=
                    (nextRandom() * 2.0f - 1.0f)
                    * grainJitter
                    * history * 0.32f;

                while (read < 0.0f)
                    read += 16384.0f;
                while (read >= 16384.0f)
                    read -= 16384.0f;

                const int r0 =
                    static_cast<int>(read) & 16383;
                const int r1 =
                    (r0 + 1) & 16383;
                const float frac =
                    read - std::floor(read);

                const float grain =
                    juce::jmap(
                        frac,
                        grainBuffer[index][static_cast<size_t>(r0)],
                        grainBuffer[index][static_cast<size_t>(r1)]);

                wet =
                    juce::jmap(
                        grainMix
                        * (0.34f + 0.56f * window),
                        wet,
                        grain);
            }

            if (localShift > 0.001f)
            {
                const float carrierHz =
                    20.0f
                    + 1350.0f
                      * localShift
                      * localShift;

                alienPhase[index] +=
                    2.0f * pi
                    * carrierHz
                    / static_cast<float>(
                        currentSampleRate);

                if (alienPhase[index]
                    >= 2.0f * pi)
                {
                    alienPhase[index] -=
                        2.0f * pi;
                }

                const float carrier =
                    std::sin(
                        alienPhase[index]);

                wet =
                    juce::jmap(
                        localShift * 0.40f,
                        wet,
                        wet
                        * (0.74f
                           + 0.26f
                             * carrier));
            }

            if (alien > 0.001f)
            {
                wet +=
                    wet
                    * std::sin(
                        alienPhase[index]
                        * 0.61f)
                    * alien
                    * 0.18f;
            }

            if (resonance > 0.001f)
            {
                const int bufferSize =
                    static_cast<int>(
                        resonatorBuffer[index]
                            .size());

                const int delaySamples =
                    juce::jlimit(
                        24,
                        bufferSize - 1,
                        static_cast<int>(
                            (0.010f
                             + resonance
                               * 0.070f)
                            * currentSampleRate));

                int readIndex =
                    resonatorWriteIndex
                    - delaySamples;

                if (readIndex < 0)
                    readIndex += bufferSize;

                const float delayed =
                    resonatorBuffer[index]
                        [static_cast<size_t>(
                            readIndex)];

                wet +=
                    delayed
                    * resonance
                    * 0.22f;

                resonatorBuffer[index]
                    [static_cast<size_t>(
                        resonatorWriteIndex)] =
                    wet
                    + delayed
                      * (0.10f
                         + resonance
                           * 0.34f);
            }

            const int gridSlots =
                glitchGrid == 1
                    ? 8
                    : glitchGrid == 2
                        ? 16
                        : glitchGrid == 3
                            ? 32
                            : 0;

            bool gridBoundary = true;

            if (gridSlots > 0)
            {
                const float cycles =
                    syncPhase
                    / (2.0f * pi);

                const float wrapped =
                    cycles - std::floor(cycles);

                const int slot =
                    static_cast<int>(
                        wrapped * gridSlots);

                gridBoundary =
                    slot != lastGlitchGridSlot;

                if (gridBoundary)
                    lastGlitchGridSlot = slot;
            }

            const float lengthBeats =
                std::pow(
                    2.0f,
                    -6.0f
                    + static_cast<float>(
                        glitchLength));

            const int requestedGlitchSamples =
                juce::jlimit(
                    8,
                    16300,
                    static_cast<int>(
                        std::round(
                            (60.0 / bpm)
                            * lengthBeats
                            * currentSampleRate)));

            int timelineSlot =
                static_cast<int>(
                    std::floor(
                        (syncPhase / (2.0f * pi)
                         - std::floor(syncPhase / (2.0f * pi)))
                        * 8.0f));

            timelineSlot =
                juce::jlimit(0, 7, timelineSlot);

            const float timelineLevel =
                juce::jmap(
                    timelineMix,
                    1.0f,
                    timelineSteps[timelineSlot]);

            float patternGate = 1.0f;
            switch (glitchPattern)
            {
                case 1: patternGate = (timelineSlot & 1) == 0 ? 1.0f : 0.0f; break;
                case 2: patternGate = (timelineSlot & 1) != 0 ? 1.0f : 0.0f; break;
                case 3:
                    patternGate =
                        (timelineSlot == 0 || timelineSlot == 3
                         || timelineSlot == 4 || timelineSlot == 6)
                            ? 1.0f : 0.0f;
                    break;
                case 4:
                    patternGate =
                        (timelineSlot == 0 || timelineSlot == 4)
                            ? 1.0f : 0.0f;
                    break;
                case 5: patternGate = 1.0f; break;
                case 6: patternGate = timelineSlot < 4 ? 1.0f : 0.0f; break;
                default: break;
            }

            float triggerChance =
                gridSlots > 0
                    ? localGlitch
                      * glitchDensity
                      * glitchProbability
                      * (0.025f
                         + dynamicIntensity * 0.12f)
                      * (gridBoundary ? 1.0f : 0.0f)
                      * timelineLevel
                      * patternGate
                    : localGlitch
                      * glitchDensity
                      * glitchProbability
                      * 0.000018f
                      * (0.65f
                         + dynamicIntensity * 1.55f)
                      * (0.45f + 0.55f * patternGate);

            const float humanTiming =
                0.72f + humanRandom * (0.18f + transientAmount * 0.42f);
            triggerChance =
                juce::jlimit(
                    0.0f,
                    1.0f,
                    triggerChance * humanTiming
                    + (performActive && performType == 2 ? 1.0f : 0.0f));

            if (glitchCooldown[index] > 0)
                --glitchCooldown[index];

            glitchBuffer[index][
                static_cast<size_t>(
                    glitchWriteIndex)] = wet;

            if (triggerChance > 0.0f
                && glitchRemaining[index] <= 0
                && glitchCooldown[index] <= 0
                && glitchRoll < triggerChance)
            {
                const float eventVariation =
                    0.55f
                    + 0.65f * localGlitch
                    + (nextRandom() * 2.0f - 1.0f)
                      * glitchVariation * 0.55f;

                glitchEventLength[index] =
                    juce::jlimit(
                        8,
                        16300,
                        static_cast<int>(
                            std::round(
                                requestedGlitchSamples
                                * eventVariation
                                * (0.86f + humanRandom * 0.30f)));

                glitchEventAge[index] = 0;
                glitchRemaining[index] =
                    glitchEventLength[index];

                glitchCooldown[index] =
                    juce::jlimit(
                        180,
                        10000,
                        requestedGlitchSamples * 2
                        + static_cast<int>(
                            nextRandom() * 2200.0f));

                glitchValue[index] = wet;
            }

            if (performActive && performType == 3)
                wet = juce::jmap(
                    performFade * 0.84f,
                    wet,
                    performHeldSample[index]);

            if (performActive && performType == 4)
            {
                const float failWave = std::sin(
                    movementPhase * (5.0f + chaosBoost * 17.0f)
                    + static_cast<float>(index));
                wet *= 0.58f + 0.42f * failWave;
            }

            if (glitchRemaining[index] > 0)
            {
                const int eventLength =
                    juce::jmax(
                        8,
                        glitchEventLength[index]);

                const float p =
                    juce::jlimit(
                        0.0f,
                        1.0f,
                        static_cast<float>(
                            glitchEventAge[index])
                        / static_cast<float>(
                            juce::jmax(
                                1,
                                eventLength - 1)));

                const float fadeCurve =
                    std::sin(
                        p * pi);

                const float glitchBlend =
                    juce::jlimit(
                        0.0f,
                        1.0f,
                        0.30f
                        + glitchFade * 0.70f)
                    * fadeCurve
                    * timelineLevel;

                switch (glitchMode)
                {
                    case 1: // Stutter.
                    {
                        const float smoothP =
                            p * p * (3.0f - 2.0f * p);

                        wet =
                            juce::jmap(
                                0.78f
                                * (1.0f
                                   - smoothP * 0.30f),
                                wet,
                                glitchValue[index]);
                        break;
                    }

                    case 2: // Repeat.
                    {
                        const int loopLength =
                            juce::jmax(
                                8,
                                juce::jmin(
                                    eventLength,
                                    16380));

                        int read =
                            glitchWriteIndex
                            - 1
                            - (glitchEventAge[index]
                               % loopLength);

                        while (read < 0)
                            read += 16384;

                        wet =
                            juce::jmap(
                                0.90f,
                                wet,
                                glitchBuffer[index][
                                    static_cast<size_t>(
                                        read)]);
                        break;
                    }

                    case 3: // Tape stop.
                    {
                        const float stop =
                            1.0f - p * p;

                        wet *=
                            0.06f
                            + 0.94f * stop;
                        break;
                    }

                    case 4: // Gate.
                    {
                        const float gate =
                            p < 0.78f
                                ? 0.10f
                                : (1.0f - p)
                                  * 0.45f;

                        wet *= gate;
                        break;
                    }

                    case 5: // Reverse.
                    {
                        const int loopLength =
                            juce::jmax(
                                8,
                                juce::jmin(
                                    eventLength,
                                    16380));

                        const int offset =
                            glitchEventAge[index]
                            % loopLength;

                        int read =
                            glitchWriteIndex
                            - 1
                            - (loopLength - 1 - offset);

                        while (read < 0)
                            read += 16384;

                        wet =
                            juce::jmap(
                                0.94f,
                                wet,
                                glitchBuffer[index][
                                    static_cast<size_t>(
                                        read)]);
                        break;
                    }

                    case 6: // Beat chop.
                    {
                        const int sub =
                            (glitchEventAge[index]
                             / juce::jmax(
                                 1,
                                 eventLength / 8)) & 1;

                        wet *=
                            sub == 0 ? 1.0f : 0.12f;
                        break;
                    }

                    default: // Freeze.
                    {
                        wet =
                            juce::jmap(
                                0.95f,
                                wet,
                                glitchValue[index]);
                        break;
                    }
                }

                wet =
                    juce::jmap(
                        glitchBlend,
                        wet,
                        glitchValue[index]);

                ++glitchEventAge[index];
                --glitchRemaining[index];
            }

            const float cutoffMod =
                localFilter
                + movementBipolar
                  * movement
                  * 0.16f;

            const float modCutoff =
                juce::jlimit(
                    100.0f,
                    static_cast<float>(
                        currentSampleRate)
                    * 0.44f,
                    baseCutoff
                    * std::pow(
                        2.0f,
                        cutoffMod));

            if (std::abs(
                    modCutoff
                    - lastFilterCutoff[index]) > 1.0f)
            {
                postFilter[index]
                    .setCutoffFrequency(
                        modCutoff);

                lastFilterCutoff[index] =
                    modCutoff;
            }

            wet =
                postFilter[index]
                    .processSample(
                        0,
                        wet);

            const float transientProtection =
                juce::jmap(
                    transient,
                    0.0f,
                    1.0f,
                    0.54f,
                    0.16f);

            const float bodyWet =
                juce::jmap(
                    body,
                    0.0f,
                    1.0f,
                    0.28f,
                    0.82f);

            const float dynamicWet =
                juce::jlimit(
                    0.0f,
                    1.0f,
                    bodyWet
                    - transientAmount
                      * transientProtection);

            // Shakal macro controls the wet contribution but never kills
            // the dry transient completely.
            float destroyed =
                juce::jmap(
                    dynamicWet
                    * envAmount,
                    dry,
                    wet);

            if (routing == 1)
            {
                const float preDamage =
                    shapedSample(
                        dry,
                        localDestroy
                        * 0.35f,
                        localCrush
                        * 0.40f);

                destroyed =
                    juce::jmap(
                        dynamicIntensity
                        * 0.65f,
                        preDamage,
                        destroyed);
            }

            if (routing == 2)
            {
                const float postDamage =
                    shapedSample(
                        destroyed,
                        localDestroy
                        * 0.45f,
                        localCrush
                        * 0.50f);

                destroyed =
                    juce::jmap(
                        dynamicIntensity
                        * 0.72f,
                        destroyed,
                        postDamage);
            }

            destroyed =
                juce::jmap(
                    smooth * 0.26f,
                    destroyed,
                    dry);

            const float wetAmount =
                clamp01(
                    dynamicIntensity
                    * (0.56f
                       + 0.44f
                         * localShakal));

            float out =
                juce::jmap(
                    mix * wetAmount,
                    dry,
                    destroyed);

            // v6 routing topology.
            switch (routingTopology)
            {
                case 1:
                    out =
                        dry * (0.52f - 0.18f * macro)
                        + out * (0.48f + 0.18f * macro);
                    break;

                case 2:
                    out =
                        juce::jmap(
                            juce::jlimit(0.0f, 1.0f,
                                         0.38f + reactiveSignal * 0.84f),
                            dry,
                            out);
                    break;

                case 3:
                    out =
                        juce::jmap(
                            juce::jlimit(0.0f, 1.0f,
                                         0.5f + 0.5f * movementBipolar),
                            dry,
                            out);
                    out *=
                        0.92f
                        + 0.08f * std::sin(morphPhase * 1.73f);
                    break;

                case 4:
                    out =
                        juce::jlimit(
                            -1.18f,
                            1.18f,
                            out
                            + feedbackState[index]
                              * feedback
                              * (0.08f + 0.06f * spaceBoost));
                    break;

                case 5:
                    out *=
                        index == 0
                            ? 1.0f + stereo * (0.16f + 0.20f * spaceBoost)
                            : 1.0f - stereo * (0.16f + 0.20f * spaceBoost);
                    break;

                default:
                    break;
            }

            grainBuffer[index][
                static_cast<size_t>(grainWriteIndex)] = out;

            feedbackWriteIndex =
                feedbackWriteIndex >= 8191
                    ? 0
                    : feedbackWriteIndex + 1;


            buffer.setSample(
                ch,
                sample,
                out);

            processedEnergy +=
                out * out;

            blockPeak =
                juce::jmax(
                    blockPeak,
                    std::abs(out));
        }

        if (performRemaining > 0)
        {
            --performRemaining;
            if (performRemaining == 0)
                performType = 0;
        }

        ++resonatorWriteIndex;

        if (resonatorWriteIndex
            >= static_cast<int>(
                resonatorBuffer[0].size()))
        {
            resonatorWriteIndex = 0;
        }

        ++glitchWriteIndex;
        ++grainWriteIndex;

        if (glitchWriteIndex >= 16384)
            glitchWriteIndex = 0;

        if (grainWriteIndex >= 16384)
            grainWriteIndex = 0;
    }

    // M/S post-stage.
    if (channels == 2
        && msMode != 0)
    {
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
                (left + right)
                * 0.5f;

            const float side =
                (left - right)
                * 0.5f;

            float outL = left;
            float outR = right;

            if (msMode == 1)
            {
                outL = mid;
                outR = mid;
            }
            else if (msMode == 2)
            {
                outL = side;
                outR = -side;
            }
            else
            {
                outL =
                    mid
                    + side * (1.0f + stereo * 0.40f);

                outR =
                    mid
                    - side * (1.0f + stereo * 0.40f);
            }

            buffer.setSample(
                0,
                sample,
                outL);

            buffer.setSample(
                1,
                sample,
                outR);
        }
    }
    else if (channels == 2
             && stereo > 0.001f)
    {
        const float width =
            1.0f
            + stereo * 0.34f;

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
                (left + right)
                * 0.5f;

            const float side =
                (left - right)
                * 0.5f;

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
                1.60f,
                inputRms
                / outputRms);

        autoMatchGain +=
            0.035f
            * (target
               - autoMatchGain);

        if (!std::isfinite(autoMatchGain))
            autoMatchGain = 1.0f;

        autoMatchGain =
            juce::jlimit(
                0.35f,
                2.0f,
                autoMatchGain);
    }
    else
    {
        autoMatchGain +=
            0.025f
            * (1.0f
               - autoMatchGain);
    }

    if (!std::isfinite(autoMatchGain))
        autoMatchGain = 1.0f;

    const float finalGain =
        juce::Decibels::decibelsToGain(
            outputDb)
        * juce::jlimit(
            0.35f,
            2.0f,
            autoMatchGain);

    if (std::isfinite(finalGain))
        buffer.applyGain(finalGain);

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
                    .processSample(
                        0,
                        x);

            const float noiseFloor =
                0.0008f + antiNoise * 0.0042f;
            const float sourceLevel =
                std::abs(dryBuffer.getSample(ch, sample));
            float gate =
                juce::jlimit(
                    0.0f,
                    1.0f,
                    (sourceLevel - noiseFloor * 0.35f)
                    / juce::jmax(0.0001f, noiseFloor * 1.25f));
            gate =
                gate * gate * (3.0f - 2.0f * gate);
            x *=
                juce::jmap(
                    antiNoise * 0.34f,
                    1.0f,
                    0.40f + gate * 0.60f);

            const float dcAlpha =
                0.00034f + antiDc * antiNoise * 0.0012f;
            dcBlockState[static_cast<size_t>(ch)] +=
                dcAlpha * (x - dcBlockState[static_cast<size_t>(ch)]);
            const float dcClean =
                x - dcBlockState[static_cast<size_t>(ch)];
            const float guardStrength =
                0.26f
                + smooth * 0.62f
                + antiNoise * antiPeak * 0.80f;

            x =
                softCeiling(
                    x,
                    guardStrength);

            if (!std::isfinite(x))
                x = 0.0f;

            x =
                juce::jlimit(
                    -1.25f,
                    1.25f,
                    x);

            x *= 0.96f;

            buffer.setSample(
                ch,
                sample,
                x);
        }
    }

    for (int sample = 0;
         sample < samples;
         sample += juce::jmax(1, samples / 32))
    {
        const int slot =
            scopeWriteIndex.fetch_add(1)
            & 255;

        const float left =
            buffer.getSample(0, sample);

        const float right =
            channels > 1
                ? buffer.getSample(1, sample)
                : left;

        scopeBuffer[
            static_cast<size_t>(slot)]
            .store(
                juce::jlimit(
                    -1.0f,
                    1.0f,
                    0.5f * (left + right)));
    }

    for (size_t band = 0;
         band < bandLevels.size();
         ++band)
    {
        const float previous =
            bandLevels[band].load();

        bandLevels[band].store(
            0.20f * bandPeak[band]
            + 0.80f * previous);
    }

    float glitchActivityValue = 0.0f;

    if (glitch > 0.0001f)
        for (int ch = 0; ch < channels; ++ch)
            if (glitchRemaining[static_cast<size_t>(ch)] > 0)
                glitchActivityValue = 1.0f;

    float modulationActivityValue = 0.0f;
    for (const auto state : modSmoothState)
        modulationActivityValue =
            juce::jmax(
                modulationActivityValue,
                std::abs(state));

    glitchActivity.store(
        0.15f * glitchActivityValue
        + 0.85f * glitchActivity.load());

    modulationActivity.store(
        0.12f
        * juce::jlimit(
            0.0f,
            1.0f,
            modulationActivityValue)
        + 0.88f * modulationActivity.load());

    const double blockElapsedMs =
        juce::Time::getMillisecondCounterHiRes()
        - blockStartMs;

    const double budgetMs =
        samples > 0
            ? static_cast<double>(samples)
              * 1000.0
              / currentSampleRate
            : 1.0;

    const float cpuPercent =
        static_cast<float>(
            juce::jlimit(
                0.0,
                4.0,
                blockElapsedMs
                / juce::jmax(
                    0.05,
                    budgetMs)));

    const float previousCpu =
        cpuLoad.load();

    cpuLoad.store(
        0.12f * cpuPercent
        + 0.88f * previousCpu);

    meterLevel.store(
        juce::jlimit(
            0.0f,
            1.0f,
            juce::jmax(
                blockPeak
                * finalGain
                * 0.96f,
                meterLevel.load()
                * 0.92f)));
}

juce::AudioProcessorEditor*
ShakalizerAudioProcessor::createEditor()
{
    return new ShakalizerAudioProcessorEditor(
        *this);
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
// CI V7 final validation marker.
