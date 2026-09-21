#include "PluginEditor.h"

namespace
{
const juce::Colour background = juce::Colour::fromRGB(10, 9, 12);
const juce::Colour panel = juce::Colour::fromRGB(19, 17, 22);
const juce::Colour panel2 = juce::Colour::fromRGB(28, 25, 32);
const juce::Colour accent = juce::Colour::fromRGB(237, 35, 60);
const juce::Colour text = juce::Colour::fromRGB(245, 242, 245);
const juce::Colour muted = juce::Colour::fromRGB(151, 143, 154);
}

ShakalizerAudioProcessorEditor::ShakalLookAndFeel::ShakalLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, accent);
    setColour(juce::Slider::rotarySliderFillColourId, accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(58, 51, 62));
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, panel2);
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(65, 57, 69));
    setColour(juce::ComboBox::textColourId, text);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float p, float start, float end, juce::Slider&)
{
    auto b = juce::Rectangle<float>(
        (float)x, (float)y, (float)width, (float)height).reduced(7.0f);

    const float r = juce::jmin(
        b.getWidth(), b.getHeight()) * 0.37f;

    const auto c = b.getCentre();

    g.setColour(juce::Colour::fromRGB(42, 37, 46));
    g.fillEllipse(c.x-r, c.y-r, r*2.0f, r*2.0f);

    g.setColour(juce::Colour::fromRGB(76, 67, 81));
    g.drawEllipse(c.x-r, c.y-r, r*2.0f, r*2.0f, 2.0f);

    const float angle = start + p * (end-start);

    juce::Path arc;
    arc.addCentredArc(
        c.x, c.y, r+3.0f, r+3.0f,
        0.0f, start, angle, true);

    g.setColour(accent);
    g.strokePath(
        arc,
        juce::PathStrokeType(
            3.5f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    const float dotR = 3.5f;
    const float dx =
        c.x + std::cos(
            angle - juce::MathConstants<float>::halfPi)
        * (r-4.0f);

    const float dy =
        c.y + std::sin(
            angle - juce::MathConstants<float>::halfPi)
        * (r-4.0f);

    g.setColour(text);
    g.fillEllipse(
        dx-dotR, dy-dotR,
        dotR*2.0f, dotR*2.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool down,
    int, int, int, int, juce::ComboBox&)
{
    g.setColour(down ? panel : panel2);
    g.fillRoundedRectangle(
        0.0f, 0.0f,
        (float)width, (float)height, 8.0f);

    g.setColour(accent.withAlpha(0.45f));
    g.drawRoundedRectangle(
        0.5f, 0.5f,
        (float)width-1.0f,
        (float)height-1.0f,
        8.0f, 1.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button,
    const juce::Colour&, bool highlighted, bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced(0.5f);

    g.setColour(
        (down || highlighted)
            ? panel2.brighter(0.08f)
            : panel2);

    g.fillRoundedRectangle(b, 8.0f);

    g.setColour(
        button.getToggleState()
            ? accent.withAlpha(0.85f)
            : juce::Colour::fromRGB(68, 59, 72));

    g.drawRoundedRectangle(b, 8.0f, 1.0f);
}

void ShakalizerAudioProcessorEditor::Meter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(juce::Colour::fromRGB(37, 32, 41));
    g.fillRoundedRectangle(b, 5.0f);

    const float h = b.getHeight() * level;

    g.setColour(accent);
    g.fillRoundedRectangle(
        b.withTop(b.getBottom() - h), 5.0f);
}

ShakalizerAudioProcessorEditor::ShakalizerAudioProcessorEditor(
    ShakalizerAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(900, 680, 1500, 980);
    setSize(1240, 820);

    titleLabel.setText(
        "SHAKALIZER",
        juce::dontSendNotification);

    titleLabel.setFont(
        juce::Font(
            juce::FontOptions(
                32.0f,
                juce::Font::bold)));

    titleLabel.setColour(
        juce::Label::textColourId,
        text);

    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "DIGITAL DESTRUCTION / SPECTRAL SHATTER / MOVEMENT",
        juce::dontSendNotification);

    subtitleLabel.setFont(
        juce::Font(
            juce::FontOptions(11.0f)));

    subtitleLabel.setColour(
        juce::Label::textColourId,
        muted);

    addAndMakeVisible(subtitleLabel);

    modeBox.addItemList(
        { "Clean", "Crunch", "Shakal", "Destroy", "Fried",
          "Pixel", "Alien", "Melt", "Shatter" }, 1);

    resampleBox.addItemList(
        { "Hold", "Linear", "Stair", "Smear", "Random" }, 1);

    filterBox.addItemList(
        { "Low Pass", "Band Pass", "High Pass" }, 1);

    movementBox.addItemList(
        { "Sine", "Triangle", "Sample+Hold", "Stepped" }, 1);

    qualityBox.addItemList(
        { "1x", "2x", "4x" }, 3);

    syncBox.addItemList(
        { "Free", "1/4", "1/8", "1/16", "1/32" }, 1);

    glitchGridBox.addItemList(
        { "Free", "1/8", "1/16", "1/32" }, 1);

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

    for (auto* box : {
        &modeBox, &resampleBox, &filterBox, &movementBox,
        &qualityBox, &syncBox, &glitchGridBox })
    {
        addAndMakeVisible(*box);
    }

    saveAButton.onClick = [this] { saveA(); };
    abButton.onClick = [this] { swapAB(); };
    autoMatchButton.onClick = [this] { toggleAutoMatch(); };
    randomButton.onClick = [this] { randomize(); };

    for (auto* button : {
        &saveAButton, &abButton,
        &autoMatchButton, &randomButton })
    {
        button->setColour(
            juce::TextButton::textColourOnId, text);

        button->setColour(
            juce::TextButton::textColourOffId, text);

        addAndMakeVisible(*button);
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

    addAndMakeVisible(meterLabel);
    addAndMakeVisible(meter);

    const std::array<double, sliderCount> mins {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        80, 0.05, 0, -12,
        0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, -24
    };

    const std::array<double, sliderCount> maxs {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
        18000, 0.95, 1, 6,
        1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 12
    };

    const std::array<double, sliderCount> steps {
        // 1-6: core
        0.001, 0.001, 0.001, 0.001, 0.001, 0.001,
        // 7-12: glitch / dynamics
        0.001, 0.001, 0.001, 0.001, 0.001, 0.001,
        // 13-19: movement / filter / mix / output
        0.001, 0.001, 0.001, 1.0, 0.001, 0.001, 0.01,
        // 20-29: shatter engine
        0.001, 0.001, 0.001, 0.001, 0.001,
        0.001, 0.001, 0.001, 0.001, 0.001,
        // 30: pre gain
        0.01
    };

    for (int i = 0; i < sliderCount; ++i)
    {
        configureSlider(
            *sliders[(size_t)i],
            sliderNames[(size_t)i],
            mins[(size_t)i],
            maxs[(size_t)i],
            steps[(size_t)i]);

        addAndMakeVisible(
            *sliders[(size_t)i]);
    }

    const std::array<const char*, sliderCount> ids {
        "shakal", "destroy", "crush", "decimate",
        "drive", "clip", "glitch", "jitter",
        "split", "transient", "body", "stereo",
        "movement", "unstable", "alien",
        "filterFreq", "filterRes", "mix", "output",
        "shatter", "fold", "shift", "resonance",
        "envFollow", "bandLow", "bandMid", "bandHigh",
        "bandAir", "character", "preGain"
    };

    for (int i = 0; i < sliderCount; ++i)
        addAttachment(
            ids[(size_t)i],
            *sliders[(size_t)i]);

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
        false, 74, 18);

    slider.setRange(
        min, max, step);

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        text);

    slider.setTooltip(name);

    if (name == "FILTER FREQ")
        slider.setSkewFactorFromMidPoint(1600.0);
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

    auto outer =
        getLocalBounds().reduced(14);

    g.setColour(panel);
    g.fillRoundedRectangle(
        outer.toFloat(), 16.0f);

    g.setColour(
        accent.withAlpha(0.88f));

    g.fillRoundedRectangle(
        14.0f, 14.0f,
        7.0f, 100.0f,
        3.0f);

    g.setColour(panel2);
    g.fillRoundedRectangle(
        26.0f, 122.0f,
        (float)getWidth() - 52.0f,
        (float)getHeight() - 138.0f,
        14.0f);

    g.setColour(
        juce::Colour::fromRGB(49, 43, 52));

    for (int y : { 244, 356, 468, 580, 692 })
        if (y < getHeight() - 20)
            g.drawHorizontalLine(
                y, 44.0f,
                (float)getWidth() - 44.0f);

    g.setColour(
        accent.withAlpha(0.07f));

    g.fillEllipse(
        40.0f, 144.0f,
        150.0f, 150.0f);
}

void ShakalizerAudioProcessorEditor::resized()
{
    const int w = getWidth();

    titleLabel.setBounds(
        40, 22, 310, 40);

    subtitleLabel.setBounds(
        41, 64, 430, 22);

    const int comboY = 28;

    modeBox.setBounds(
        w - 760, comboY, 118, 32);

    resampleBox.setBounds(
        w - 635, comboY, 100, 32);

    filterBox.setBounds(
        w - 528, comboY, 100, 32);

    movementBox.setBounds(
        w - 421, comboY, 122, 32);

    qualityBox.setBounds(
        w - 290, comboY, 68, 32);

    syncBox.setBounds(
        w - 216, comboY, 76, 32);

    glitchGridBox.setBounds(
        w - 134, comboY, 76, 32);

    saveAButton.setBounds(
        w - 350, 70, 68, 28);

    abButton.setBounds(
        w - 276, 70, 54, 28);

    autoMatchButton.setBounds(
        w - 216, 70, 104, 28);

    randomButton.setBounds(
        w - 108, 70, 92, 28);

    meterLabel.setBounds(
        w - 70, 104, 30, 12);

    meter.setBounds(
        w - 32, 100, 8, 28);

    const int contentX = 40;
    const int contentY = 132;
    const int contentW = w - 80;
    const int columns = 6;
    const int rows = 5;
    const int gap = 5;

    const int cellW =
        (contentW - gap * (columns - 1))
        / columns;

    const int cellH =
        (getHeight() - contentY - 26
         - gap * (rows - 1))
        / rows;

    for (int i = 0; i < sliderCount; ++i)
    {
        const int row = i / columns;
        const int col = i % columns;

        sliders[(size_t)i]->setBounds(
            contentX + col * (cellW + gap),
            contentY + row * (cellH + gap),
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
    const bool on =
        processor.getAPVTS()
            .getRawParameterValue(
                "autoMatch")->load() > 0.5f;

    autoMatchButton.setToggleState(
        on,
        juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::captureState(
    std::vector<float>& destination)
{
    destination.clear();
    destination.reserve(
        (size_t)processor.getParameters().size());

    for (auto* parameter : processor.getParameters())
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
            (int)state.size(),
            processor.getParameters().size());

    for (int i = 0; i < count; ++i)
    {
        if (auto* parameter =
                processor.getParameters()[i])
        {
            parameter->setValueNotifyingHost(
                juce::jlimit(
                    0.0f,
                    1.0f,
                    state[(size_t)i]));
        }
    }
}

void ShakalizerAudioProcessorEditor::saveA()
{
    captureState(abState);
    hasAState = !abState.empty();
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
}

void ShakalizerAudioProcessorEditor::toggleAutoMatch()
{
    auto* parameter =
        processor.getAPVTS().getParameter(
            "autoMatch");

    if (parameter == nullptr)
        return;

    const bool current =
        parameter->getValue() > 0.5f;

    parameter->setValueNotifyingHost(
        current ? 0.0f : 1.0f);

    updateAutoMatchButton();
}

void ShakalizerAudioProcessorEditor::randomize()
{
    // GLOBAL RANDOM: every parameter in the plugin is randomized,
    // including modes, quality/sync choices and Auto Match.
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
                random.nextBool() ? 1.0f : 0.0f);
        }
        else
        {
            parameter->setValueNotifyingHost(
                random.nextFloat());
        }
    }
}
