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

ShakalizerAudioProcessorEditor::ShakalizerAudioProcessorEditor(
    ShakalizerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      modSourceBoxes {
          &modSource1Box,
          &modSource2Box,
          &modSource3Box,
          &modSource4Box },
      modDestBoxes {
          &modDest1Box,
          &modDest2Box,
          &modDest3Box,
          &modDest4Box }
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(1180, 780, 1900, 1250);
    setSize(1680, 1040);

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
            "CHAOS LAB"
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
    glitchLengthBox.addItemList(
        { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2" }, 1);
    spectralModeBox.addItemList(
        { "Smooth", "Shatter", "Blur", "Freeze", "Bits", "Ring" }, 1);
    routingBox.addItemList(
        { "Standard", "Damage > Shatter",
          "Shatter > Damage", "Parallel" }, 1);
    msModeBox.addItemList(
        { "Stereo", "Mid", "Side", "Split" }, 1);
    liveSceneBox.addItemList(
        { "Normal", "Impact", "Glitch",
          "Melt", "Broken", "Chaos" }, 1);

    for (auto* box : {
        &presetBox, &modeBox, &resampleBox, &filterBox,
        &movementBox, &qualityBox, &syncBox,
        &glitchGridBox, &glitchModeBox, &glitchLengthBox,
        &spectralModeBox, &routingBox, &msModeBox,
        &liveSceneBox })
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

    glitchLengthAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "glitchLength", glitchLengthBox);

    spectralModeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.getAPVTS(), "spectralMode", spectralModeBox);

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

    for (int i = 0; i < 4; ++i)
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

    for (auto* button : {
        &saveAButton,
        &abButton,
        &autoMatchButton,
        &smartButton,
        &randomAllButton,
        &savePresetButton,
        &loadPresetButton,
        &randomCoreButton,
        &randomShatterButton,
        &randomGlitchButton
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

    addAndMakeVisible(
        meter);

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
        "mod4Amount", "morph"
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

    const auto outer =
        getLocalBounds()
            .reduced(10);

    g.setColour(panel);

    g.fillRoundedRectangle(
        outer.toFloat(),
        15.0f);

    g.setColour(accent);

    g.fillRoundedRectangle(
        10.0f,
        10.0f,
        7.0f,
        92.0f,
        3.0f);

    const int leftWidth =
        286;

    g.setColour(panel2);

    g.fillRoundedRectangle(
        28.0f,
        116.0f,
        static_cast<float>(
            leftWidth - 42),
        static_cast<float>(
            getHeight() - 136),
        13.0f);

    g.setColour(panel2);

    g.fillRoundedRectangle(
        static_cast<float>(
            leftWidth),
        116.0f,
        static_cast<float>(
            getWidth() - leftWidth - 26),
        static_cast<float>(
            getHeight() - 136),
        13.0f);

    g.setColour(
        accent.withAlpha(0.08f));

    g.fillEllipse(
        47.0f,
        144.0f,
        205.0f,
        205.0f);

    if (getHeight() > 850)
    {
        const auto scope =
            juce::Rectangle<float>(
                43.0f,
                static_cast<float>(
                    getHeight() - 178),
                218.0f,
                142.0f);

        g.setColour(
            juce::Colour::fromRGB(
                11, 10, 14));
        g.fillRoundedRectangle(
            scope,
            9.0f);

        g.setColour(
            accent.withAlpha(0.22f));
        g.drawRoundedRectangle(
            scope.reduced(0.5f),
            9.0f,
            1.0f);

        juce::Path wave;
        const int count = 256;
        const float centreY =
            scope.getCentreY();
        const float halfH =
            scope.getHeight() * 0.38f;

        for (int i = 0; i < count; ++i)
        {
            const int write =
                processor.getScopeWriteIndex();

            const float value =
                processor.getScopeSample(
                    (write + i) & 255);
            const float x =
                scope.getX()
                + static_cast<float>(i)
                  / static_cast<float>(count - 1)
                  * scope.getWidth();
            const float y =
                centreY - value * halfH;

            if (i == 0)
                wave.startNewSubPath(x, y);
            else
                wave.lineTo(x, y);
        }

        g.setColour(
            accent.withAlpha(0.82f));
        g.strokePath(
            wave,
            juce::PathStrokeType(
                1.15f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));

        g.setColour(muted);
        g.setFont(
            juce::Font(
                juce::FontOptions(
                    8.0f,
                    juce::Font::bold)));
        g.drawText(
            "LIVE SCOPE",
            static_cast<int>(scope.getX() + 8.0f),
            static_cast<int>(scope.getY() + 6.0f),
            static_cast<int>(scope.getWidth() - 16.0f),
            14,
            juce::Justification::left,
            false);
    }

    g.setColour(line);

    for (int y : {
        252, 384, 516,
        648, 780
    })
    {
        if (y < getHeight() - 20)
        {
            g.drawHorizontalLine(
                y,
                static_cast<float>(
                    leftWidth + 18),
                static_cast<float>(
                    getWidth() - 35));
        }
    }

    g.setFont(
        juce::Font(
            juce::FontOptions(
                9.0f,
                juce::Font::bold)));

    const std::array<
        const char*, 6> sections {
        "CORE",
        "DYNAMICS",
        "FILTER",
        "SHATTER",
        "CHARACTER",
        "MOD / MORPH"
    };

    for (size_t i = 0;
         i < sections.size();
         ++i)
    {
        g.setColour(muted);

        g.drawText(
            sections[i],
            leftWidth + 18,
            122
            + static_cast<int>(i) * 132,
            120,
            16,
            juce::Justification::left,
            false);
    }

    g.setColour(
        accent.withAlpha(0.14f));

    g.fillRoundedRectangle(
        43.0f,
        140.0f,
        5.0f,
        58.0f,
        2.0f);
}

void ShakalizerAudioProcessorEditor::resized()
{
    const int w =
        getWidth();

    titleLabel.setBounds(
        36,
        18,
        260,
        38);

    subtitleLabel.setBounds(
        37,
        56,
        290,
        17);

    presetBox.setBounds(
        312,
        18,
        160,
        31);

    modeBox.setBounds(
        480,
        18,
        105,
        31);

    resampleBox.setBounds(
        593,
        18,
        93,
        31);

    filterBox.setBounds(
        694,
        18,
        94,
        31);

    movementBox.setBounds(
        796,
        18,
        110,
        31);

    qualityBox.setBounds(
        914,
        18,
        66,
        31);

    syncBox.setBounds(
        988,
        18,
        72,
        31);

    glitchGridBox.setBounds(
        1068,
        18,
        78,
        31);

    routingBox.setBounds(
        1154,
        18,
        128,
        31);

    msModeBox.setBounds(
        1290,
        18,
        78,
        31);

    liveSceneBox.setBounds(
        1376,
        18,
        95,
        31);

    glitchModeBox.setBounds(
        920,
        56,
        102,
        26);

    glitchLengthBox.setBounds(
        1028,
        56,
        78,
        26);

    spectralModeBox.setBounds(
        1112,
        56,
        108,
        26);

    saveAButton.setBounds(
        312,
        56,
        67,
        26);

    abButton.setBounds(
        385,
        56,
        53,
        26);

    autoMatchButton.setBounds(
        444,
        56,
        55,
        26);

    smartButton.setBounds(
        505,
        56,
        61,
        26);

    randomCoreButton.setBounds(
        582,
        56,
        62,
        26);

    randomShatterButton.setBounds(
        650,
        56,
        78,
        26);

    randomGlitchButton.setBounds(
        734,
        56,
        70,
        26);

    randomAllButton.setBounds(
        810,
        56,
        103,
        26);

    savePresetButton.setBounds(
        926,
        87,
        58,
        23);

    loadPresetButton.setBounds(
        990,
        87,
        58,
        23);

    meterLabel.setBounds(
        1431,
        58,
        30,
        12);

    meter.setBounds(
        1466,
        53,
        7,
        30);

    shakalSlider.setBounds(
        48,
        178,
        215,
        165);

    const int left = 315;
    const int top = 140;
    const int availableWidth =
        w - left - 35;

    const int cols = 7;
    const int rows = 5;
    const int gap = 4;

    const int cellW =
        (availableWidth
         - gap * (cols - 1))
        / cols;

    const int bottom =
        getHeight()
        - 22;

    const int cellH =
        (bottom
         - top
         - gap * (rows - 1))
        / rows;

    // First five rows of the right-hand controls are the 35 parameters
    // after the large SHAKAL macro.
    for (int i = 1;
         i < sliderCount;
         ++i)
    {
        const int local =
            i - 1;

        const int row =
            local / cols;

        const int col =
            local % cols;

        sliders[
            static_cast<size_t>(i)]
            ->setBounds(
                left
                + col * (cellW + gap),
                top
                + row * (cellH + gap),
                cellW,
                cellH);
    }

    // The modulation controls are placed over the left-lower panel.
    const int modY = 408;

    for (int i = 0;
         i < 4;
         ++i)
    {
        const int y =
            modY + i * 91;

        modSourceBoxes[
            static_cast<size_t>(i)]
            ->setBounds(
                42,
                y,
                92,
                27);

        modDestBoxes[
            static_cast<size_t>(i)]
            ->setBounds(
                140,
                y,
                105,
                27);

        sliders[
            static_cast<size_t>(
                31 + i)]
            ->setBounds(
                45,
                y + 31,
                198,
                52);
    }

    meter.setBounds(
        w - 26,
        56,
        7,
        30);
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

    autoMatchButton.setToggleState(
        autoOn,
        juce::dontSendNotification);

    smartButton.setToggleState(
        smartOn,
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
        this);
}

void ShakalizerAudioProcessorEditor::saveA()
{
    captureState(
        abState);

    hasAState =
        !abState.empty();
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
    static juce::Random random;

    for (auto* parameter :
         processor.getParameters())
    {
        if (parameter == nullptr)
            continue;

        if (dynamic_cast<
                juce::AudioParameterBool*>(
                parameter) != nullptr)
        {
            parameter->setValueNotifyingHost(
                random.nextFloat()
                    > 0.72f
                    ? 1.0f
                    : 0.0f);
        }
        else
        {
            parameter->setValueNotifyingHost(
                random.nextFloat());
        }
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
        const char*, 13> spectral {
        "shatter", "fold", "shift",
        "resonance", "envFollow",
        "bandLow", "bandMid", "bandHigh",
        "bandAir", "character", "morph",
        "filterFreq", "spectralMode"
    };

    const std::array<
        const char*, 14> glitch {
        "glitch", "jitter", "movement",
        "unstable", "stereo", "alien",
        "glitchMode", "glitchLength",
        "mod1Amount", "mod2Amount",
        "mod3Amount", "mod4Amount",
        "morph", "mix"
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

    if (scope == 1)
        randomizeIds(core);
    else if (scope == 2)
        randomizeIds(spectral);
    else if (scope == 3)
        randomizeIds(glitch);

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

        for (int i = 1; i <= 4; ++i)
        {
            setNormalised(
                processor,
                ("mod"
                 + juce::String(i)
                 + "Amount").toRawUTF8(),
                0.5f);
        }

        setNormalised(
            processor,
            "morph",
            0.0f);

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

        for (int i = 1; i <= 4; ++i)
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

        default:
            break;
    }

    presetBox.setSelectedId(
        index + 2,
        juce::dontSendNotification);
}
