#include "PluginEditor.h"

namespace
{
const juce::Colour background = juce::Colour::fromRGB(8, 7, 10);
const juce::Colour panel = juce::Colour::fromRGB(16, 14, 19);
const juce::Colour panel2 = juce::Colour::fromRGB(25, 22, 29);
const juce::Colour panel3 = juce::Colour::fromRGB(34, 30, 39);
const juce::Colour accent = juce::Colour::fromRGB(237, 35, 60);
const juce::Colour text = juce::Colour::fromRGB(247, 244, 247);
const juce::Colour muted = juce::Colour::fromRGB(151, 143, 154);
const juce::Colour line = juce::Colour::fromRGB(50, 44, 54);

void setNormalised(
    ShakalizerAudioProcessor& processor,
    const char* id,
    float value)
{
    if (auto* parameter =
            processor.getAPVTS().getParameter(id))
    {
        parameter->setValueNotifyingHost(
            juce::jlimit(
                0.0f,
                1.0f,
                value));
    }
}

void setChoice(
    ShakalizerAudioProcessor& processor,
    const char* id,
    int index,
    int count)
{
    if (count <= 1)
        return;

    setNormalised(
        processor,
        id,
        static_cast<float>(index)
        / static_cast<float>(count - 1));
}

void setBool(
    ShakalizerAudioProcessor& processor,
    const char* id,
    bool state)
{
    setNormalised(
        processor,
        id,
        state ? 1.0f : 0.0f);
}
}

ShakalizerAudioProcessorEditor::ShakalLookAndFeel::ShakalLookAndFeel()
{
    setColour(
        juce::Slider::thumbColourId,
        accent);

    setColour(
        juce::Slider::rotarySliderFillColourId,
        accent);

    setColour(
        juce::Slider::rotarySliderOutlineColourId,
        juce::Colour::fromRGB(
            62, 55, 67));

    setColour(
        juce::Slider::textBoxTextColourId,
        text);

    setColour(
        juce::Slider::textBoxBackgroundColourId,
        juce::Colours::transparentBlack);

    setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);

    setColour(
        juce::ComboBox::backgroundColourId,
        panel3);

    setColour(
        juce::ComboBox::outlineColourId,
        juce::Colour::fromRGB(
            70, 61, 73));

    setColour(
        juce::ComboBox::textColourId,
        text);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float position,
    float startAngle,
    float endAngle,
    juce::Slider&)
{
    auto bounds =
        juce::Rectangle<float>(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(width),
            static_cast<float>(height))
        .reduced(8.0f);

    const float radius =
        juce::jmin(
            bounds.getWidth(),
            bounds.getHeight())
        * 0.34f;

    const auto centre =
        bounds.getCentre();

    g.setColour(
        juce::Colour::fromRGB(
            38, 33, 42));

    g.fillEllipse(
        centre.x - radius,
        centre.y - radius,
        radius * 2.0f,
        radius * 2.0f);

    g.setColour(
        juce::Colour::fromRGB(
            79, 70, 84));

    g.drawEllipse(
        centre.x - radius,
        centre.y - radius,
        radius * 2.0f,
        radius * 2.0f,
        1.6f);

    const float angle =
        startAngle
        + position
          * (endAngle - startAngle);

    juce::Path arc;

    arc.addCentredArc(
        centre.x,
        centre.y,
        radius + 3.0f,
        radius + 3.0f,
        0.0f,
        startAngle,
        angle,
        true);

    g.setColour(accent);

    g.strokePath(
        arc,
        juce::PathStrokeType(
            width > 120 ? 3.5f : 2.8f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    const float marker =
        width > 120 ? 4.0f : 3.0f;

    const float px =
        centre.x
        + std::cos(
            angle
            - juce::MathConstants<float>::halfPi)
          * (radius - 4.0f);

    const float py =
        centre.y
        + std::sin(
            angle
            - juce::MathConstants<float>::halfPi)
          * (radius - 4.0f);

    g.setColour(text);

    g.fillEllipse(
        px - marker,
        py - marker,
        marker * 2.0f,
        marker * 2.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawComboBox(
    juce::Graphics& g,
    int width,
    int height,
    bool down,
    int, int, int, int,
    juce::ComboBox&)
{
    g.setColour(
        down
            ? panel2.brighter(0.08f)
            : panel3);

    g.fillRoundedRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        7.0f);

    g.setColour(
        accent.withAlpha(
            down ? 0.68f : 0.30f));

    g.drawRoundedRectangle(
        0.5f,
        0.5f,
        static_cast<float>(width - 1),
        static_cast<float>(height - 1),
        7.0f,
        1.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool highlighted,
    bool down)
{
    const auto bounds =
        button.getLocalBounds()
            .toFloat()
            .reduced(0.5f);

    g.setColour(
        (down || highlighted)
            ? panel3.brighter(0.08f)
            : panel3);

    g.fillRoundedRectangle(
        bounds,
        7.0f);

    g.setColour(
        button.getToggleState()
            ? accent
            : juce::Colour::fromRGB(
                70, 61, 73));

    g.drawRoundedRectangle(
        bounds,
        7.0f,
        button.getToggleState()
            ? 1.35f
            : 1.0f);
}

void ShakalizerAudioProcessorEditor::Meter::paint(
    juce::Graphics& g)
{
    const auto bounds =
        getLocalBounds()
            .toFloat()
            .reduced(1.0f);

    g.setColour(
        juce::Colour::fromRGB(
            38, 32, 42));

    g.fillRoundedRectangle(
        bounds,
        4.0f);

    const float h =
        bounds.getHeight() * level;

    g.setColour(accent);

    g.fillRoundedRectangle(
        bounds.withTop(
            bounds.getBottom() - h),
        4.0f);
}

void ShakalizerAudioProcessorEditor::Visualizer::paint(
    juce::Graphics& g)
{
    const auto b =
        getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(panel);
    g.fillRoundedRectangle(b, 6.0f);

    g.setColour(line);
    g.drawRoundedRectangle(b, 6.0f, 1.0f);

    const float barW = b.getWidth() / 32.0f;

    for (int i = 0; i < 32; ++i)
    {
        const float v =
            juce::jlimit(
                0.0f, 1.0f,
                processor.getSpectrumBin(i * 2));

        const float barH =
            v * (b.getHeight() - 4.0f);

        g.setColour(
            accent.withAlpha(0.16f + v * 0.46f));

        g.fillRect(
            b.getX() + static_cast<float>(i) * barW + 1.0f,
            b.getBottom() - barH - 2.0f,
            juce::jmax(1.0f, barW - 2.0f),
            barH);
    }

    const int write =
        processor.getScopeWriteIndex();

    juce::Path wave;

    for (int i = 0; i < 96; ++i)
    {
        const int idx = (write - 96 + i) & 255;
        const float sample =
            juce::jlimit(
                -1.0f, 1.0f,
                processor.getScopeSample(idx));

        const float x =
            b.getX()
            + (static_cast<float>(i) / 95.0f) * b.getWidth();

        const float y =
            b.getCentreY()
            - sample * b.getHeight() * 0.34f;

        if (i == 0)
            wave.startNewSubPath(x, y);
        else
            wave.lineTo(x, y);
    }

    g.setColour(text.withAlpha(0.82f));
    g.strokePath(wave, juce::PathStrokeType(1.0f));
}

ShakalizerAudioProcessorEditor::ShakalizerAudioProcessorEditor(
    ShakalizerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      visualizer(p),
      modSourceBoxes {
          &modSource1Box, &modSource2Box,
          &modSource3Box, &modSource4Box,
          &modSource5Box, &modSource6Box,
          &modSource7Box, &modSource8Box },
      modDestBoxes {
          &modDest1Box, &modDest2Box,
          &modDest3Box, &modDest4Box,
          &modDest5Box, &modDest6Box,
          &modDest7Box, &modDest8Box }
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(980, 620, 1500, 900);
    setSize(1180, 650);

    titleLabel.setText(
        "SHAKALIZER",
        juce::dontSendNotification);

    titleLabel.setFont(
        juce::Font(
            juce::FontOptions(
                31.0f,
                juce::Font::bold)));

    titleLabel.setColour(
        juce::Label::textColourId,
        text);

    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "FULL DIGITAL DESTRUCTION ENGINE",
        juce::dontSendNotification);

    subtitleLabel.setFont(
        juce::Font(
            juce::FontOptions(10.0f)));

    subtitleLabel.setColour(
        juce::Label::textColourId,
        muted);

    addAndMakeVisible(subtitleLabel);

    pageLabel.setText(
        "CORE",
        juce::dontSendNotification);
    pageLabel.setFont(
        juce::Font(
            juce::FontOptions(
                9.0f,
                juce::Font::bold)));
    pageLabel.setColour(
        juce::Label::textColourId,
        muted);
    addAndMakeVisible(pageLabel);

    for (size_t i = 0; i < pageButtons.size(); ++i)
    {
        auto& button = pageButtons[i];
        button.onClick = [this, i]
        {
            setPage(static_cast<int>(i));
        };
        button.setColour(
            juce::TextButton::textColourOnId,
            text);
        button.setColour(
            juce::TextButton::textColourOffId,
            text);
        addAndMakeVisible(button);
    }

    presetBox.addItem("CUSTOM", 1);
    presetBox.addItemList(
        {
            "INIT / SAFE",
            "VOCAL DIGITAL",
            "SHAKAL LEAD",
            "BROKEN 808",
            "PIXEL DRUM",
            "GLITCH GRID",
            "ALIEN",
            "MELT",
            "HARD SHATTER",
            "TAPE STUTTER",
            "REVERSE GRID",
            "SPECTRAL FREEZE",
            "CHAOS LAB",
            "FEEDBACK LOOP",
            "GRANULAR GLITCH",
            "REACTIVE SHATTER",
            "DIGITAL BASS",
            "CRUSHED KICK",
            "SYNCOPATED GLITCH",
            "BROKEN LEAD",
            "VHS DRUMS",
            "DARK RADIO",
            "SPACE CORRUPT",
            "FINAL SHAKAL"
        },
        2);

    modeBox.addItemList(
        { "Clean", "Crunch", "Shakal", "Destroy",
          "Fried", "Pixel", "Alien", "Melt", "Shatter" }, 1);
    resampleBox.addItemList(
        { "Hold", "Linear", "Stair", "Smear", "Random" }, 1);
    filterBox.addItemList(
        { "Low Pass", "Band Pass", "High Pass" }, 1);
    movementBox.addItemList(
        { "Sine", "Triangle", "Sample+Hold", "Stepped" }, 1);
    qualityBox.addItemList(
        { "1x", "2x", "4x" }, 1);
    syncBox.addItemList(
        { "Free", "1/4", "1/8", "1/16", "1/32" }, 1);
    glitchGridBox.addItemList(
        { "Free", "1/8", "1/16", "1/32" }, 1);
    glitchModeBox.addItemList(
        { "Freeze", "Stutter", "Repeat", "Tape Stop",
          "Gate", "Reverse", "Beat Chop" }, 1);
    glitchPatternBox.addItemList(
        { "Auto", "Straight", "Offbeat", "Syncopated",
          "Sparse", "Dense", "Burst" }, 1);
    glitchLengthBox.addItemList(
        { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2" }, 1);
    spectralModeBox.addItemList(
        { "Smooth", "Shatter", "Blur", "Freeze", "Bits", "Ring" }, 1);
    modWaveBox.addItemList(
        { "Sine", "Triangle", "Sample+Hold", "Stepped" }, 1);
    modSyncBox.addItemList(
        { "Free", "1/4", "1/8", "1/16", "1/32" }, 1);
    routingBox.addItemList(
        { "Standard", "Damage > Shatter",
          "Shatter > Damage", "Parallel" }, 1);
    msModeBox.addItemList(
        { "Stereo", "Mid", "Side", "Split" }, 1);
    liveSceneBox.addItemList(
        { "Normal", "Impact", "Glitch",
          "Melt", "Broken", "Chaos" }, 1);
    characterModeBox.addItemList(
        { "Neutral", "Digital", "VHS", "Console", "Radio",
          "Metallic", "Broken", "Alien", "Cheap DAC", "Corrupt" }, 1);
    fftWindowBox.addItemList(
        { "Hann", "Blackman", "Triangle", "Rect" }, 1);
    pitchModeBox.addItemList(
        { "Micro", "Semitone", "Octave", "Corrupt" }, 1);
    routingTopologyBox.addItemList(
        { "Serial", "Parallel", "Split", "Crossfade",
          "Feedback Loop", "Wide" }, 1);

    for (auto* box : {
        &presetBox, &modeBox, &resampleBox, &filterBox,
        &movementBox, &qualityBox, &syncBox,
        &glitchGridBox, &glitchModeBox, &glitchPatternBox,
        &glitchLengthBox,
        &spectralModeBox, &modWaveBox, &modSyncBox,
        &routingBox, &msModeBox,
        &liveSceneBox, &characterModeBox,
        &fftWindowBox, &pitchModeBox,
        &routingTopologyBox })
    {
        addAndMakeVisible(*box);
    }

    modeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "mode", modeBox);

    resampleAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "resampleMode", resampleBox);

    filterAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "filterType", filterBox);

    movementAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "movementShape", movementBox);

    qualityAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "quality", qualityBox);

    syncAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "syncRate", syncBox);

    glitchGridAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "glitchGrid", glitchGridBox);

    glitchModeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "glitchMode", glitchModeBox);

    glitchPatternAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.getAPVTS(), "glitchPattern", glitchPatternBox);

    glitchLengthAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "glitchLength", glitchLengthBox);

    spectralModeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "spectralMode", spectralModeBox);

    modWaveAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "modWave", modWaveBox);

    modSyncAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "modSync", modSyncBox);

    routingAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "routing", routingBox);

    msModeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "msMode", msModeBox);

    liveSceneAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "liveScene", liveSceneBox);

    characterModeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "characterMode", characterModeBox);
    fftWindowAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "fftWindow", fftWindowBox);
    pitchModeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "pitchMode", pitchModeBox);
    routingTopologyAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "routingTopology", routingTopologyBox);


    presetBox.onChange = [this]
    {
        const int selected =
            presetBox.getSelectedItemIndex();

        if (selected > 0)
            loadPreset(selected - 1);
    };

    const juce::StringArray modSources {
        "Off", "LFO", "Envelope",
        "Random", "Step", "Beat"
    };

    const juce::StringArray modDests {
        "None", "Shakal", "Destroy",
        "Crush", "Decimate", "Shatter",
        "Fold", "Shift", "Glitch", "Filter"
    };

    for (int i = 0; i < 8; ++i)
    {
        modSourceBoxes[
            static_cast<size_t>(i)]
            ->addItemList(
                modSources,
                1);

        modDestBoxes[
            static_cast<size_t>(i)]
            ->addItemList(
                modDests,
                1);

        addAndMakeVisible(
            *modSourceBoxes[
                static_cast<size_t>(i)]);

        addAndMakeVisible(
            *modDestBoxes[
                static_cast<size_t>(i)]);

        modSourceAttachments[
            static_cast<size_t>(i)] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.getAPVTS(),
                    "mod"
                    + juce::String(i + 1)
                    + "Source",
                    *modSourceBoxes[
                        static_cast<size_t>(i)]);

        modDestAttachments[
            static_cast<size_t>(i)] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                    processor.getAPVTS(),
                    "mod"
                    + juce::String(i + 1)
                    + "Dest",
                    *modDestBoxes[
                        static_cast<size_t>(i)]);
    }

    saveAButton.onClick = [this]
    {
        saveA();
    };

    abButton.onClick = [this]
    {
        swapAB();
    };

    saveCButton.onClick = [this]
    {
        saveC();
    };

    cdButton.onClick = [this]
    {
        swapCD();
    };

    smashButton.onClick = [this]
    {
        processor.triggerSmash();
    };

    glitchButton.onClick = [this]
    {
        processor.triggerGlitch();
    };

    freezeButton.onClick = [this]
    {
        processor.triggerFreeze();
    };

    failButton.onClick = [this]
    {
        processor.triggerFail();
    };

    autoMatchButton.onClick = [this]
    {
        toggleAutoMatch();
    };

    smartButton.onClick = [this]
    {
        toggleSmart();
    };

    randomAllButton.onClick = [this]
    {
        randomizeAll();
    };

    savePresetButton.onClick = [this]
    {
        savePresetToFile();
    };

    loadPresetButton.onClick = [this]
    {
        loadPresetFromFile();
    };

    favoritePresetButton.onClick = [this]
    {
        const int selected =
            presetBox.getSelectedItemIndex();

        if (selected > 0)
        {
            const int index =
                juce::jlimit(
                    0,
                    static_cast<int>(
                        favoritePresets.size()) - 1,
                    selected - 1);

            favoritePresets[
                static_cast<size_t>(index)] =
                !favoritePresets[
                    static_cast<size_t>(index)];

            favoritePresetButton.setToggleState(
                favoritePresets[
                    static_cast<size_t>(index)],
                juce::dontSendNotification);
        }
    };

    randomCoreButton.onClick = [this]
    {
        randomizeScope(1);
    };

    randomShatterButton.onClick = [this]
    {
        randomizeScope(2);
    };

    randomGlitchButton.onClick = [this]
    {
        randomizeScope(3);
    };

    randomModButton.onClick = [this]
    {
        randomizeScope(4);
    };

    for (auto* button : {
        &saveAButton,
        &abButton,
        &saveCButton,
        &cdButton,
        &smashButton,
        &glitchButton,
        &freezeButton,
        &failButton,
        &autoMatchButton,
        &smartButton,
        &randomAllButton,
        &savePresetButton,
        &loadPresetButton,
        &favoritePresetButton,
        &randomCoreButton,
        &randomShatterButton,
        &randomGlitchButton,
        &randomModButton
    })
    {
        button->setColour(
            juce::TextButton::textColourOnId,
            text);

        button->setColour(
            juce::TextButton::textColourOffId,
            text);

        addAndMakeVisible(
            *button);
    }

    meterLabel.setFont(
        juce::Font(
            juce::FontOptions(
                9.0f,
                juce::Font::bold)));

    meterLabel.setColour(
        juce::Label::textColourId,
        muted);

    meterLabel.setJustificationType(
        juce::Justification::centred);

    addAndMakeVisible(
        meterLabel);

    cpuLabel.setFont(
        juce::Font(
            juce::FontOptions(
                8.0f,
                juce::Font::bold)));

    cpuLabel.setColour(
        juce::Label::textColourId,
        muted);

    cpuLabel.setJustificationType(
        juce::Justification::centred);

    addAndMakeVisible(
        cpuLabel);

    addAndMakeVisible(
        meter);
    addAndMakeVisible(visualizer);

    const std::array<
        const char*, sliderCount> ids {{
        "shakal", "destroy", "crush", "decimate",
        "drive", "clip", "glitch", "jitter",
        "split", "transient", "body", "stereo",
        "movement", "unstable", "alien", "filterFreq",
        "filterRes", "mix", "output", "shatter",
        "fold", "shift", "resonance", "envFollow",
        "bandLow", "bandMid", "bandHigh", "bandAir",
        "character", "preGain", "smooth",
        "mod1Amount", "mod2Amount", "mod3Amount",
        "mod4Amount", "morph",
        "glitchDensity", "glitchProbability",
        "glitchFade", "glitchVariation",
        "spectralMix", "spectralSmear",
        "spectralFreezeAmount", "spectralBits",
        "spectralRing",
        "modRate", "modDepth", "modSmooth",
        "smartAmount", "smartBassProtect",
        "smartTransientProtect", "smartHighControl",
        "mod5Amount", "mod6Amount", "mod7Amount", "mod8Amount",
        "fftMix", "fftShatter", "fftFreeze", "fftBits", "fftShift",
        "grainMix", "grainSize", "grainPitch", "grainJitter",
        "feedback", "feedbackTone", "feedbackDrive", "pitchChaos",
        "timelineMix", "timelineStep1", "timelineStep2", "timelineStep3",
        "timelineStep4", "timelineStep5", "timelineStep6", "timelineStep7",
        "timelineStep8", "fftSpread", "fftThreshold", "fftWarp",
        "grainDensity", "grainPosition", "grainSpray", "grainReverse",
        "grainPan", "feedbackTime", "feedbackDiffusion", "feedbackFreeze",
        "feedbackSpread", "feedbackPitch", "pitchDamage", "pitchRange",
        "pitchDrift", "reactiveAmount", "reactiveTransient",
        "reactiveSpectral", "reactiveBass", "reactiveHigh",
        "macroCurve", "sceneMorphTime",
        "damageMacro", "motionMacro", "chaosMacro", "spaceMacro",
        "antiNoise", "antiDc", "antiAir", "antiPeak",
        "humanRandom", "audioAware", "chaosShape"
    }};

    for (int i = 0;
         i < sliderCount;
         ++i)
    {
        double min = 0.0;
        double max = 1.0;
        double step = 0.001;

        if (i == 15)
        {
            min = 80.0;
            max = 18000.0;
            step = 1.0;
        }
        else if (i == 16)
        {
            min = 0.05;
            max = 0.95;
        }
        else if (i == 18)
        {
            min = -12.0;
            max = 6.0;
            step = 0.01;
        }
        else if (i == 29)
        {
            min = -24.0;
            max = 12.0;
            step = 0.01;
        }
        else if (i >= 31
                 && i <= 34)
        {
            min = -1.0;
            max = 1.0;
            step = 0.001;
        }
        else if (i == 45)
        {
            min = 0.05;
            max = 20.0;
            step = 0.01;
        }
        else if (i >= 52 && i <= 55)
        {
            min = -1.0;
            max = 1.0;
            step = 0.001;
        }
        else if (i == 60)
        {
            min = -1.0;
            max = 1.0;
            step = 0.001;
        }
        else if (i >= 82 && i <= 84)
        {
            min = -1.0;
            max = 1.0;
            step = 0.001;
        }

        configureSlider(
            *sliders[
                static_cast<size_t>(i)],
            sliderNames[
                static_cast<size_t>(i)],
            min,
            max,
            step);

        addAndMakeVisible(
            *sliders[
                static_cast<size_t>(i)]);

        addAttachment(
            ids[
                static_cast<size_t>(i)],
            *sliders[
                static_cast<size_t>(i)]);
    }

    setPage(0);
    startTimerHz(20);
}

ShakalizerAudioProcessorEditor::~ShakalizerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ShakalizerAudioProcessorEditor::configureSlider(
    juce::Slider& slider,
    const juce::String& name,
    double min,
    double max,
    double step)
{
    slider.setName(name);

    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        70,
        18);

    slider.setRange(
        min,
        max,
        step);

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        text);

    slider.setTooltip(
        name);

    if (name == "FILTER FREQ")
    {
        slider.setSkewFactorFromMidPoint(
            1500.0);
    }

    if (name == "SHAKAL")
    {
        slider.setNumDecimalPlacesToDisplay(2);
    }
}

void ShakalizerAudioProcessorEditor::addAttachment(
    const juce::String& id,
    juce::Slider& slider)
{
    attachments.push_back(
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getAPVTS(),
                id,
                slider));
}

void ShakalizerAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(background);

    auto outer = getLocalBounds().reduced(8);

    g.setColour(panel);
    g.fillRoundedRectangle(
        outer.toFloat(),
        12.0f);

    g.setColour(accent);
    g.fillRoundedRectangle(
        8.0f, 8.0f, 5.0f,
        static_cast<float>(getHeight()) - 16.0f,
        2.0f);

    g.setColour(panel2);
    g.fillRoundedRectangle(
        22.0f, 108.0f,
        static_cast<float>(getWidth()) - 44.0f,
        static_cast<float>(getHeight()) - 124.0f,
        10.0f);

    g.setColour(line);
    g.drawHorizontalLine(
        104,
        24.0f,
        static_cast<float>(getWidth()) - 24.0f);

    g.setColour(accent.withAlpha(0.08f));
    g.fillEllipse(
        28.0f,
        124.0f,
        120.0f,
        120.0f);
}

void ShakalizerAudioProcessorEditor::resized()
{
    const int w = getWidth();
    titleLabel.setBounds(30, 15, 190, 32);
    subtitleLabel.setBounds(30, 44, 255, 15);

    presetBox.setBounds(224, 15, 106, 28);

    savePresetButton.setBounds(336, 15, 44, 28);
    loadPresetButton.setBounds(384, 15, 44, 28);
    favoritePresetButton.setBounds(432, 15, 42, 28);

    saveAButton.setBounds(480, 15, 52, 28);
    abButton.setBounds(536, 15, 44, 28);
    autoMatchButton.setBounds(584, 15, 48, 28);
    smartButton.setBounds(636, 15, 52, 28);

    randomCoreButton.setBounds(694, 15, 46, 28);
    randomShatterButton.setBounds(744, 15, 56, 28);
    randomGlitchButton.setBounds(804, 15, 50, 28);
    randomModButton.setBounds(858, 15, 46, 28);

    randomAllButton.setBounds(908, 15, 92, 28);
    pageLabel.setBounds(1008, 18, 86, 20);

    const int comboY1 = 56;
    const int comboY2 = 82;
    const int left = 24;
    const int right = w - 24;
    const int gap = 4;
    const int comboCount = 10;
    const int comboW =
        (right - left - gap * (comboCount - 1))
        / comboCount;

    auto setRow = [&](std::initializer_list<juce::Component*> boxes,
                      int y)
    {
        int x = left;
        for (auto* box : boxes)
        {
            box->setBounds(x, y, comboW, 22);
            x += comboW + gap;
        }
    };

    setRow({
        &modeBox, &resampleBox, &filterBox, &movementBox,
        &qualityBox, &syncBox, &glitchGridBox,
        &glitchModeBox, &glitchLengthBox, &spectralModeBox
    }, comboY1);

    setRow({
        &modWaveBox, &modSyncBox, &routingBox, &msModeBox,
        &liveSceneBox, &characterModeBox, &fftWindowBox,
        &pitchModeBox, &routingTopologyBox, &glitchPatternBox
    }, comboY2);

    presetBox.setBounds(w - 156, 15, 80, 28);

    const int visualizerX =
        juce::jmin(872, juce::jmax(700, w - 310));

    visualizer.setBounds(
        visualizerX,
        108,
        juce::jmax(72, w - visualizerX - 24),
        30);

    for (size_t i = 0; i < pageButtons.size(); ++i)
    {
        const int x = 30 + static_cast<int>(i) * 104;
        pageButtons[i].setBounds(x, 112, 98, 24);
        pageButtons[i].setToggleState(
            static_cast<int>(i) == currentPage,
            juce::dontSendNotification);
    }

    const int contentTop = 144;
    const int gapX = 2;
    const int gapY = 1;
    const int cols = 6;
    const int availableW = w - 48;
    const int cellW =
        (availableW - gapX * (cols - 1)) / cols;

    std::vector<int> visible;
    visible.reserve(static_cast<size_t>(sliderCount));

    for (int i = 0; i < sliderCount; ++i)
    {
        if (sliderBelongsToPage(i, currentPage))
            visible.push_back(i);
        else
            sliders[static_cast<size_t>(i)]->setVisible(false);
    }

    const int cellH = 70;

    for (size_t n = 0; n < visible.size(); ++n)
    {
        const int index = visible[n];
        const int row = static_cast<int>(n) / cols;
        const int col = static_cast<int>(n) % cols;

        auto* slider = sliders[static_cast<size_t>(index)];

        slider->setVisible(true);
        slider->setBounds(
            24 + col * (cellW + gapX),
            contentTop + row * (cellH + gapY),
            cellW,
            cellH);
    }

    // The modulation matrix is only shown on the MOD page.
    const bool modPage = currentPage == 3;

    const bool performPage = currentPage == 8;
    for (auto* button : { &saveCButton, &cdButton,
                          &smashButton, &glitchButton,
                          &freezeButton, &failButton })
        button->setVisible(performPage);

    if (performPage)
    {
        saveCButton.setBounds(30, contentTop + 152, 72, 26);
        cdButton.setBounds(106, contentTop + 152, 56, 26);
        smashButton.setBounds(30, contentTop + 184, 82, 26);
        glitchButton.setBounds(116, contentTop + 184, 82, 26);
        freezeButton.setBounds(202, contentTop + 184, 82, 26);
        failButton.setBounds(288, contentTop + 184, 82, 26);
    }

    for (int i = 0; i < 8; ++i)
    {
        auto* source = modSourceBoxes[static_cast<size_t>(i)];
        auto* dest = modDestBoxes[static_cast<size_t>(i)];

        source->setVisible(modPage);
        dest->setVisible(modPage);

        if (modPage)
        {
            const int y = contentTop + i * 56;
            source->setBounds(30, y + 8, 104, 22);
            dest->setBounds(138, y + 8, 112, 22);
        }
    }

    // On MOD page, the eight source/destination rows use the matching
    // modulation amount sliders: 1-4 and 5-8 are separated in the parameter list.
    if (modPage)
    {
        const std::array<int, 8> amountIndices {
            31, 32, 33, 34, 52, 53, 54, 55
        };

        for (int row = 0; row < 8; ++row)
        {
            const int sliderIndex =
                amountIndices[static_cast<size_t>(row)];

            auto* slider =
                sliders[static_cast<size_t>(sliderIndex)];

            slider->setVisible(true);
            slider->setBounds(
                258,
                contentTop + row * 56,
                190,
                50);
        }

        sliders[35]->setVisible(false); // Morph
        sliders[45]->setVisible(true);
        sliders[46]->setVisible(true);
        sliders[47]->setVisible(true);

        sliders[45]->setBounds(470, contentTop, 170, 72);
        sliders[46]->setBounds(650, contentTop, 170, 72);
        sliders[47]->setBounds(830, contentTop, 170, 72);
    }

    meterLabel.setBounds(w - 96, 46, 34, 12);
    cpuLabel.setBounds(w - 58, 46, 46, 12);
    meter.setBounds(w - 26, 44, 7, 30);
}

void ShakalizerAudioProcessorEditor::setPage(int page)
{
    currentPage =
        juce::jlimit(
            0,
            static_cast<int>(pageButtons.size()) - 1,
            page);

    static const char* names[] {
        "CORE", "GLITCH", "SPECTRAL", "MOD",
        "GRANULAR", "FEEDBACK", "REACTIVE", "MACRO", "PERFORM"
    };

    pageLabel.setText(
        names[currentPage],
        juce::dontSendNotification);

    for (size_t i = 0; i < pageButtons.size(); ++i)
    {
        pageButtons[i].setToggleState(
            static_cast<int>(i) == currentPage,
            juce::dontSendNotification);
    }

    resized();
    repaint();
}

bool ShakalizerAudioProcessorEditor::sliderBelongsToPage(
    int i, int page) const noexcept
{
    switch (page)
    {
        case 0: // Core.
            return (i >= 0 && i <= 5)
                || (i >= 8 && i <= 18)
                || (i >= 28 && i <= 30);

        case 1: // Glitch / timeline.
            return i == 6 || i == 7
                || (i >= 36 && i <= 39)
                || (i >= 68 && i <= 77);

        case 2: // Spectral / FFT.
            return (i >= 19 && i <= 27)
                || (i >= 40 && i <= 44)
                || (i >= 56 && i <= 60)
                || (i >= 78 && i <= 80);

        case 3: // Modulation matrix.
            return (i >= 31 && i <= 35)
                || (i >= 45 && i <= 47)
                || (i >= 52 && i <= 55);

        case 4: // Granular.
            return (i >= 61 && i <= 64)
                || (i >= 81 && i <= 85);

        case 5: // Feedback.
            return (i >= 65 && i <= 67)
                || (i >= 86 && i <= 90);

        case 6: // Reactive / pitch.
            return (i >= 48 && i <= 51)
                || (i >= 91 && i <= 100);

        case 7: // Master macros.
            return i == 0
                || i == 17
                || (i >= 101 && i <= 104);

        case 8: // Performance / intelligent cleanup.
            return i >= 105 && i <= 111;

        default:
            return false;
    }
}

void ShakalizerAudioProcessorEditor::timerCallback()
{
    meter.setLevel(
        processor.getMeterLevel());

    const bool autoOn =
        processor.getAPVTS()
            .getRawParameterValue(
                "autoMatch")
            ->load() > 0.5f;

    const bool smartOn =
        processor.getAPVTS()
            .getRawParameterValue(
                "smart")
            ->load() > 0.5f;

    const int selectedPreset =
        presetBox.getSelectedItemIndex();

    const bool favoriteOn =
        selectedPreset > 0
        && favoritePresets[
            static_cast<size_t>(
                juce::jlimit(
                    0,
                    static_cast<int>(
                        favoritePresets.size()) - 1,
                    selectedPreset - 1))];

    favoritePresetButton.setToggleState(
        favoriteOn,
        juce::dontSendNotification);

    autoMatchButton.setToggleState(
        autoOn,
        juce::dontSendNotification);

    smartButton.setToggleState(
        smartOn,
        juce::dontSendNotification);

    const float cpu =
        juce::jlimit(
            0.0f,
            400.0f,
            processor.getCpuLoad() * 100.0f);

    cpuLabel.setText(
        "CPU "
        + juce::String(
            cpu,
            1)
        + "%",
        juce::dontSendNotification);

    const float glitchActivity =
        juce::jlimit(
            0.0f,
            1.0f,
            processor.getGlitchActivity());

    const float modActivity =
        juce::jlimit(
            0.0f,
            1.0f,
            processor.getModulationActivity());

    visualizer.repaint();

    subtitleLabel.setText(
        "DESTRUCTION "
        + juce::String(glitchActivity * 100.0f, 0)
        + "%  MOD "
        + juce::String(modActivity * 100.0f, 0) + "%",
        juce::dontSendNotification);

    repaint();
}


void ShakalizerAudioProcessorEditor::savePresetToFile()
{
    presetFileChooser =
        std::make_unique<juce::FileChooser>(
            "Save Shakalizer Preset",
            juce::File::getSpecialLocation(
                juce::File::userDocumentsDirectory),
            "*.shakal");

    presetFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (file != juce::File())
            {
                auto xml =
                    processor.getAPVTS()
                        .copyState()
                        .createXml();

                if (xml != nullptr)
                    file.replaceWithText(
                        xml->toString());
            }

        },
        nullptr);
}

void ShakalizerAudioProcessorEditor::loadPresetFromFile()
{
    presetFileChooser =
        std::make_unique<juce::FileChooser>(
            "Load Shakalizer Preset",
            juce::File::getSpecialLocation(
                juce::File::userDocumentsDirectory),
            "*.shakal");

    presetFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();

            if (file != juce::File())
            {
                if (auto xml = juce::parseXML(file))
                {
                    if (xml->hasTagName(
                            processor.getAPVTS()
                                .state.getType()))
                    {
                        processor.getAPVTS().replaceState(
                            juce::ValueTree::fromXml(*xml));

                        presetBox.setSelectedId(
                            1,
                            juce::dontSendNotification);
                    }
                }
            }

        },
        nullptr);
}

void ShakalizerAudioProcessorEditor::saveA()
{
    captureState(
        abState);

    hasAState =
        !abState.empty();
}

void ShakalizerAudioProcessorEditor::saveC()
{
    captureState(cdState);
    hasCState = !cdState.empty();
}

void ShakalizerAudioProcessorEditor::swapCD()
{
    if (!hasCState)
    {
        saveC();
        return;
    }

    std::vector<float> current;
    captureState(current);
    applyState(cdState);
    cdState = std::move(current);
    presetBox.setSelectedId(1, juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::swapAB()
{
    if (!hasAState)
    {
        saveA();
        return;
    }

    std::vector<float> current;

    captureState(current);
    applyState(abState);
    abState = std::move(current);

    presetBox.setSelectedId(
        1,
        juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::toggleAutoMatch()
{
    if (auto* parameter =
            processor.getAPVTS()
                .getParameter(
                    "autoMatch"))
    {
        const bool current =
            parameter->getValue() > 0.5f;

        parameter->setValueNotifyingHost(
            current ? 0.0f : 1.0f);
    }
}

void ShakalizerAudioProcessorEditor::toggleSmart()
{
    if (auto* parameter =
            processor.getAPVTS()
                .getParameter(
                    "smart"))
    {
        const bool current =
            parameter->getValue() > 0.5f;

        parameter->setValueNotifyingHost(
            current ? 0.0f : 1.0f);
    }
}

void ShakalizerAudioProcessorEditor::captureState(
    std::vector<float>& destination)
{
    destination.clear();

    destination.reserve(
        static_cast<size_t>(
            processor.getParameters().size()));

    for (auto* parameter :
         processor.getParameters())
    {
        if (parameter != nullptr)
            destination.push_back(
                parameter->getValue());
    }
}

void ShakalizerAudioProcessorEditor::applyState(
    const std::vector<float>& state)
{
    const int count =
        juce::jmin(
            static_cast<int>(
                state.size()),
            processor.getParameters().size());

    for (int i = 0;
         i < count;
         ++i)
    {
        if (auto* parameter =
                processor.getParameters()[i])
        {
            parameter->setValueNotifyingHost(
                juce::jlimit(
                    0.0f,
                    1.0f,
                    state[
                        static_cast<size_t>(
                            i)]));
        }
    }
}

void ShakalizerAudioProcessorEditor::randomizeAll()
{
    // Smart random: first randomize the complete state, then shape the
    // result into a coherent, usable effect instead of pure chaos.
    static juce::Random random;

    for (auto* parameter : processor.getParameters())
    {
        if (parameter == nullptr)
            continue;

        if (dynamic_cast<juce::AudioParameterBool*>(
                parameter) != nullptr)
        {
            parameter->setValueNotifyingHost(
                random.nextFloat() > 0.78f ? 1.0f : 0.0f);
        }
        else
        {
            parameter->setValueNotifyingHost(
                juce::jlimit(
                    0.02f,
                    0.94f,
                    0.10f + random.nextFloat() * 0.84f));
        }
    }

    auto setNorm =
        [this](const char* id, float v)
    {
        if (auto* parameter =
                processor.getAPVTS().getParameter(id))
        {
            parameter->setValueNotifyingHost(
                juce::jlimit(0.0f, 1.0f, v));
        }
    };

    auto setChoice =
        [this](const char* id, int count)
    {
        if (auto* parameter =
                processor.getAPVTS().getParameter(id))
        {
            if (count > 1)
            {
                const int index =
                    random.nextInt(count);

                parameter->setValueNotifyingHost(
                    static_cast<float>(index)
                    / static_cast<float>(count - 1));
            }
        }
    };

    // Give the patch a clear main personality.
    const float main =
        0.35f + random.nextFloat() * 0.55f;

    setNorm("shakal", main);
    setNorm("destroy", 0.18f + main * 0.56f
                        + random.nextFloat() * 0.16f);
    setNorm("crush", 0.08f + main * 0.40f
                     + random.nextFloat() * 0.12f);
    setNorm("decimate", 0.04f + main * 0.30f
                        + random.nextFloat() * 0.10f);
    setNorm("drive", 0.10f + main * 0.34f
                     + random.nextFloat() * 0.10f);
    setNorm("clip", 0.08f + main * 0.30f);
    setNorm("shatter", 0.10f + main * 0.55f);
    setNorm("spectralMix", 0.35f + random.nextFloat() * 0.55f);
    setNorm("smooth", 0.18f + random.nextFloat() * 0.55f);
    setNorm("mix", 0.62f + random.nextFloat() * 0.32f);
    setNorm("output", 0.56f + random.nextFloat() * 0.20f);

    // Keep the dangerous combinations from stacking all at once.
    float feedback =
        processor.getAPVTS()
            .getRawParameterValue("feedback")->load();

    float grainMix =
        processor.getAPVTS()
            .getRawParameterValue("grainMix")->load();

    float resonance =
        processor.getAPVTS()
            .getRawParameterValue("resonance")->load();

    float shift =
        processor.getAPVTS()
            .getRawParameterValue("shift")->load();

    if (feedback > 0.58f)
    {
        setNorm(
            "feedbackDrive",
            std::min(
                processor.getAPVTS()
                    .getRawParameterValue("feedbackDrive")
                    ->load(),
                0.46f));

        setNorm(
            "feedbackDiffusion",
            std::min(
                processor.getAPVTS()
                    .getRawParameterValue("feedbackDiffusion")
                    ->load(),
                0.54f));

        setNorm(
            "grainMix",
            std::min(grainMix, 0.42f));

        setNorm(
            "resonance",
            std::min(resonance, 0.48f));
    }

    if (grainMix > 0.62f)
    {
        setNorm("glitch", std::min(
            processor.getAPVTS()
                .getRawParameterValue("glitch")->load(),
            0.46f));

        setNorm(
            "feedback",
            std::min(
                processor.getAPVTS()
                    .getRawParameterValue("feedback")
                    ->load(),
                0.48f));
    }

    if (shift > 0.68f)
    {
        setNorm(
            "resonance",
            std::min(
                processor.getAPVTS()
                    .getRawParameterValue("resonance")
                    ->load(),
                0.36f));
    }

    // Avoid choosing the heaviest possible combination too often.
    setChoice("quality", random.nextFloat() < 0.45f ? 2 : 1);
    setChoice("mode", 9);
    setChoice("resampleMode", 5);
    setChoice("glitchMode", 7);
    setChoice("glitchLength", 6);
    setChoice("spectralMode", 6);
    setChoice("routingTopology", 6);
    setChoice("characterMode", 10);
    setChoice("pitchMode", 4);
    setChoice("fftWindow", 4);
    setChoice("routing", 4);
    setChoice("msMode", 4);
    setChoice("liveScene", 6);
    setChoice("syncRate", random.nextFloat() < 0.65f ? 5 : 1);
    setChoice("glitchGrid", 4);
    setChoice("movementShape", 4);
    setChoice("modWave", 4);
    setChoice("modSync", 5);

    // Randomize modulation slots while keeping mostly sparse routing.
    for (int i = 1; i <= 8; ++i)
    {
        setChoice(
            ("mod" + juce::String(i) + "Source").toRawUTF8(),
            6);

        setChoice(
            ("mod" + juce::String(i) + "Dest").toRawUTF8(),
            10);

        setNorm(
            ("mod" + juce::String(i) + "Amount").toRawUTF8(),
            0.22f + random.nextFloat() * 0.56f);
    }

    // A few parameter relationships are deliberately biased.
    setNorm("glitchDensity", 0.12f + random.nextFloat() * 0.64f);
    setNorm("glitchProbability", 0.10f + random.nextFloat() * 0.65f);
    setNorm("glitchFade", 0.30f + random.nextFloat() * 0.62f);
    setNorm("timelineMix", 0.15f + random.nextFloat() * 0.70f);

    setNorm("grainDensity", 0.08f + random.nextFloat() * 0.58f);
    setNorm("grainSize", 0.10f + random.nextFloat() * 0.55f);
    setNorm("grainSpray", random.nextFloat() * 0.46f);
    setNorm("grainJitter", random.nextFloat() * 0.42f);

    setNorm("feedbackTime", 0.10f + random.nextFloat() * 0.54f);
    setNorm("feedbackTone", 0.22f + random.nextFloat() * 0.68f);
    setNorm("feedbackPitch", 0.30f + random.nextFloat() * 0.40f);

    setNorm("reactiveAmount", random.nextFloat() * 0.78f);
    setNorm("smartAmount", 0.30f + random.nextFloat() * 0.60f);
    setNorm("macroCurve", 0.20f + random.nextFloat() * 0.68f);
    setNorm("damageMacro", 0.12f + main * 0.68f);
    setNorm("motionMacro", random.nextFloat() * 0.72f);
    setNorm("chaosMacro", 0.05f + random.nextFloat() * 0.70f);
    setNorm("spaceMacro", random.nextFloat() * 0.58f);
    setChoice("glitchPattern", 7);

    // Auto Match and Smart stay available, but Smart is favored for random patches.
    setNorm(
        "autoMatch",
        random.nextFloat() > 0.42f ? 1.0f : 0.0f);

    setNorm(
        "smart",
        random.nextFloat() > 0.30f ? 1.0f : 0.0f);

    // Timeline: create a sparse rhythmic pattern rather than eight random blobs.
    for (int i = 1; i <= 8; ++i)
    {
        const float hit =
            (i == 1 || i == 5)
                ? 0.55f + random.nextFloat() * 0.40f
                : random.nextFloat() > 0.65f
                    ? random.nextFloat() * 0.55f
                    : 0.0f;

        setNorm(
            ("timelineStep" + juce::String(i)).toRawUTF8(),
            hit);
    }

    presetBox.setSelectedId(
        1,
        juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::randomizeScope(
    int scope)
{
    static juce::Random random;

    const std::array<
        const char*, 9> core {
        "shakal", "destroy", "crush",
        "decimate", "drive", "clip",
        "preGain", "smooth", "character"
    };

    const std::array<
        const char*, 18> spectral {
        "shatter", "fold", "shift",
        "resonance", "envFollow",
        "bandLow", "bandMid", "bandHigh",
        "bandAir", "character", "morph",
        "filterFreq", "spectralMode",
        "spectralMix", "spectralSmear",
        "spectralFreezeAmount", "spectralBits",
        "spectralRing"
    };

    const std::array<
        const char*, 68> glitch {
        "glitch", "jitter", "movement",
        "unstable", "stereo", "alien",
        "glitchMode", "glitchLength",
        "glitchDensity", "glitchProbability",
        "glitchFade", "glitchVariation",
        "mod1Amount", "mod2Amount",
        "mod3Amount", "mod4Amount",
        "morph", "mix",
        "smartAmount", "smartBassProtect",
        "smartTransientProtect", "smartHighControl",
        "fftMix", "fftShatter", "fftFreeze", "fftBits", "fftShift",
        "grainMix", "grainSize", "grainPitch", "grainJitter",
        "feedback", "feedbackTone", "feedbackDrive", "pitchChaos",
        "timelineMix", "timelineStep1", "timelineStep2", "timelineStep3",
        "timelineStep4", "timelineStep5", "timelineStep6", "timelineStep7",
        "timelineStep8", "characterMode",
        "fftSpread", "fftThreshold", "fftWarp",
        "grainDensity", "grainPosition", "grainSpray", "grainReverse",
        "grainPan", "feedbackTime", "feedbackDiffusion", "feedbackFreeze",
        "feedbackSpread", "feedbackPitch", "pitchDamage", "pitchRange",
        "pitchDrift", "reactiveAmount", "reactiveTransient",
        "reactiveSpectral", "reactiveBass", "reactiveHigh",
        "macroCurve", "sceneMorphTime"
    };

    const auto randomizeIds =
        [this](const auto& ids)
    {
        for (const auto* id : ids)
        {
            if (auto* parameter =
                    processor.getAPVTS()
                        .getParameter(id))
            {
                parameter->setValueNotifyingHost(
                    random.nextFloat());
            }
        }
    };

    const std::array<
        const char*, 49> modulation {
        "mod1Amount", "mod2Amount", "mod3Amount", "mod4Amount",
        "mod5Amount", "mod6Amount", "mod7Amount", "mod8Amount",
        "modRate", "modDepth", "modSmooth",
        "modWave", "modSync",
        "fftMix", "fftShatter", "fftFreeze", "fftBits", "fftShift",
        "grainMix", "grainSize", "grainPitch", "grainJitter",
        "feedback", "feedbackTone", "feedbackDrive", "pitchChaos",
        "fftSpread", "fftThreshold", "fftWarp",
        "grainDensity", "grainPosition", "grainSpray", "grainReverse",
        "grainPan", "feedbackTime", "feedbackDiffusion", "feedbackFreeze",
        "feedbackSpread", "feedbackPitch", "pitchDamage", "pitchRange",
        "pitchDrift", "reactiveAmount", "reactiveTransient",
        "reactiveSpectral", "reactiveBass", "reactiveHigh",
        "macroCurve", "sceneMorphTime"
    };

    if (scope == 1)
        randomizeIds(core);
    else if (scope == 2)
        randomizeIds(spectral);
    else if (scope == 3)
        randomizeIds(glitch);
    else if (scope == 4)
        randomizeIds(modulation);

    presetBox.setSelectedId(
        1,
        juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::loadPreset(
    int index)
{
    const auto base = [this]
    {
        const std::array<
            std::pair<const char*, float>, 31> values {{
            { "shakal", 0.50f },
            { "destroy", 0.42f },
            { "crush", 0.34f },
            { "decimate", 0.28f },
            { "drive", 0.30f },
            { "clip", 0.20f },
            { "glitch", 0.08f },
            { "jitter", 0.08f },
            { "split", 0.56f },
            { "transient", 0.74f },
            { "body", 0.55f },
            { "stereo", 0.18f },
            { "movement", 0.14f },
            { "unstable", 0.06f },
            { "alien", 0.02f },
            { "filterFreq", 0.80f },
            { "filterRes", 0.30f },
            { "mix", 0.76f },
            { "output", 0.58f },
            { "shatter", 0.24f },
            { "fold", 0.06f },
            { "shift", 0.00f },
            { "resonance", 0.00f },
            { "envFollow", 0.28f },
            { "bandLow", 0.10f },
            { "bandMid", 0.52f },
            { "bandHigh", 0.68f },
            { "bandAir", 0.42f },
            { "character", 0.52f },
            { "preGain", 0.667f },
            { "smooth", 0.34f }
        }};

        for (const auto& [id, amount] : values)
            setNormalised(
                processor,
                id,
                amount);

        for (int i = 1; i <= 8; ++i)
        {
            setNormalised(
                processor,
                ("mod"
                 + juce::String(i)
                 + "Amount").toRawUTF8(),
                0.5f);
        }

        setNormalised(processor, "morph", 0.0f);
        setNormalised(processor, "glitchDensity", 0.32f);
        setNormalised(processor, "glitchProbability", 0.42f);
        setNormalised(processor, "glitchFade", 0.56f);
        setNormalised(processor, "glitchVariation", 0.28f);
        setNormalised(processor, "spectralMix", 0.72f);
        setNormalised(processor, "spectralSmear", 0.16f);
        setNormalised(processor, "spectralFreezeAmount", 0.0f);
        setNormalised(processor, "spectralBits", 0.08f);
        setNormalised(processor, "spectralRing", 0.0f);
        setNormalised(
            processor,
            "modRate",
            (1.25f - 0.05f)
            / (20.0f - 0.05f));
        setNormalised(processor, "modDepth", 0.70f);
        setNormalised(processor, "modSmooth", 0.54f);
        setNormalised(processor, "smartAmount", 0.72f);
        setNormalised(processor, "smartBassProtect", 0.76f);
        setNormalised(processor, "smartTransientProtect", 0.72f);
        setNormalised(processor, "smartHighControl", 0.68f);
        setChoice(processor, "modWave", 0, 4);
        setChoice(processor, "modSync", 0, 5);

        setChoice(
            processor, "mode", 2, 9);
        setChoice(
            processor, "resampleMode", 1, 5);
        setChoice(
            processor, "filterType", 0, 3);
        setChoice(
            processor, "movementShape", 0, 4);
        setChoice(
            processor, "quality", 2, 3);
        setChoice(
            processor, "syncRate", 0, 5);
        setChoice(
            processor, "glitchGrid", 0, 4);
        setChoice(
            processor, "glitchMode", 1, 7);
        setChoice(
            processor, "glitchLength", 2, 6);
        setChoice(
            processor, "spectralMode", 1, 6);
        setChoice(
            processor, "routing", 0, 4);
        setChoice(
            processor, "msMode", 0, 4);
        setChoice(
            processor, "liveScene", 0, 6);

        for (int i = 1; i <= 8; ++i)
        {
            setChoice(
                processor,
                ("mod"
                 + juce::String(i)
                 + "Source").toRawUTF8(),
                0,
                6);

            setChoice(
                processor,
                ("mod"
                 + juce::String(i)
                 + "Dest").toRawUTF8(),
                0,
                10);
        }

        setBool(
            processor,
            "autoMatch",
            false);

        setBool(
            processor,
            "smart",
            false);

        setChoice(processor, "characterMode", 0, 10);
        setNormalised(processor, "fftMix", 0.0f);
        setNormalised(processor, "fftShatter", 0.0f);
        setNormalised(processor, "fftFreeze", 0.0f);
        setNormalised(processor, "fftBits", 0.0f);
        setNormalised(processor, "fftShift", 0.0f);
        setNormalised(processor, "grainMix", 0.0f);
        setNormalised(processor, "grainSize", 0.28f);
        setNormalised(processor, "grainPitch", 0.50f);
        setNormalised(processor, "grainJitter", 0.10f);
        setNormalised(processor, "feedback", 0.0f);
        setNormalised(processor, "feedbackTone", 0.48f);
        setNormalised(processor, "feedbackDrive", 0.16f);
        setNormalised(processor, "pitchChaos", 0.0f);
        setNormalised(processor, "timelineMix", 0.0f);

        for (int i = 1; i <= 8; ++i)
            setNormalised(
                processor,
                ("timelineStep" + juce::String(i)).toRawUTF8(),
                (i == 1 || i == 5) ? 0.85f : 0.0f);

        setNormalised(processor, "fftSpread", 0.18f);
        setNormalised(processor, "fftThreshold", 0.0f);
        setNormalised(processor, "fftWarp", 0.0f);
        setNormalised(processor, "grainDensity", 0.18f);
        setNormalised(processor, "grainPosition", 0.50f);
        setNormalised(processor, "grainSpray", 0.08f);
        setNormalised(processor, "grainReverse", 0.0f);
        setNormalised(processor, "grainPan", 0.50f);
        setNormalised(processor, "feedbackTime", 0.22f);
        setNormalised(processor, "feedbackDiffusion", 0.12f);
        setNormalised(processor, "feedbackFreeze", 0.0f);
        setNormalised(processor, "feedbackSpread", 0.18f);
        setNormalised(processor, "feedbackPitch", 0.50f);
        setNormalised(processor, "pitchDamage", 0.0f);
        setNormalised(processor, "pitchRange", 0.32f);
        setNormalised(processor, "pitchDrift", 0.0f);
        setNormalised(processor, "reactiveAmount", 0.0f);
        setNormalised(processor, "reactiveTransient", 0.55f);
        setNormalised(processor, "reactiveSpectral", 0.38f);
        setNormalised(processor, "reactiveBass", 0.22f);
        setNormalised(processor, "reactiveHigh", 0.44f);
        setNormalised(processor, "macroCurve", 0.50f);
        setNormalised(processor, "sceneMorphTime", 0.50f);
        setNormalised(processor, "damageMacro", 0.0f);
        setNormalised(processor, "motionMacro", 0.0f);
        setNormalised(processor, "chaosMacro", 0.0f);
        setNormalised(processor, "spaceMacro", 0.0f);
    setNormalised(processor, "antiNoise", 0.46f);
    setNormalised(processor, "antiDc", 0.70f);
    setNormalised(processor, "antiAir", 0.62f);
    setNormalised(processor, "antiPeak", 0.72f);
    setNormalised(processor, "humanRandom", 0.68f);
    setNormalised(processor, "audioAware", 0.66f);
    setNormalised(processor, "chaosShape", 0.58f);
        setChoice(processor, "fftWindow", 0, 4);
        setChoice(processor, "pitchMode", 0, 4);
        setChoice(processor, "routingTopology", 0, 6);
        setChoice(processor, "glitchPattern", 0, 7);
    };

    base();

    switch (index)
    {
        case 1:
            setNormalised(processor, "shakal", 0.56f);
            setNormalised(processor, "destroy", 0.44f);
            setNormalised(processor, "crush", 0.30f);
            setNormalised(processor, "decimate", 0.20f);
            setNormalised(processor, "drive", 0.32f);
            setNormalised(processor, "shatter", 0.15f);
            setNormalised(processor, "bandMid", 0.28f);
            setNormalised(processor, "bandHigh", 0.42f);
            setNormalised(processor, "bandAir", 0.18f);
            setNormalised(processor, "smooth", 0.58f);
            setBool(processor, "smart", true);
            break;

        case 2:
            setNormalised(processor, "shakal", 0.76f);
            setNormalised(processor, "destroy", 0.58f);
            setNormalised(processor, "crush", 0.40f);
            setNormalised(processor, "decimate", 0.34f);
            setNormalised(processor, "drive", 0.44f);
            setNormalised(processor, "shatter", 0.34f);
            setNormalised(processor, "bandMid", 0.58f);
            setNormalised(processor, "bandHigh", 0.72f);
            setNormalised(processor, "fold", 0.12f);
            setNormalised(processor, "smooth", 0.44f);
            setChoice(processor, "mode", 2, 9);
            break;

        case 3:
            setNormalised(processor, "shakal", 0.66f);
            setNormalised(processor, "destroy", 0.50f);
            setNormalised(processor, "crush", 0.36f);
            setNormalised(processor, "decimate", 0.34f);
            setNormalised(processor, "drive", 0.34f);
            setNormalised(processor, "split", 0.80f);
            setNormalised(processor, "bandLow", 0.03f);
            setNormalised(processor, "bandMid", 0.32f);
            setNormalised(processor, "bandHigh", 0.34f);
            setNormalised(processor, "bandAir", 0.08f);
            setNormalised(processor, "smooth", 0.64f);
            setChoice(processor, "mode", 3, 9);
            break;

        case 4:
            setNormalised(processor, "shakal", 0.68f);
            setNormalised(processor, "destroy", 0.60f);
            setNormalised(processor, "crush", 0.48f);
            setNormalised(processor, "decimate", 0.42f);
            setNormalised(processor, "glitch", 0.18f);
            setNormalised(processor, "jitter", 0.16f);
            setNormalised(processor, "shatter", 0.34f);
            setNormalised(processor, "bandHigh", 0.72f);
            setNormalised(processor, "bandAir", 0.46f);
            setNormalised(processor, "smooth", 0.38f);
            setChoice(processor, "mode", 5, 9);
            setChoice(processor, "resampleMode", 2, 5);
            break;

        case 5:
            setNormalised(processor, "shakal", 0.60f);
            setNormalised(processor, "destroy", 0.44f);
            setNormalised(processor, "glitch", 0.42f);
            setNormalised(processor, "jitter", 0.30f);
            setNormalised(processor, "movement", 0.44f);
            setNormalised(processor, "unstable", 0.18f);
            setNormalised(processor, "shatter", 0.24f);
            setNormalised(processor, "smooth", 0.46f);
            setChoice(processor, "mode", 7, 9);
            setChoice(processor, "movementShape", 2, 4);
            setChoice(processor, "syncRate", 2, 5);
            setChoice(processor, "glitchGrid", 2, 4);
            setChoice(processor, "resampleMode", 4, 5);
            break;

        case 6:
            setNormalised(processor, "shakal", 0.62f);
            setNormalised(processor, "destroy", 0.44f);
            setNormalised(processor, "alien", 0.64f);
            setNormalised(processor, "shift", 0.34f);
            setNormalised(processor, "resonance", 0.12f);
            setNormalised(processor, "shatter", 0.20f);
            setNormalised(processor, "smooth", 0.56f);
            setChoice(processor, "mode", 6, 9);
            break;

        case 7:
            setNormalised(processor, "shakal", 0.72f);
            setNormalised(processor, "destroy", 0.54f);
            setNormalised(processor, "crush", 0.38f);
            setNormalised(processor, "decimate", 0.28f);
            setNormalised(processor, "fold", 0.56f);
            setNormalised(processor, "shift", 0.12f);
            setNormalised(processor, "resonance", 0.20f);
            setNormalised(processor, "shatter", 0.28f);
            setNormalised(processor, "smooth", 0.48f);
            setChoice(processor, "mode", 7, 9);
            break;

        case 8:
            setNormalised(processor, "shakal", 0.84f);
            setNormalised(processor, "destroy", 0.70f);
            setNormalised(processor, "crush", 0.54f);
            setNormalised(processor, "decimate", 0.48f);
            setNormalised(processor, "drive", 0.46f);
            setNormalised(processor, "shatter", 0.78f);
            setNormalised(processor, "bandMid", 0.76f);
            setNormalised(processor, "bandHigh", 0.86f);
            setNormalised(processor, "bandAir", 0.60f);
            setNormalised(processor, "fold", 0.22f);
            setNormalised(processor, "smooth", 0.34f);
            setChoice(processor, "mode", 8, 9);
            setChoice(processor, "spectralMode", 1, 6);
            break;

        case 9:
            setNormalised(processor, "shakal", 0.62f);
            setNormalised(processor, "glitch", 0.72f);
            setNormalised(processor, "jitter", 0.24f);
            setNormalised(processor, "movement", 0.38f);
            setNormalised(processor, "smooth", 0.52f);
            setChoice(processor, "glitchMode", 1, 7);
            setChoice(processor, "glitchLength", 2, 6);
            setChoice(processor, "glitchGrid", 2, 4);
            break;

        case 10:
            setNormalised(processor, "shakal", 0.70f);
            setNormalised(processor, "destroy", 0.54f);
            setNormalised(processor, "glitch", 0.64f);
            setNormalised(processor, "shatter", 0.44f);
            setChoice(processor, "glitchMode", 5, 7);
            setChoice(processor, "glitchLength", 3, 6);
            setChoice(processor, "glitchGrid", 1, 4);
            setChoice(processor, "spectralMode", 1, 6);
            break;

        case 11:
            setNormalised(processor, "shakal", 0.58f);
            setNormalised(processor, "destroy", 0.34f);
            setNormalised(processor, "shatter", 0.66f);
            setNormalised(processor, "bandMid", 0.76f);
            setNormalised(processor, "bandHigh", 0.44f);
            setNormalised(processor, "bandAir", 0.20f);
            setNormalised(processor, "smooth", 0.62f);
            setChoice(processor, "spectralMode", 3, 6);
            setChoice(processor, "glitchMode", 0, 7);
            setChoice(processor, "glitchLength", 2, 6);
            break;

        case 12:
            setNormalised(processor, "shakal", 0.86f);
            setNormalised(processor, "destroy", 0.76f);
            setNormalised(processor, "crush", 0.62f);
            setNormalised(processor, "glitch", 0.56f);
            setNormalised(processor, "jitter", 0.44f);
            setNormalised(processor, "unstable", 0.26f);
            setNormalised(processor, "shatter", 0.72f);
            setNormalised(processor, "fold", 0.32f);
            setNormalised(processor, "smooth", 0.30f);
            setChoice(processor, "glitchMode", 6, 7);
            setChoice(processor, "glitchLength", 1, 6);
            setChoice(processor, "spectralMode", 5, 6);
            setChoice(processor, "glitchGrid", 3, 4);
            setChoice(processor, "mode", 7, 9);
            break;

        case 13:
            setNormalised(processor, "shakal", 0.74f);
            setNormalised(processor, "destroy", 0.52f);
            setNormalised(processor, "feedback", 0.72f);
            setNormalised(processor, "feedbackTime", 0.34f);
            setNormalised(processor, "feedbackTone", 0.64f);
            setNormalised(processor, "feedbackDrive", 0.42f);
            setNormalised(processor, "feedbackDiffusion", 0.26f);
            setNormalised(processor, "feedbackSpread", 0.62f);
            setNormalised(processor, "shatter", 0.28f);
            setChoice(processor, "routingTopology", 4, 6);
            setChoice(processor, "characterMode", 9, 10);
            break;

        case 14:
            setNormalised(processor, "shakal", 0.68f);
            setNormalised(processor, "destroy", 0.44f);
            setNormalised(processor, "grainMix", 0.72f);
            setNormalised(processor, "grainSize", 0.22f);
            setNormalised(processor, "grainDensity", 0.64f);
            setNormalised(processor, "grainSpray", 0.34f);
            setNormalised(processor, "grainJitter", 0.28f);
            setNormalised(processor, "grainReverse", 0.38f);
            setNormalised(processor, "pitchDamage", 0.34f);
            setNormalised(processor, "pitchRange", 0.48f);
            setChoice(processor, "pitchMode", 3, 4);
            setNormalised(processor, "glitch", 0.38f);
            setChoice(processor, "glitchMode", 2, 7);
            break;

        case 15:
            setNormalised(processor, "shakal", 0.62f);
            setNormalised(processor, "destroy", 0.48f);
            setNormalised(processor, "reactiveAmount", 0.82f);
            setNormalised(processor, "reactiveTransient", 0.76f);
            setNormalised(processor, "reactiveSpectral", 0.68f);
            setNormalised(processor, "reactiveBass", 0.34f);
            setNormalised(processor, "reactiveHigh", 0.72f);
            setNormalised(processor, "fftMix", 0.56f);
            setNormalised(processor, "fftShatter", 0.48f);
            setNormalised(processor, "fftSpread", 0.42f);
            setNormalised(processor, "fftWarp", 0.18f);
            setNormalised(processor, "glitch", 0.34f);
            setNormalised(processor, "timelineMix", 0.66f);
            setChoice(processor, "routingTopology", 3, 6);
            setBool(processor, "smart", true);
            break;

        case 16:
            setNormalised(processor, "shakal", 0.70f);
            setNormalised(processor, "destroy", 0.58f);
            setNormalised(processor, "crush", 0.34f);
            setNormalised(processor, "decimate", 0.22f);
            setNormalised(processor, "character", 0.64f);
            setChoice(processor, "characterMode", 1, 10);
            setNormalised(processor, "damageMacro", 0.38f);
            setNormalised(processor, "spaceMacro", 0.14f);
            break;

        case 17:
            setNormalised(processor, "shakal", 0.76f);
            setNormalised(processor, "destroy", 0.72f);
            setNormalised(processor, "crush", 0.46f);
            setNormalised(processor, "glitch", 0.12f);
            setNormalised(processor, "damageMacro", 0.72f);
            setChoice(processor, "mode", 3, 9);
            break;

        case 18:
            setNormalised(processor, "shakal", 0.62f);
            setNormalised(processor, "glitch", 0.78f);
            setNormalised(processor, "glitchDensity", 0.76f);
            setNormalised(processor, "glitchProbability", 0.72f);
            setNormalised(processor, "timelineMix", 0.88f);
            setChoice(processor, "glitchPattern", 3, 7);
            setChoice(processor, "glitchMode", 6, 7);
            setNormalised(processor, "motionMacro", 0.68f);
            setNormalised(processor, "chaosMacro", 0.52f);
            break;

        case 19:
            setNormalised(processor, "shakal", 0.74f);
            setNormalised(processor, "destroy", 0.52f);
            setNormalised(processor, "fold", 0.18f);
            setNormalised(processor, "shift", 0.08f);
            setNormalised(processor, "character", 0.72f);
            setChoice(processor, "characterMode", 6, 10);
            setNormalised(processor, "chaosMacro", 0.46f);
            break;

        case 20:
            setNormalised(processor, "shakal", 0.58f);
            setNormalised(processor, "destroy", 0.42f);
            setNormalised(processor, "crush", 0.54f);
            setNormalised(processor, "glitch", 0.26f);
            setNormalised(processor, "character", 0.60f);
            setChoice(processor, "characterMode", 2, 10);
            setNormalised(processor, "motionMacro", 0.34f);
            break;

        case 21:
            setNormalised(processor, "shakal", 0.48f);
            setNormalised(processor, "destroy", 0.32f);
            setNormalised(processor, "shatter", 0.38f);
            setNormalised(processor, "character", 0.78f);
            setChoice(processor, "characterMode", 4, 10);
            setNormalised(processor, "spaceMacro", 0.28f);
            break;

        case 22:
            setNormalised(processor, "shakal", 0.82f);
            setNormalised(processor, "destroy", 0.62f);
            setNormalised(processor, "fftMix", 0.52f);
            setNormalised(processor, "fftShatter", 0.64f);
            setNormalised(processor, "grainMix", 0.28f);
            setNormalised(processor, "spaceMacro", 0.48f);
            setNormalised(processor, "chaosMacro", 0.62f);
            setChoice(processor, "characterMode", 9, 10);
            break;

        case 23:
            setNormalised(processor, "shakal", 0.96f);
            setNormalised(processor, "destroy", 0.88f);
            setNormalised(processor, "crush", 0.72f);
            setNormalised(processor, "decimate", 0.64f);
            setNormalised(processor, "shatter", 0.82f);
            setNormalised(processor, "glitch", 0.58f);
            setNormalised(processor, "damageMacro", 0.86f);
            setNormalised(processor, "motionMacro", 0.62f);
            setNormalised(processor, "chaosMacro", 0.76f);
            setNormalised(processor, "spaceMacro", 0.42f);
            setChoice(processor, "mode", 8, 9);
            setChoice(processor, "characterMode", 9, 10);
            setChoice(processor, "glitchPattern", 5, 7);
            setBool(processor, "smart", true);
            break;

        default:
            break;
    }

    presetBox.setSelectedId(
        index + 2,
        juce::dontSendNotification);
}
