#include "PluginEditor.h"

namespace
{
const juce::Colour background = juce::Colour::fromRGB(11, 10, 13);
const juce::Colour panel = juce::Colour::fromRGB(20, 18, 23);
const juce::Colour panel2 = juce::Colour::fromRGB(28, 25, 31);
const juce::Colour accent = juce::Colour::fromRGB(235, 35, 57);
const juce::Colour text = juce::Colour::fromRGB(245, 242, 245);
const juce::Colour muted = juce::Colour::fromRGB(154, 146, 156);
}

ShakalizerAudioProcessorEditor::ShakalLookAndFeel::ShakalLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, accent);
    setColour(juce::Slider::rotarySliderFillColourId, accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(59, 52, 62));
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, panel2);
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(67, 59, 70));
    setColour(juce::ComboBox::textColourId, text);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float p, float start, float end, juce::Slider&)
{
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(8.0f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.38f;
    const auto c = b.getCentre();

    g.setColour(juce::Colour::fromRGB(42, 37, 45));
    g.fillEllipse(c.x-r, c.y-r, r*2.0f, r*2.0f);

    g.setColour(juce::Colour::fromRGB(80, 71, 84));
    g.drawEllipse(c.x-r, c.y-r, r*2.0f, r*2.0f, 2.0f);

    const float angle = start + p * (end-start);
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, r+3.0f, r+3.0f, 0.0f, start, angle, true);

    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float dotR = 4.0f;
    const float dx = c.x + std::cos(angle - juce::MathConstants<float>::halfPi) * (r-4.0f);
    const float dy = c.y + std::sin(angle - juce::MathConstants<float>::halfPi) * (r-4.0f);

    g.setColour(text);
    g.fillEllipse(dx-dotR, dy-dotR, dotR*2.0f, dotR*2.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawLinearSlider(
    juce::Graphics& g, int x, int y, int width, int height, float p, float minPos, float maxPos,
    juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(minPos, maxPos, style, slider);
    const float cy = y + height * 0.5f;

    g.setColour(juce::Colour::fromRGB(51, 44, 54));
    g.fillRoundedRectangle((float)x, cy-2.0f, (float)width, 4.0f, 2.0f);

    g.setColour(accent);
    g.fillRoundedRectangle((float)x, cy-2.0f, width*p, 4.0f, 2.0f);

    g.setColour(text);
    g.fillEllipse((float)x + width*p - 5.0f, cy-5.0f, 10.0f, 10.0f);
}

void ShakalizerAudioProcessorEditor::ShakalLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool down, int, int, int, int, juce::ComboBox&)
{
    g.setColour(down ? panel : panel2);
    g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 8.0f);
    g.setColour(accent.withAlpha(0.55f));
    g.drawRoundedRectangle(0.5f, 0.5f, (float)width-1.0f, (float)height-1.0f, 8.0f, 1.0f);
}

void ShakalizerAudioProcessorEditor::Meter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour::fromRGB(37, 32, 41));
    g.fillRoundedRectangle(b, 6.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(b.withTop(b.getBottom() - b.getHeight()*level), 6.0f);
}

ShakalizerAudioProcessorEditor::ShakalizerAudioProcessorEditor(ShakalizerAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(false, false);
    setSize(860, 570);

    titleLabel.setText("SHAKALIZER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("DIGITAL DESTRUCTION / LO-FI / GLITCH", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(12.0f, juce::Font::plain));
    subtitleLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(subtitleLabel);

    modeLabel.setText("MODE", juce::dontSendNotification);
    modeLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    modeLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(modeLabel);

    modeBox.addItemList({ "Clean", "Crunch", "Shakal", "Destroy", "Fried" }, 1);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getAPVTS(), "mode", modeBox);
    addAndMakeVisible(modeBox);

    randomButton.setColour(juce::TextButton::buttonColourId, panel2);
    randomButton.setColour(juce::TextButton::textColourOnId, text);
    randomButton.setColour(juce::TextButton::textColourOffId, text);
    randomButton.onClick = [this] { randomize(); };
    addAndMakeVisible(randomButton);

    meterLabel.setFont(juce::Font(9.0f, juce::Font::bold));
    meterLabel.setColour(juce::Label::textColourId, muted);
    meterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterLabel);
    addAndMakeVisible(meter);

    for (size_t i=0; i<sliders.size(); ++i)
    {
        configureSlider(*sliders[i], sliderNames[i]);
        addAndMakeVisible(*sliders[i]);
    }

    addAttachment("destroy", destroySlider);
    addAttachment("crush", crushSlider);
    addAttachment("decimate", decimateSlider);
    addAttachment("drive", driveSlider);
    addAttachment("clip", clipSlider);
    addAttachment("glitch", glitchSlider);
    addAttachment("jitter", jitterSlider);
    addAttachment("tone", toneSlider);
    addAttachment("mix", mixSlider);
    addAttachment("output", outputSlider);

    startTimerHz(30);
}

ShakalizerAudioProcessorEditor::~ShakalizerAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void ShakalizerAudioProcessorEditor::configureSlider(juce::Slider& slider, const juce::String& name)
{
    slider.setName(name);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 19);
    slider.setRange(0.0, 1.0, 0.001);
    slider.setColour(juce::Slider::textBoxTextColourId, text);
}

void ShakalizerAudioProcessorEditor::addAttachment(const juce::String& id, juce::Slider& slider)
{
    attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.getAPVTS(), id, slider));
}

void ShakalizerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    auto outer = getLocalBounds().reduced(18);
    g.setColour(panel);
    g.fillRoundedRectangle(outer.toFloat(), 16.0f);

    g.setColour(accent.withAlpha(0.85f));
    g.fillRoundedRectangle(18.0f, 18.0f, 7.0f, 90.0f, 3.0f);

    g.setColour(panel2);
    g.fillRoundedRectangle(32.0f, 120.0f, 796.0f, 388.0f, 14.0f);

    g.setColour(juce::Colour::fromRGB(46, 40, 49));
    g.drawHorizontalLine(250, 50.0f, 812.0f);
    g.drawHorizontalLine(378, 50.0f, 812.0f);
}

void ShakalizerAudioProcessorEditor::resized()
{
    titleLabel.setBounds(48, 28, 260, 40);
    subtitleLabel.setBounds(49, 68, 300, 24);
    modeLabel.setBounds(545, 34, 55, 18);
    modeBox.setBounds(594, 28, 128, 34);
    randomButton.setBounds(734, 28, 76, 34);
    meterLabel.setBounds(785, 78, 30, 14);
    meter.setBounds(792, 94, 16, 22);

    const int startX = 45, startY = 145, cellW = 155, cellH = 128;

    for (int i=0; i<8; ++i)
    {
        const int row=i/4, col=i%4;
        sliders[(size_t)i]->setBounds(startX+col*cellW, startY+row*cellH, cellW-10, cellH-8);
    }

    mixSlider.setBounds(355, 401, 155, 95);
    outputSlider.setBounds(515, 401, 155, 95);
}

void ShakalizerAudioProcessorEditor::timerCallback()
{
    meter.setLevel(processor.getMeterLevel());
}

void ShakalizerAudioProcessorEditor::randomize()
{
    static juce::Random random;
    const std::array<const char*,8> ids { "destroy","crush","decimate","drive","clip","glitch","jitter","tone" };

    for (const auto* id : ids)
        if (auto* parameter = processor.getAPVTS().getParameter(id))
            parameter->setValueNotifyingHost(random.nextFloat());

    if (auto* parameter = processor.getAPVTS().getParameter("mix"))
        parameter->setValueNotifyingHost(juce::jlimit(0.0f,1.0f,0.65f+random.nextFloat()*0.30f));
}
