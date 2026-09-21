#include "PluginEditor.h"

namespace
{
const juce::Colour background = juce::Colour::fromRGB(8, 7, 10);
const juce::Colour panel = juce::Colour::fromRGB(17, 15, 20);
const juce::Colour panel2 = juce::Colour::fromRGB(26, 23, 30);
const juce::Colour panel3 = juce::Colour::fromRGB(35, 31, 40);
const juce::Colour accent = juce::Colour::fromRGB(237, 35, 60);
const juce::Colour text = juce::Colour::fromRGB(247, 244, 247);
const juce::Colour muted = juce::Colour::fromRGB(151, 143, 154);
const juce::Colour line = juce::Colour::fromRGB(52, 46, 56);

void setNormalised(ShakalizerAudioProcessor& processor,
                   const char* id,
                   float value)
{
    if (auto* parameter =
            processor.getAPVTS().getParameter(id))
    {
        parameter->setValueNotifyingHost(
            juce::jlimit(0.0f, 1.0f, value));
    }
}

void setChoice(ShakalizerAudioProcessor& processor,
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

void setBool(ShakalizerAudioProcessor& processor,
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
            62, 54, 65));

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
            69, 60, 72));

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
    float p,
    float start,
    float end,
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
        * 0.35f;

    const auto centre =
        bounds.getCentre();

    g.setColour(
        juce::Colour::fromRGB(
            39, 34, 43));

    g.fillEllipse(
        centre.x - radius,
        centre.y - radius,
        radius * 2.0f,
        radius * 2.0f);

    g.setColour(
        juce::Colour::fromRGB(
            79, 69, 83));

    g.drawEllipse(
        centre.x - radius,
        centre.y - radius,
        radius * 2.0f,
        radius * 2.0f,
        2.0f);

    const float angle =
        start + p * (end - start);

    juce::Path arc;

    arc.addCentredArc(
        centre.x,
        centre.y,
        radius + 3.0f,
        radius + 3.0f,
        0.0f,
        start,
        angle,
        true);

    g.setColour(accent);

    g.strokePath(
        arc,
        juce::PathStrokeType(
            3.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    const float dotRadius =
        width > 120 ? 4.0f : 3.0f;

    const float dotX =
        centre.x
        + std::cos(
            angle
            - juce::MathConstants<float>::halfPi)
          * (radius - 4.0f);

    const float dotY =
        centre.y
        + std::sin(
            angle
            - juce::MathConstants<float>::halfPi)
          * (radius - 4.0f);

    g.setColour(text);

    g.fillEllipse(
        dotX - dotRadius,
        dotY - dotRadius,
        dotRadius * 2.0f,
        dotRadius * 2.0f);
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
        down ? panel2.brighter(0.08f)
             : panel3);

    g.fillRoundedRectangle(
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        8.0f);

    g.setColour(
        accent.withAlpha(
            down ? 0.70f : 0.35f));

    g.drawRoundedRectangle(
        0.5f,
        0.5f,
        static_cast<float>(width - 1),
        static_cast<float>(height - 1),
        8.0f,
        1.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool highlighted,
    bool down)
{
    auto bounds =
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
                71, 61, 75));

    g.drawRoundedRectangle(
        bounds,
        7.0f,
        button.getToggleState()
            ? 1.4f : 1.0f);
}

void ShakalizerAudioProcessorEditor::Meter::paint(
    juce::Graphics& g)
{
    auto bounds =
        getLocalBounds()
            .toFloat()
            .reduced(1.0f);

    g.setColour(
        juce::Colour::fromRGB(
            38, 32, 42));

    g.fillRoundedRectangle(
        bounds,
        4.0f);

    const float height =
        bounds.getHeight() * level;

    g.setColour(accent);

    g.fillRoundedRectangle(
        bounds.withTop(
            bounds.getBottom() - height),
        4.0f);
}

ShakalizerAudioProcessorEditor::ShakalizerAudioProcessorEditor(
    ShakalizerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p)
{
    setLookAndFeel(
        &lookAndFeel);

    setResizable(
        true,
        true);

    setResizeLimits(
        1080,
        760,
        1600,
        1050);

    setSize(
        1320,
        900);

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

    addAndMakeVisible(
        titleLabel);

    subtitleLabel.setText(
        "CONTROLLED DIGITAL DESTRUCTION",
        juce::dontSendNotification);

    subtitleLabel.setFont(
        juce::Font(
            juce::FontOptions(10.0f)));

    subtitleLabel.setColour(
        juce::Label::textColourId,
        muted);

    addAndMakeVisible(
        subtitleLabel);

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
            "HARD SHATTER"
        },
        1);

    presetBox.setSelectedId(
        1,
        juce::dontSendNotification);

    modeBox.addItemList(
        {
            "Clean", "Crunch", "Shakal",
            "Destroy", "Fried", "Pixel",
            "Alien", "Melt", "Shatter"
        },
        1);

    resampleBox.addItemList(
        {
            "Hold", "Linear", "Stair",
            "Smear", "Random"
        },
        1);

    filterBox.addItemList(
        {
            "Low Pass", "Band Pass", "High Pass"
        },
        1);

    movementBox.addItemList(
        {
            "Sine", "Triangle",
            "Sample+Hold", "Stepped"
        },
        1);

    qualityBox.addItemList(
        { "1x", "2x", "4x" },
        1);

    syncBox.addItemList(
        {
            "Free", "1/4", "1/8",
            "1/16", "1/32"
        },
        1);

    glitchGridBox.addItemList(
        {
            "Free", "1/8",
            "1/16", "1/32"
        },
        1);

    const std::array<
        std::pair<juce::ComboBox*, const char*>, 7> combos {
        std::pair { &modeBox, "mode" },
        std::pair { &resampleBox, "resampleMode" },
        std::pair { &filterBox, "filterType" },
        std::pair { &movementBox, "movementShape" },
        std::pair { &qualityBox, "quality" },
        std::pair { &syncBox, "syncRate" },
        std::pair { &glitchGridBox, "glitchGrid" }
    };

    for (auto [box, id] : combos)
    {
        juce::ignoreUnused(id);
        addAndMakeVisible(*box);
    }

    addAndMakeVisible(
        presetBox);

    presetBox.onChange = [this]
    {
        const int index =
            presetBox.getSelectedItemIndex();

        if (index >= 0)
            loadPreset(index);
    };

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

    randomAllButton.onClick = [this]
    {
        randomizeAll();
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
        &randomAllButton,
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

    const std::array<const char*, sliderCount> ids {
        "shakal", "destroy", "crush", "decimate",
        "drive", "clip", "glitch", "jitter",
        "split", "transient", "body", "stereo",
        "movement", "unstable", "alien",
        "filterFreq", "filterRes", "mix", "output",
        "shatter", "fold", "shift", "resonance",
        "envFollow", "bandLow", "bandMid",
        "bandHigh", "bandAir", "character",
        "preGain", "smooth"
    };

    for (int i = 0; i < sliderCount; ++i)
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
    setLookAndFeel(
        nullptr);
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
        72,
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
        slider.setSkewFactorFromMidPoint(
            1500.0);
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
    g.fillAll(
        background);

    auto outer =
        getLocalBounds()
            .reduced(12);

    g.setColour(panel);
    g.fillRoundedRectangle(
        outer.toFloat(),
        16.0f);

    g.setColour(
        accent.withAlpha(0.92f));

    g.fillRoundedRectangle(
        12.0f,
        12.0f,
        7.0f,
        104.0f,
        3.0f);

    // Main engine area.
    g.setColour(panel2);

    g.fillRoundedRectangle(
        28.0f,
        128.0f,
        static_cast<float>(
            getWidth() - 56),
        static_cast<float>(
            getHeight() - 148),
        14.0f);

    // Large macro well.
    g.setColour(panel3);

    g.fillRoundedRectangle(
        40.0f,
        150.0f,
        235.0f,
        static_cast<float>(
            getHeight() - 192),
        12.0f);

    g.setColour(
        accent.withAlpha(0.08f));

    g.fillEllipse(
        54.0f,
        170.0f,
        205.0f,
        205.0f);

    const int rightX = 292;

    g.setColour(line);

    for (int y : {
        274, 394, 514, 634, 754
    })
    {
        if (y < getHeight() - 25)
        {
            g.drawHorizontalLine(
                y,
                static_cast<float>(rightX),
                static_cast<float>(
                    getWidth() - 42));
        }
    }

    const std::array<const char*, 5> sectionNames {
        "CORE / DAMAGE",
        "MOTION / DYNAMICS",
        "FILTER / OUTPUT",
        "SPECTRAL SHATTER",
        "CHARACTER / SAFETY"
    };

    g.setFont(
        juce::Font(
            juce::FontOptions(
                9.0f,
                juce::Font::bold)));

    for (int i = 0; i < 5; ++i)
    {
        const int y =
            140 + i * 120;

        g.setColour(
            muted);

        g.drawText(
            sectionNames[
                static_cast<size_t>(i)],
            rightX,
            y,
            180,
            16,
            juce::Justification::left,
            false);
    }

    g.setColour(
        accent.withAlpha(0.18f));

    g.fillRoundedRectangle(
        48.0f,
        152.0f,
        5.0f,
        66.0f,
        2.0f);
}

void ShakalizerAudioProcessorEditor::resized()
{
    const int w =
        getWidth();

    titleLabel.setBounds(
        38,
        22,
        270,
        38);

    subtitleLabel.setBounds(
        39,
        61,
        300,
        18);

    presetBox.setBounds(
        320,
        22,
        160,
        32);

    modeBox.setBounds(
        488,
        22,
        104,
        32);

    resampleBox.setBounds(
        600,
        22,
        92,
        32);

    filterBox.setBounds(
        700,
        22,
        94,
        32);

    movementBox.setBounds(
        802,
        22,
        108,
        32);

    qualityBox.setBounds(
        918,
        22,
        65,
        32);

    syncBox.setBounds(
        991,
        22,
        72,
        32);

    glitchGridBox.setBounds(
        1071,
        22,
        78,
        32);

    saveAButton.setBounds(
        488,
        64,
        70,
        27);

    abButton.setBounds(
        564,
        64,
        55,
        27);

    autoMatchButton.setBounds(
        625,
        64,
        55,
        27);

    randomCoreButton.setBounds(
        687,
        64,
        67,
        27);

    randomShatterButton.setBounds(
        761,
        64,
        82,
        27);

    randomGlitchButton.setBounds(
        850,
        64,
        67,
        27);

    randomAllButton.setBounds(
        924,
        64,
        120,
        27);

    meterLabel.setBounds(
        1052,
        67,
        30,
        12);

    meter.setBounds(
        1087,
        61,
        8,
        30);

    shakalSlider.setBounds(
        55,
        235,
        205,
        245);

    const int rightX = 286;
    const int contentWidth =
        w - rightX - 46;

    const int columns = 6;
    const int rows = 5;
    const int gap = 4;

    const int cellW =
        (contentWidth
         - gap * (columns - 1))
        / columns;

    const int cellH =
        (getHeight()
         - 170
         - gap * (rows - 1)
         - 35)
        / rows;

    // Slider 0 is the macro and gets its own large well.
    for (int i = 1;
         i < sliderCount;
         ++i)
    {
        const int local =
            i - 1;

        const int row =
            local / columns;

        const int col =
            local % columns;

        sliders[
            static_cast<size_t>(i)]
            ->setBounds(
                rightX
                + col * (cellW + gap),
                158
                + row * (cellH + gap),
                cellW,
                cellH);
    }
}

void ShakalizerAudioProcessorEditor::timerCallback()
{
    meter.setLevel(
        processor.getMeterLevel());

    updateAutoMatchButton();
}

void ShakalizerAudioProcessorEditor::updateAutoMatchButton()
{
    const bool enabled =
        processor.getAPVTS()
            .getRawParameterValue(
                "autoMatch")
            ->load() > 0.5f;

    autoMatchButton.setToggleState(
        enabled,
        juce::dontSendNotification);
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
                        static_cast<size_t>(i)]));
        }
    }
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

    abState =
        std::move(current);

    presetBox.setSelectedId(
        0,
        juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::toggleAutoMatch()
{
    auto* parameter =
        processor.getAPVTS()
            .getParameter(
                "autoMatch");

    if (parameter == nullptr)
        return;

    const bool current =
        parameter->getValue() > 0.5f;

    parameter->setValueNotifyingHost(
        current ? 0.0f : 1.0f);

    updateAutoMatchButton();
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
        0,
        juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::randomizeScope(
    int scope)
{
    static juce::Random random;

    const std::array<const char*, 8> coreIds {
        "shakal", "destroy", "crush", "decimate",
        "drive", "clip", "preGain", "smooth"
    };

    const std::array<const char*, 11> shatterIds {
        "shatter", "fold", "shift", "resonance",
        "envFollow", "bandLow", "bandMid",
        "bandHigh", "bandAir", "character", "smooth"
    };

    const std::array<const char*, 10> glitchIds {
        "glitch", "jitter", "movement", "unstable",
        "stereo", "alien", "movementShape",
        "syncRate", "glitchGrid", "resampleMode"
    };

    if (scope == 1)
    {
        for (const auto* id : coreIds)
            if (auto* parameter =
                    processor.getAPVTS().getParameter(id))
                parameter->setValueNotifyingHost(
                    random.nextFloat());
    }
    else if (scope == 2)
    {
        for (const auto* id : shatterIds)
            if (auto* parameter =
                    processor.getAPVTS().getParameter(id))
                parameter->setValueNotifyingHost(
                    random.nextFloat());
    }
    else if (scope == 3)
    {
        for (const auto* id : glitchIds)
            if (auto* parameter =
                    processor.getAPVTS().getParameter(id))
                parameter->setValueNotifyingHost(
                    random.nextFloat());
    }

    presetBox.setSelectedId(
        0,
        juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::loadPreset(
    int index)
{
    // Built-in presets intentionally set a complete, safe starting state.
    // Individual controls can then be tweaked and the state is saved with the host.
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

        for (const auto& [id, value] : values)
            setNormalised(
                processor,
                id,
                value);

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

        setBool(
            processor,
            "autoMatch",
            false);
    };

    base();

    switch (index)
    {
        case 1: // Vocal Digital
            setNormalised(processor, "shakal", 0.58f);
            setNormalised(processor, "destroy", 0.46f);
            setNormalised(processor, "crush", 0.30f);
            setNormalised(processor, "decimate", 0.22f);
            setNormalised(processor, "drive", 0.34f);
            setNormalised(processor, "glitch", 0.04f);
            setNormalised(processor, "shatter", 0.18f);
            setNormalised(processor, "bandMid", 0.34f);
            setNormalised(processor, "bandHigh", 0.48f);
            setNormalised(processor, "bandAir", 0.22f);
            setNormalised(processor, "smooth", 0.52f);
            setChoice(processor, "mode", 2, 9);
            break;

        case 2: // Shakal Lead
            setNormalised(processor, "shakal", 0.76f);
            setNormalised(processor, "destroy", 0.58f);
            setNormalised(processor, "crush", 0.40f);
            setNormalised(processor, "decimate", 0.34f);
            setNormalised(processor, "drive", 0.44f);
            setNormalised(processor, "clip", 0.24f);
            setNormalised(processor, "shatter", 0.34f);
            setNormalised(processor, "bandMid", 0.56f);
            setNormalised(processor, "bandHigh", 0.72f);
            setNormalised(processor, "fold", 0.14f);
            setNormalised(processor, "smooth", 0.40f);
            setChoice(processor, "mode", 2, 9);
            break;

        case 3: // Broken 808
            setNormalised(processor, "shakal", 0.68f);
            setNormalised(processor, "destroy", 0.50f);
            setNormalised(processor, "crush", 0.38f);
            setNormalised(processor, "decimate", 0.38f);
            setNormalised(processor, "drive", 0.36f);
            setNormalised(processor, "split", 0.78f);
            setNormalised(processor, "bandLow", 0.03f);
            setNormalised(processor, "bandMid", 0.38f);
            setNormalised(processor, "bandHigh", 0.34f);
            setNormalised(processor, "bandAir", 0.08f);
            setNormalised(processor, "resonance", 0.08f);
            setNormalised(processor, "smooth", 0.58f);
            setChoice(processor, "mode", 3, 9);
            break;

        case 4: // Pixel Drum
            setNormalised(processor, "shakal", 0.70f);
            setNormalised(processor, "destroy", 0.62f);
            setNormalised(processor, "crush", 0.48f);
            setNormalised(processor, "decimate", 0.46f);
            setNormalised(processor, "glitch", 0.20f);
            setNormalised(processor, "jitter", 0.18f);
            setNormalised(processor, "shatter", 0.34f);
            setNormalised(processor, "bandHigh", 0.72f);
            setNormalised(processor, "bandAir", 0.48f);
            setNormalised(processor, "smooth", 0.35f);
            setChoice(processor, "mode", 5, 9);
            setChoice(processor, "resampleMode", 2, 5);
            break;

        case 5: // Glitch Grid
            setNormalised(processor, "shakal", 0.62f);
            setNormalised(processor, "destroy", 0.44f);
            setNormalised(processor, "glitch", 0.46f);
            setNormalised(processor, "jitter", 0.32f);
            setNormalised(processor, "movement", 0.46f);
            setNormalised(processor, "unstable", 0.20f);
            setNormalised(processor, "shatter", 0.25f);
            setNormalised(processor, "smooth", 0.42f);
            setChoice(processor, "mode", 7, 9);
            setChoice(processor, "resampleMode", 4, 5);
            setChoice(processor, "movementShape", 2, 4);
            setChoice(processor, "syncRate", 2, 5);
            setChoice(processor, "glitchGrid", 2, 4);
            break;

        case 6: // Alien
            setNormalised(processor, "shakal", 0.62f);
            setNormalised(processor, "destroy", 0.46f);
            setNormalised(processor, "drive", 0.30f);
            setNormalised(processor, "alien", 0.64f);
            setNormalised(processor, "shift", 0.34f);
            setNormalised(processor, "resonance", 0.16f);
            setNormalised(processor, "shatter", 0.20f);
            setNormalised(processor, "smooth", 0.50f);
            setChoice(processor, "mode", 6, 9);
            break;

        case 7: // Melt
            setNormalised(processor, "shakal", 0.72f);
            setNormalised(processor, "destroy", 0.56f);
            setNormalised(processor, "crush", 0.40f);
            setNormalised(processor, "decimate", 0.30f);
            setNormalised(processor, "fold", 0.62f);
            setNormalised(processor, "shift", 0.12f);
            setNormalised(processor, "resonance", 0.24f);
            setNormalised(processor, "shatter", 0.30f);
            setNormalised(processor, "smooth", 0.44f);
            setChoice(processor, "mode", 7, 9);
            break;

        case 8: // Hard Shatter
            setNormalised(processor, "shakal", 0.86f);
            setNormalised(processor, "destroy", 0.72f);
            setNormalised(processor, "crush", 0.56f);
            setNormalised(processor, "decimate", 0.50f);
            setNormalised(processor, "drive", 0.48f);
            setNormalised(processor, "clip", 0.30f);
            setNormalised(processor, "shatter", 0.82f);
            setNormalised(processor, "bandMid", 0.78f);
            setNormalised(processor, "bandHigh", 0.88f);
            setNormalised(processor, "bandAir", 0.66f);
            setNormalised(processor, "fold", 0.24f);
            setNormalised(processor, "smooth", 0.30f);
            setChoice(processor, "mode", 8, 9);
            break;

        default:
            // INIT / SAFE uses the processor defaults.
            break;
    }

    presetBox.setSelectedId(
        index + 1,
        juce::dontSendNotification);
}
