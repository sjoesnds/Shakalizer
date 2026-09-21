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
    addFloat("crush", "Crush", 0, 1, 0.34f * 1.0f, 0.34f);
    addFloat("decimate", "Decimate", 0, 1, 0.001f, 0.28f);
    addFloat("drive", "Drive", 0, 1, 0.001f, 0.30f);
    addFloat("clip", "Clip", 0, 1, 0.001f, 0.20f);
    addFloat("glitch", "Glitch", 0, 1, 0.001f, 0.08f);
    addFloat("jitter", "Jitter", 0, 1, 0.001f, 0.08f);

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

    for (int i = 1; i <= 4; ++i)
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
    morphPhase = 0.0f;
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

    const float split =
        clamp01(value("split"));
    const float transient =
        clamp01(value("transient"));
    const float body =
        clamp01(value("body"));
    const float stereo =
        clamp01(value("stereo"));
    const float movement =
        clamp01(value("movement"));
    const float unstable =
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

    float macro =
        shakal
        * (0.72f + 0.28f * modeScale);

    macro =
        juce::jmap(
            morph,
            macro,
            juce::jmin(
                1.0f,
                macro + 0.35f));

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
    float modToShakal = 0.0f;
    float modToDestroy = 0.0f;
    float modToCrush = 0.0f;
    float modToDecimate = 0.0f;
    float modToShatter = 0.0f;
    float modToFold = 0.0f;
    float modToShift = 0.0f;
    float modToGlitch = 0.0f;
    float modToFilter = 0.0f;

    const float modAmounts[4] {
        value("mod1Amount"),
        value("mod2Amount"),
        value("mod3Amount"),
        value("mod4Amount")
    };

    const int modSources[4] {
        choiceIndex(apvts, "mod1Source", 0),
        choiceIndex(apvts, "mod2Source", 0),
        choiceIndex(apvts, "mod3Source", 0),
        choiceIndex(apvts, "mod4Source", 0)
    };

    const int modDests[4] {
        choiceIndex(apvts, "mod1Dest", 0),
        choiceIndex(apvts, "mod2Dest", 0),
        choiceIndex(apvts, "mod3Dest", 0),
        choiceIndex(apvts, "mod4Dest", 0)
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
                bpm = *hostBpm;
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
            - smooth * 10500.0f);

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
        [this, drive, clip, preGain]
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

                data[sample] =
                    shapedSample(
                        x,
                        drive * 0.82f,
                        clip * 0.70f);
            }
        }
    };

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

    float inputEnergy = 0.0f;
    float processedEnergy = 0.0f;
    float blockPeak = 0.0f;

    for (int sample = 0;
         sample < samples;
         ++sample)
    {
        movementPhase +=
            movementIncrement;

        syncPhase +=
            movementIncrement;

        if (movementPhase >= 2.0f * pi)
            movementPhase -= 2.0f * pi;

        if (syncPhase >= 2.0f * pi)
            syncPhase -= 2.0f * pi;

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
                 slot < 4;
                 ++slot)
            {
                float source = 0.0f;

                switch (modSources[slot])
                {
                    case 1:
                        source =
                            bipolarToUnit(
                                movementBipolar);
                        break;

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

                applyMod(
                    modDests[slot],
                    signedSource
                    * modAmounts[slot],
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

            float dynamicIntensity =
                intensity;

            if (smart)
            {
                const float inputShape =
                    clamp01(
                        transientAmount * 0.70f
                        + bodyAmount * 0.30f);

                dynamicIntensity =
                    clamp01(
                        dynamicIntensity
                        * (0.78f
                           + inputShape * 0.34f));

                localShatter =
                    clamp01(
                        localShatter
                        * (0.80f
                           + inputShape * 0.28f));
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
                 character, resampled]
                (float band,
                 float amount)
            {
                if (amount <= 0.0001f)
                    return band;

                const float crushAmount =
                    clamp01(
                        localCrush
                        * (0.42f
                           + amount * 0.78f));

                const int bits =
                    juce::jlimit(
                        4,
                        16,
                        static_cast<int>(
                            std::round(
                                16.0f
                                - crushAmount
                                  * 10.0f)));

                const float levels =
                    static_cast<float>(
                        (1u << bits) - 1u);

                float x =
                    juce::jmap(
                        amount
                        * (0.64f
                           + localDecimate
                             * 0.25f),
                        band,
                        resampled);

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
                             * 0.25f
                             * crushAmount))
                        * levels)
                    / levels;

                x =
                    juce::jmap(
                        amount * 0.58f,
                        x,
                        quantized);

                x =
                    waveFold(
                        x,
                        localFold
                        * amount
                        * juce::jmap(
                            character,
                            0.45f,
                            1.08f));

                return softCeiling(
                    x,
                    0.08f
                    + amount * 0.17f);
            };

            const float shattered =
                destroyBand(low, lowAmt)
                + destroyBand(mid, midAmt)
                + destroyBand(high, highAmt)
                + destroyBand(air, airAmt);

            float wet =
                juce::jmap(
                    lowAmt,
                    source,
                    shattered);

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
                const float wrapped =
                    syncPhase
                    / (2.0f * pi)
                    - std::floor(
                        syncPhase
                        / (2.0f * pi));

                const int slot =
                    static_cast<int>(
                        wrapped
                        * gridSlots);

                gridBoundary =
                    slot
                    != lastGlitchGridSlot;

                if (gridBoundary)
                    lastGlitchGridSlot =
                        slot;
            }

            const float glitchChance =
                localGlitch
                * (0.008f
                   + dynamicIntensity
                     * 0.04f)
                * (gridSlots > 0
                    ? (gridBoundary
                        ? 0.42f
                        : 0.0f)
                    : 0.00006f);

            if (glitchCooldown[index] > 0)
                --glitchCooldown[index];

            if (glitchChance > 0.0f
                && glitchRemaining[index] <= 0
                && glitchCooldown[index] <= 0
                && nextRandom()
                   < glitchChance)
            {
                glitchRemaining[index] =
                    14
                    + static_cast<int>(
                        nextRandom()
                        * 220.0f);

                glitchCooldown[index] =
                    850
                    + static_cast<int>(
                        nextRandom()
                        * 5000.0f);

                glitchValue[index] =
                    wet;
            }

            if (glitchRemaining[index] > 0)
            {
                const float holdMix =
                    glitchRemaining[index] < 24
                        ? static_cast<float>(
                            glitchRemaining[index])
                          / 24.0f
                        : 1.0f;

                wet =
                    juce::jmap(
                        holdMix,
                        wet,
                        glitchValue[index]);

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

            postFilter[index]
                .setCutoffFrequency(
                    modCutoff);

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

            const float out =
                juce::jmap(
                    mix * wetAmount,
                    dry,
                    destroyed);

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

        ++resonatorWriteIndex;

        if (resonatorWriteIndex
            >= static_cast<int>(
                resonatorBuffer[0].size()))
        {
            resonatorWriteIndex = 0;
        }
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
    }
    else
    {
        autoMatchGain +=
            0.025f
            * (1.0f
               - autoMatchGain);
    }

    const float finalGain =
        juce::Decibels::decibelsToGain(
            outputDb)
        * autoMatchGain;

    buffer.applyGain(
        finalGain);

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

            x =
                softCeiling(
                    x,
                    0.26f
                    + smooth * 0.62f);

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
