#include "PluginEditor.h"

void PrecisionSlider::mouseDown(const juce::MouseEvent& e)
{
    dragStartValue = getValue();
    dragStartY = e.getScreenY();
    precisionDrag = e.mods.isCtrlDown();

    juce::Slider::mouseDown(e);
}

void PrecisionSlider::mouseDrag(const juce::MouseEvent& e)
{
    const bool wantsPrecision = e.mods.isCtrlDown();

    if (wantsPrecision) {
        if (!precisionDrag) {
            dragStartValue = getValue();
            dragStartY = e.getScreenY();
            precisionDrag = true;
        }

        const auto range = getRange();
        const double span = range.getLength();
        const double deltaPixels = static_cast<double>(dragStartY - e.getScreenY());
        const double newValue = dragStartValue + (deltaPixels * span / precisionPixelsForFullRange);
        setValue(juce::jlimit(range.getStart(), range.getEnd(), newValue), juce::sendNotificationSync);
        return;
    }

    precisionDrag = false;
    juce::Slider::mouseDrag(e);
}

void PrecisionSlider::mouseUp(const juce::MouseEvent& e)
{
    precisionDrag = false;
    juce::Slider::mouseUp(e);
}

namespace {

constexpr std::array<const char*, 5> names { "SUB", "KICK", "LOW", "MID", "HIGH" };
juce::String pre(int i) { return juce::String(names[(size_t)i]).toLowerCase(); }
}

PRISMVSTAudioProcessorEditor::PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(980, 540);

    for (int i = 0; i < 5; ++i) {
        auto& u = bandUi[(size_t)i];
        u.title.setText(names[(size_t)i], juce::dontSendNotification);
        u.title.setJustificationType(juce::Justification::centred);
        u.title.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
        addAndMakeVisible(u.title);

        configureSlider(u.scrape, " dB");
        configureSlider(u.depth, " dB");
        configureSlider(u.tone);
        configureSlider(u.density);
        configureSlider(u.trim, " dB");

        for (auto* s : { &u.scrape, &u.depth, &u.tone, &u.density, &u.trim })
            addAndMakeVisible(*s);

        addAndMakeVisible(u.solo);
        addAndMakeVisible(u.bypass);

        const auto id = pre(i);
        u.aScrape = std::make_unique<SliderAttachment>(processor.apvts, id + "_scrape", u.scrape);
        u.aDepth = std::make_unique<SliderAttachment>(processor.apvts, id + "_depth", u.depth);
        u.aTone = std::make_unique<SliderAttachment>(processor.apvts, id + "_tone", u.tone);
        u.aDensity = std::make_unique<SliderAttachment>(processor.apvts, id + "_density", u.density);
        u.aTrim = std::make_unique<SliderAttachment>(processor.apvts, id + "_trim", u.trim);
        u.aSolo = std::make_unique<ButtonAttachment>(processor.apvts, id + "_solo", u.solo);
        u.aBypass = std::make_unique<ButtonAttachment>(processor.apvts, id + "_bypass", u.bypass);
    }

    configureSlider(masterTrim, " dB");
    configureSlider(ceiling, " dB");
    addAndMakeVisible(masterTrim);
    addAndMakeVisible(ceiling);
    addAndMakeVisible(masterBypass);
    addAndMakeVisible(peakLabel);
    addAndMakeVisible(lufsShortLabel);
    addAndMakeVisible(lufsIntLabel);

    for (auto* l : { &peakLabel, &lufsShortLabel, &lufsIntLabel }) {
        l->setJustificationType(juce::Justification::centredLeft);
        l->setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    }

    masterTrimA = std::make_unique<SliderAttachment>(processor.apvts, "master_trim", masterTrim);
    ceilingA = std::make_unique<SliderAttachment>(processor.apvts, "ceiling", ceiling);
    masterBypassA = std::make_unique<ButtonAttachment>(processor.apvts, "master_bypass", masterBypass);

    startTimerHz(20);
}

void PRISMVSTAudioProcessorEditor::configureSlider(juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::LinearVertical);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 20);
    s.setTextValueSuffix(suffix);
}

void PRISMVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(10, 12, 15));
    g.setColour(juce::Colour::fromRGB(235, 238, 242));
    g.setFont(juce::Font(juce::FontOptions(24.0f, juce::Font::bold)));
    g.drawText("PRISM", 22, 14, 160, 32, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(90, 96, 108));
    g.drawLine(20.0f, 54.0f, (float)getWidth() - 20.0f, 54.0f, 1.0f);

    static constexpr std::array<const char*, 5> rows { "SCRAPE", "DEPTH", "TONE", "DENSITY", "TRIM" };
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.setColour(juce::Colour::fromRGB(145, 151, 163));
    for (int r = 0; r < 5; ++r)
        g.drawText(rows[(size_t)r], 20, 95 + r * 72, 70, 18, juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("MASTER", getWidth() - 145, 65, 100, 20, juce::Justification::centred);
    g.drawText("TRIM", getWidth() - 145, 92, 100, 18, juce::Justification::centred);
    g.drawText("CEILING", getWidth() - 145, 280, 100, 18, juce::Justification::centred);
}

void PRISMVSTAudioProcessorEditor::resized()
{
    const int left = 92;
    const int top = 65;
    const int bandW = 142;

    for (int i = 0; i < 5; ++i) {
        auto& u = bandUi[(size_t)i];
        const int x = left + i * bandW;
        u.title.setBounds(x, top, 110, 24);
        u.scrape.setBounds(x + 10, 95, 90, 62);
        u.depth.setBounds(x + 10, 167, 90, 62);
        u.tone.setBounds(x + 10, 239, 90, 62);
        u.density.setBounds(x + 10, 311, 90, 62);
        u.trim.setBounds(x + 10, 383, 90, 62);
        u.solo.setBounds(x + 12, 468, 42, 24);
        u.bypass.setBounds(x + 60, 468, 42, 24);
    }

    const int mx = getWidth() - 145;
    masterTrim.setBounds(mx + 10, 110, 80, 150);
    ceiling.setBounds(mx + 10, 298, 80, 120);
    masterBypass.setBounds(mx + 5, 430, 90, 28);

    peakLabel.setBounds(mx - 15, 465, 140, 18);
    lufsShortLabel.setBounds(mx - 15, 485, 140, 18);
    lufsIntLabel.setBounds(mx - 15, 505, 140, 18);
}

void PRISMVSTAudioProcessorEditor::timerCallback()
{
    peakLabel.setText("PEAK  " + juce::String(processor.getPeakDb(), 1) + " dBFS", juce::dontSendNotification);
    lufsShortLabel.setText("LUFS-S " + juce::String(processor.getLufsShort(), 1), juce::dontSendNotification);
    lufsIntLabel.setText("LUFS-I " + juce::String(processor.getLufsIntegrated(), 1), juce::dontSendNotification);
}
