#include "PluginEditor.h"

namespace
{
const juce::Colour background = juce::Colour::fromRGB(10, 9, 12);
const juce::Colour panel = juce::Colour::fromRGB(19, 17, 22);
const juce::Colour panel2 = juce::Colour::fromRGB(28, 25, 32);
const juce::Colour accent = juce::Colour::fromRGB(237, 35, 60);
const juce::Colour text = juce::Colour::fromRGB(245, 242, 245);
const juce::Colour muted = juce::Colour::fromRGB(151, 143, 154);

const std::array<const char*, 24> allParameterIds {
    "shakal", "destroy", "crush", "decimate", "drive", "clip", "glitch", "jitter",
    "split", "transient", "body", "stereo", "movement", "unstable", "alien",
    "filterFreq", "filterRes", "mix", "output", "mode", "resampleMode",
    "filterType", "movementShape", "autoMatch"
};
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
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(7.0f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.37f;
    const auto c = b.getCentre();

    g.setColour(juce::Colour::fromRGB(42, 37, 46));
    g.fillEllipse(c.x-r, c.y-r, r*2.0f, r*2.0f);

    g.setColour(juce::Colour::fromRGB(76, 67, 81));
    g.drawEllipse(c.x-r, c.y-r, r*2.0f, r*2.0f, 2.0f);

    const float angle = start + p * (end-start);
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, r+3.0f, r+3.0f, 0.0f, start, angle, true);

    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

    const float dotR = 3.5f;
    const float dx = c.x + std::cos(angle - juce::MathConstants<float>::halfPi) * (r-4.0f);
    const float dy = c.y + std::sin(angle - juce::MathConstants<float>::halfPi) * (r-4.0f);

    g.setColour(text);
    g.fillEllipse(dx-dotR, dy-dotR, dotR*2.0f, dotR*2.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool down, int, int, int, int, juce::ComboBox&)
{
    g.setColour(down ? panel : panel2);
    g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 8.0f);
    g.setColour(accent.withAlpha(0.45f));
    g.drawRoundedRectangle(0.5f, 0.5f, (float)width-1.0f, (float)height-1.0f, 8.0f, 1.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button, const juce::Colour&,
    bool highlighted, bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour((down || highlighted) ? panel2.brighter(0.08f) : panel2);
    g.fillRoundedRectangle(b, 8.0f);

    if (button.getToggleState())
        g.setColour(accent.withAlpha(0.85f));
    else
        g.setColour(juce::Colour::fromRGB(68, 59, 72));

    g.drawRoundedRectangle(b, 8.0f, 1.0f);
}

void ShakalizerAudioProcessorEditor::Meter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour::fromRGB(37, 32, 41));
    g.fillRoundedRectangle(b, 5.0f);

    const float h = b.getHeight() * level;
    g.setColour(accent);
    g.fillRoundedRectangle(b.withTop(b.getBottom() - h), 5.0f);
}

ShakalizerAudioProcessorEditor::ShakalizerAudioProcessorEditor(ShakalizerAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(false, false);
    setSize(1120, 760);

    titleLabel.setText("SHAKALIZER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(32.0f, juce::Font::bold)));
    titleLabel.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("DIGITAL DESTRUCTION / MOVEMENT / STEREO", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    subtitleLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(subtitleLabel);

    modeBox.addItemList({ "Clean", "Crunch", "Shakal", "Destroy", "Fried", "Pixel", "Alien", "Melt" }, 1);
    resampleBox.addItemList({ "Hold", "Linear", "Stair", "Smear", "Random" }, 1);
    filterBox.addItemList({ "Low Pass", "Band Pass", "High Pass" }, 1);
    movementBox.addItemList({ "Sine", "Triangle", "Sample+Hold", "Stepped" }, 1);

    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "mode", modeBox);
    resampleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "resampleMode", resampleBox);
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "filterType", filterBox);
    movementAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "movementShape", movementBox);

    addAndMakeVisible(modeBox);
    addAndMakeVisible(resampleBox);
    addAndMakeVisible(filterBox);
    addAndMakeVisible(movementBox);

    saveAButton.onClick = [this] { saveA(); };
    abButton.onClick = [this] { swapAB(); };
    autoMatchButton.onClick = [this] { toggleAutoMatch(); };
    randomButton.onClick = [this] { randomize(); };

    for (auto* button : { &saveAButton, &abButton, &autoMatchButton, &randomButton })
    {
        button->setColour(juce::TextButton::textColourOnId, text);
        button->setColour(juce::TextButton::textColourOffId, text);
        addAndMakeVisible(*button);
    }

    meterLabel.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    meterLabel.setColour(juce::Label::textColourId, muted);
    meterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterLabel);
    addAndMakeVisible(meter);

    const std::array<double, 19> mins {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,80,0.05,0,-12
    };
    const std::array<double, 19> maxs {
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,18000,0.95,1,6
    };
    const std::array<double, 19> steps {
        0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,
        0.001,0.001,0.001,0.001,0.001,1,0.001,0.001,0.01
    };

    for (size_t i = 0; i < sliders.size(); ++i)
    {
        configureSlider(*sliders[i], sliderNames[i], mins[i], maxs[i], steps[i]);
        addAndMakeVisible(*sliders[i]);
    }

    addAttachment("shakal", shakalSlider);
    addAttachment("destroy", destroySlider);
    addAttachment("crush", crushSlider);
    addAttachment("decimate", decimateSlider);
    addAttachment("drive", driveSlider);
    addAttachment("clip", clipSlider);
    addAttachment("glitch", glitchSlider);
    addAttachment("jitter", jitterSlider);
    addAttachment("split", splitSlider);
    addAttachment("transient", transientSlider);
    addAttachment("body", bodySlider);
    addAttachment("stereo", stereoSlider);
    addAttachment("movement", movementSlider);
    addAttachment("unstable", unstableSlider);
    addAttachment("alien", alienSlider);
    addAttachment("filterFreq", filterFreqSlider);
    addAttachment("filterRes", filterResSlider);
    addAttachment("mix", mixSlider);
    addAttachment("output", outputSlider);

    startTimerHz(20);
}

ShakalizerAudioProcessorEditor::~ShakalizerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ShakalizerAudioProcessorEditor::configureSlider(
    juce::Slider& slider, const juce::String& name,
    double min, double max, double step)
{
    slider.setName(name);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    slider.setRange(min, max, step);
    slider.setColour(juce::Slider::textBoxTextColourId, text);
    slider.setTooltip(name);

    if (name == "FILTER FREQ")
        slider.setSkewFactorFromMidPoint(1600.0);
}

void ShakalizerAudioProcessorEditor::addAttachment(
    const juce::String& id, juce::Slider& slider)
{
    attachments.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getAPVTS(), id, slider));
}

void ShakalizerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    auto outer = getLocalBounds().reduced(16);
    g.setColour(panel);
    g.fillRoundedRectangle(outer.toFloat(), 16.0f);

    g.setColour(accent.withAlpha(0.88f));
    g.fillRoundedRectangle(16.0f, 16.0f, 7.0f, 104.0f, 3.0f);

    g.setColour(panel2);
    g.fillRoundedRectangle(30.0f, 130.0f, 1060.0f, 590.0f, 14.0f);

    g.setColour(juce::Colour::fromRGB(49, 43, 52));
    g.drawHorizontalLine(304, 48.0f, 1070.0f);
    g.drawHorizontalLine(478, 48.0f, 1070.0f);

    g.setColour(accent.withAlpha(0.10f));
    g.fillEllipse(54.0f, 154.0f, 190.0f, 190.0f);
}

void ShakalizerAudioProcessorEditor::resized()
{
    titleLabel.setBounds(42, 25, 300, 42);
    subtitleLabel.setBounds(43, 68, 360, 22);

    modeBox.setBounds(440, 29, 128, 34);
    resampleBox.setBounds(578, 29, 108, 34);
    filterBox.setBounds(696, 29, 108, 34);
    movementBox.setBounds(814, 29, 130, 34);

    saveAButton.setBounds(956, 29, 62, 34);
    abButton.setBounds(1024, 29, 56, 34);

    randomButton.setBounds(440, 76, 108, 30);
    autoMatchButton.setBounds(556, 76, 108, 30);
    meterLabel.setBounds(1024, 76, 40, 14);
    meter.setBounds(1074, 72, 8, 34);

    // Large macro.
    shakalSlider.setBounds(48, 154, 192, 142);

    const int left = 260;
    const int top = 148;
    const int cellW = 136;
    const int cellH = 112;

    // First row: core destruction.
    const std::array<int, 3> first { 1, 2, 3 };
    for (int i = 0; i < 3; ++i)
        sliders[(size_t)first[(size_t)i]]->setBounds(left + i*cellW, top, cellW-8, cellH);

    // Second row: nonlinear + glitch.
    const std::array<int, 5> second { 4, 5, 6, 7, 8 };
    for (int i = 0; i < 5; ++i)
        sliders[(size_t)second[(size_t)i]]->setBounds(260 + i*136, 266, 128, 102);

    // Third row: dynamics / stereo / movement.
    const std::array<int, 5> third { 9, 10, 11, 12, 13 };
    for (int i = 0; i < 5; ++i)
        sliders[(size_t)third[(size_t)i]]->setBounds(260 + i*136, 378, 128, 102);

    // Bottom row: alien / filter / output.
    const std::array<int, 6> bottom { 14, 15, 16, 17, 18, 0 };
    sliders[14]->setBounds(260, 492, 128, 102);
    sliders[15]->setBounds(396, 492, 128, 102);
    sliders[16]->setBounds(532, 492, 128, 102);
    sliders[17]->setBounds(668, 492, 128, 102);
    sliders[18]->setBounds(804, 492, 128, 102);
}

void ShakalizerAudioProcessorEditor::timerCallback()
{
    meter.setLevel(processor.getMeterLevel());
    updateAutoMatchButton();
}

void ShakalizerAudioProcessorEditor::updateAutoMatchButton()
{
    const bool on = processor.getAPVTS()
        .getRawParameterValue("autoMatch")->load() > 0.5f;

    autoMatchButton.setToggleState(on, juce::dontSendNotification);
}

void ShakalizerAudioProcessorEditor::captureState(std::vector<float>& destination)
{
    destination.clear();
    destination.reserve(allParameterIds.size());

    for (const auto* id : allParameterIds)
    {
        if (auto* p = processor.getAPVTS().getParameter(id))
            destination.push_back(p->getValue());
    }
}

void ShakalizerAudioProcessorEditor::applyState(const std::vector<float>& state)
{
    const auto count = juce::jmin((int)state.size(), (int)allParameterIds.size());

    for (int i = 0; i < count; ++i)
    {
        if (auto* p = processor.getAPVTS().getParameter(allParameterIds[(size_t)i]))
            p->setValueNotifyingHost(state[(size_t)i]);
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
    const bool current = processor.getAPVTS()
        .getRawParameterValue("autoMatch")->load() > 0.5f;

    if (auto* p = processor.getAPVTS().getParameter("autoMatch"))
        p->setValueNotifyingHost(current ? 0.0f : 1.0f);

    updateAutoMatchButton();
}

void ShakalizerAudioProcessorEditor::randomize()
{
    static juce::Random random;

    const std::array<const char*, 18> ids {
        "shakal", "destroy", "crush", "decimate", "drive", "clip", "glitch", "jitter",
        "split", "transient", "body", "stereo", "movement", "unstable", "alien",
        "filterFreq", "filterRes", "mix"
    };

    for (const auto* id : ids)
    {
        if (auto* p = processor.getAPVTS().getParameter(id))
            p->setValueNotifyingHost(random.nextFloat());
    }

    // Keep destructive auto-randomisation from accidentally creating silence.
    if (auto* p = processor.getAPVTS().getParameter("mix"))
        p->setValueNotifyingHost(0.65f + random.nextFloat() * 0.30f);

    if (auto* p = processor.getAPVTS().getParameter("output"))
        p->setValueNotifyingHost(0.30f + random.nextFloat() * 0.35f);
}
