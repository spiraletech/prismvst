#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr juce::Colour kBlack { 0xff07090b };
constexpr juce::Colour kPanel { 0xff11161a };
constexpr juce::Colour kBorder { 0xff46515a };
constexpr juce::Colour kText { 0xffe8edf0 };
constexpr juce::Colour kMuted { 0xff8b979f };
constexpr juce::Colour kCyan { 0xff4cc9ef };
constexpr juce::Colour kCyanSoft { 0xff9ee8ff };

constexpr std::array<const char*, PRISMVSTAudioProcessor::numSections> kSectionNames {
    "SUB", "KICK", "LOW", "LOWER MID", "MID", "HIGH", "HIGHER"
};

constexpr std::array<const char*, 6> kControlNames {
    "INPUT", "OUTPUT", "ATTACK", "RELEASE", "WIDTH", "ONYX"
};

float clamp01(float x)
{
    return juce::jlimit(0.0f, 1.0f, x);
}
}

PrismLookAndFeel::PrismLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, kText);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0a0d0f));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff7a858c));
    setColour(juce::TextButton::textColourOffId, kText);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void PrismLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height)
                            .reduced(6.0f);
    const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float>(size, size).withCentre(bounds.getCentre());
    const auto centre = knob.getCentre();
    const float radius = knob.getWidth() * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour(juce::Colour(0xff060809));
    g.fillEllipse(knob.expanded(3.0f));
    g.setColour(juce::Colour(0xff59636a));
    g.drawEllipse(knob.expanded(1.0f), 1.0f);

    juce::ColourGradient metal(juce::Colour(0xff343a3e), centre.x - radius, centre.y - radius,
                               juce::Colour(0xff090b0d), centre.x + radius, centre.y + radius, false);
    metal.addColour(0.52, juce::Colour(0xff181d20));
    g.setGradientFill(metal);
    g.fillEllipse(knob);

    const auto arcBounds = knob.reduced(4.0f);
    juce::Path track;
    track.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f,
                        arcBounds.getHeight() * 0.5f, 0.0f,
                        startAngle, endAngle, true);
    g.setColour(juce::Colour(0xff313b41));
    g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f,
                           arcBounds.getHeight() * 0.5f, 0.0f,
                           startAngle, angle, true);
    g.setColour(kCyan);
    g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    const float pointerLength = radius * 0.60f;
    const float px = centre.x + std::sin(angle) * pointerLength;
    const float py = centre.y - std::cos(angle) * pointerLength;
    g.setColour(kText);
    g.drawLine(centre.x, centre.y, px, py, 2.0f);

    juce::Path highlight;
    const auto hi = knob.reduced(6.0f);
    highlight.addCentredArc(centre.x, centre.y, hi.getWidth() * 0.5f, hi.getHeight() * 0.5f,
                            0.0f, juce::MathConstants<float>::pi * 1.05f,
                            juce::MathConstants<float>::pi * 1.55f, true);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.strokePath(highlight, juce::PathStrokeType(1.0f));
}

void PrismLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&, bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();

    juce::ColourGradient grad(
        on ? juce::Colour(0xff173744) : juce::Colour(0xff1a2024), r.getX(), r.getY(),
        on ? juce::Colour(0xff0c1e25) : juce::Colour(0xff0b0e10), r.getX(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, 4.0f);

    if (on)
    {
        g.setColour(kCyan.withAlpha(down ? 0.95f : 0.78f));
        g.drawRoundedRectangle(r, 4.0f, 1.4f);
        g.setColour(kCyan.withAlpha(highlighted ? 0.14f : 0.08f));
        g.fillRoundedRectangle(r.reduced(2.0f), 3.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xff4a555d).withAlpha(highlighted ? 0.9f : 0.6f));
        g.drawRoundedRectangle(r, 4.0f, 1.0f);
    }
}

void PrismLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool, bool)
{
    g.setColour(button.getToggleState() ? juce::Colours::white : kText.withAlpha(0.82f));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4),
                     juce::Justification::centred, 1);
}

void PrismLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                               minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    const float cx = bounds.getCentreX();
    const float top = (float)y + 8.0f;
    const float bottom = (float)(y + height) - 8.0f;

    g.setColour(juce::Colour(0xff050708));
    g.fillRoundedRectangle(cx - 8.0f, top, 16.0f, bottom - top, 5.0f);
    g.setColour(juce::Colour(0xff4a555c));
    g.drawRoundedRectangle(cx - 8.0f, top, 16.0f, bottom - top, 5.0f, 1.0f);

    g.setColour(kCyan.withAlpha(0.85f));
    g.fillRoundedRectangle(cx - 2.0f, sliderPos, 4.0f, bottom - sliderPos, 2.0f);

    auto handle = juce::Rectangle<float>(22.0f, 16.0f).withCentre({ cx, sliderPos });
    juce::ColourGradient chrome(juce::Colour(0xfff4f4ef), handle.getX(), handle.getY(),
                                juce::Colour(0xff63686b), handle.getRight(), handle.getBottom(), false);
    g.setGradientFill(chrome);
    g.fillRoundedRectangle(handle, 5.0f);
    g.setColour(juce::Colour(0xff151719));
    g.drawRoundedRectangle(handle, 5.0f, 1.0f);
}

SpectrumDisplay::SpectrumDisplay(PRISMVSTAudioProcessor& p)
    : processor(p)
{
    latestSpectrum.fill(-144.0f);
    auraEnvelope.fill(0.0f);
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    setRepaintsOnMouseActivity(true);
}

juce::Rectangle<float> SpectrumDisplay::graphBounds() const
{
    return getLocalBounds().toFloat().reduced(20.0f, 18.0f);
}

float SpectrumDisplay::frequencyToX(float frequency) const
{
    const auto b = graphBounds();
    const float f = juce::jlimit(20.0f, 20000.0f, frequency);
    const float norm = std::log10(f / 20.0f) / std::log10(1000.0f);
    return b.getX() + norm * b.getWidth();
}

float SpectrumDisplay::xToFrequency(float x) const
{
    const auto b = graphBounds();
    const float norm = juce::jlimit(0.0f, 1.0f, (x - b.getX()) / b.getWidth());
    return 20.0f * std::pow(1000.0f, norm);
}

float SpectrumDisplay::displayDbToY(float db) const
{
    const auto b = graphBounds();
    return juce::jmap(juce::jlimit(-36.0f, 0.0f, db), -36.0f, 0.0f,
                      b.getBottom(), b.getY());
}

float SpectrumDisplay::rawDbAtFrequency(float frequency) const
{
    const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sr * 0.5;
    const int bin = juce::jlimit(0, PRISMVSTAudioProcessor::spectrumBins - 1,
        juce::roundToInt((float)(frequency / nyquist)
                         * (PRISMVSTAudioProcessor::spectrumBins - 1)));
    return latestSpectrum[(size_t)bin];
}

int SpectrumDisplay::sectionForFrequency(float frequency) const
{
    for (int i = 0; i < PRISMVSTAudioProcessor::numSplits; ++i)
        if (frequency < PRISMVSTAudioProcessor::splitFrequencies[(size_t)i])
            return i;
    return PRISMVSTAudioProcessor::numSections - 1;
}

juce::String SpectrumDisplay::noteForFrequency(float frequency) const
{
    if (frequency <= 0.0f)
        return "--";

    static constexpr std::array<const char*, 12> names {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    const int midi = juce::roundToInt(69.0 + 12.0 * std::log2((double)frequency / 440.0));
    const int clamped = juce::jlimit(0, 127, midi);
    const int note = clamped % 12;
    const int octave = clamped / 12 - 1;
    return juce::String(names[(size_t)note]) + juce::String(octave);
}

void SpectrumDisplay::setSelectedSection(int section)
{
    selectedSection = juce::jlimit(0, PRISMVSTAudioProcessor::numSections - 1, section);
    repaint();
}

void SpectrumDisplay::pushSpectrum(
    const std::array<float, PRISMVSTAudioProcessor::spectrumBins>& values)
{
    latestSpectrum = values;
    const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sr * 0.5;

    for (int i = 0; i < (int)auraEnvelope.size(); ++i)
    {
        const float norm = (float)i / (float)(auraEnvelope.size() - 1);
        const float frequency = 20.0f * std::pow(1000.0f, norm);
        const int bin = juce::jlimit(0, PRISMVSTAudioProcessor::spectrumBins - 1,
            juce::roundToInt((float)(frequency / nyquist)
                             * (PRISMVSTAudioProcessor::spectrumBins - 1));

        const float raw = latestSpectrum[(size_t)bin];
        const float slopeComp = 4.5f * std::log2(juce::jmax(20.0f, frequency) / 1000.0f);
        const float displayDb = raw + slopeComp;
        const float target = clamp01((displayDb + 36.0f) / 36.0f);
        auraEnvelope[(size_t)i] = juce::jmax(target, auraEnvelope[(size_t)i] * 0.955f);
    }

    repaint();
}

void SpectrumDisplay::paint(juce::Graphics& g)
{
    const auto b = graphBounds();

    g.setColour(kPanel);
    g.fillRoundedRectangle(b, 4.0f);

    const float leftFreq = selectedSection == 0
        ? 20.0f : PRISMVSTAudioProcessor::splitFrequencies[(size_t)(selectedSection - 1)];
    const float rightFreq = selectedSection == PRISMVSTAudioProcessor::numSections - 1
        ? 20000.0f : PRISMVSTAudioProcessor::splitFrequencies[(size_t)selectedSection];
    const float sx0 = frequencyToX(leftFreq);
    const float sx1 = frequencyToX(rightFreq);
    g.setColour(kCyan.withAlpha(0.035f));
    g.fillRect(juce::Rectangle<float>(sx0, b.getY(), sx1 - sx0, b.getHeight()));

    const std::array<float, 10> gridFreq {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    for (float frequency : gridFreq)
    {
        const float x = frequencyToX(frequency);
        g.setColour(juce::Colour(0xff334047).withAlpha(0.48f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

        g.setColour(kMuted);
        const juce::String label = frequency >= 1000.0f
            ? juce::String(frequency / 1000.0f, frequency >= 10000.0f ? 0 : 1) + "k"
            : juce::String((int)frequency);
        g.drawText(label, juce::roundToInt(x - 24.0f), juce::roundToInt(b.getBottom() - 16.0f),
                   48, 14, juce::Justification::centred);
    }

    for (int db : { 0, -6, -12, -18, -24, -30, -36 })
    {
        const float y = displayDbToY((float)db);
        g.setColour(juce::Colour(0xff334047).withAlpha(db == 0 ? 0.72f : 0.42f));
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());
        g.setColour(kMuted);
        g.drawText(juce::String(db), juce::roundToInt(b.getX() + 4.0f),
                   juce::roundToInt(y - 8.0f), 34, 14, juce::Justification::centredLeft);
    }

    for (float frequency : PRISMVSTAudioProcessor::splitFrequencies)
    {
        const float x = frequencyToX(frequency);
        g.setColour(kCyan.withAlpha(0.74f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

        const juce::String text = frequency >= 1000.0f
            ? juce::String(frequency / 1000.0f, frequency < 10000.0f ? 2 : 0) + " kHz"
            : juce::String(frequency, 0) + " Hz";
        auto pill = juce::Rectangle<float>(62.0f, 18.0f).withCentre({ x, b.getY() + 26.0f });
        g.setColour(juce::Colour(0xff05080a).withAlpha(0.94f));
        g.fillRoundedRectangle(pill, 3.0f);
        g.setColour(kText);
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.drawFittedText(text, pill.toNearestInt(), juce::Justification::centred, 1);
    }

    float weightedX = 0.0f;
    float weightSum = 0.0f;
    for (int i = 0; i < (int)auraEnvelope.size(); ++i)
    {
        const float w = auraEnvelope[(size_t)i] * auraEnvelope[(size_t)i];
        const float x = b.getX() + ((float)i / (float)(auraEnvelope.size() - 1)) * b.getWidth();
        weightedX += x * w;
        weightSum += w;
    }

    const float auraX = weightSum > 0.001f ? weightedX / weightSum : b.getCentreX();
    const float auraY = b.getCentreY() + b.getHeight() * 0.10f;

    for (int ring = 5; ring >= 1; --ring)
    {
        const float rx = b.getWidth() * (0.08f + ring * 0.035f);
        const float ry = b.getHeight() * (0.10f + ring * 0.050f);
        g.setColour(kCyan.withAlpha(0.010f + 0.006f * (6 - ring)));
        g.drawEllipse(auraX - rx, auraY - ry, rx * 2.0f, ry * 2.0f, 1.0f);
    }

    juce::Path spectrumPath;
    juce::Path fillPath;
    bool started = false;
    const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sr * 0.5;

    for (int px = 0; px <= juce::roundToInt(b.getWidth()); px += 2)
    {
        const float x = b.getX() + (float)px;
        const float frequency = xToFrequency(x);
        const int bin = juce::jlimit(0, PRISMVSTAudioProcessor::spectrumBins - 1,
            juce::roundToInt((float)(frequency / nyquist)
                             * (PRISMVSTAudioProcessor::spectrumBins - 1)));
        const float raw = latestSpectrum[(size_t)bin];
        const float slopeComp = 4.5f * std::log2(juce::jmax(20.0f, frequency) / 1000.0f);
        const float displayDb = juce::jlimit(-36.0f, 0.0f, raw + slopeComp);
        const float y = displayDbToY(displayDb);

        if (!started)
        {
            spectrumPath.startNewSubPath(x, y);
            fillPath.startNewSubPath(x, b.getBottom());
            fillPath.lineTo(x, y);
            started = true;
        }
        else
        {
            spectrumPath.lineTo(x, y);
            fillPath.lineTo(x, y);
        }
    }

    fillPath.lineTo(b.getRight(), b.getBottom());
    fillPath.closeSubPath();

    juce::ColourGradient fill(kCyan.withAlpha(0.26f), b.getCentreX(), b.getY(),
                              kCyan.withAlpha(0.015f), b.getCentreX(), b.getBottom(), false);
    g.setGradientFill(fill);
    g.fillPath(fillPath);

    g.setColour(kCyan.withAlpha(0.10f));
    g.strokePath(spectrumPath, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    g.setColour(kCyanSoft.withAlpha(0.92f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

    g.setColour(kBorder.withAlpha(0.72f));
    g.drawRoundedRectangle(b, 4.0f, 1.0f);

    if (hovering && b.contains(hoverPoint))
    {
        const float frequency = xToFrequency(hoverPoint.x);
        const float db = rawDbAtFrequency(frequency);
        g.setColour(kCyan.withAlpha(0.55f));
        g.drawVerticalLine(juce::roundToInt(hoverPoint.x), b.getY(), b.getBottom());
        g.drawHorizontalLine(juce::roundToInt(hoverPoint.y), b.getX(), b.getRight());

        juce::String readout = frequency >= 1000.0f
            ? juce::String(frequency / 1000.0f, 2) + " kHz"
            : juce::String(frequency, 1) + " Hz";
        readout << "   " << juce::String(db, 1) << " dBFS   " << noteForFrequency(frequency);

        auto box = juce::Rectangle<float>(198.0f, 24.0f);
        box.setPosition(juce::jlimit(b.getX(), b.getRight() - box.getWidth(), hoverPoint.x + 10.0f),
                        juce::jlimit(b.getY(), b.getBottom() - box.getHeight(), hoverPoint.y - 30.0f));
        g.setColour(juce::Colour(0xff05080a).withAlpha(0.95f));
        g.fillRoundedRectangle(box, 3.0f);
        g.setColour(kText);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(readout, box.toNearestInt().reduced(7, 2), juce::Justification::centredLeft);
    }
}

void SpectrumDisplay::mouseMove(const juce::MouseEvent& e)
{
    hoverPoint = e.position;
    hovering = graphBounds().contains(hoverPoint);
    repaint();
}

void SpectrumDisplay::mouseExit(const juce::MouseEvent&)
{
    hovering = false;
    repaint();
}

void SpectrumDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (!graphBounds().contains(e.position))
        return;

    const int section = sectionForFrequency(xToFrequency(e.position.x));
    setSelectedSection(section);
    if (onSectionSelected)
        onSectionSelected(section);
}

PRISMVSTAudioProcessorEditor::PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), spectrumDisplay(p)
{
    setLookAndFeel(&lookAndFeel);
    setOpaque(true);
    setResizable(true, true);
    setResizeLimits(1080, 700, 1800, 1200);
    setSize(1360, 880);

    for (int i = 0; i < PRISMVSTAudioProcessor::numSections; ++i)
    {
        sectionButtons[(size_t)i] = std::make_unique<juce::TextButton>(kSectionNames[(size_t)i]);
        auto& button = *sectionButtons[(size_t)i];
        button.setClickingTogglesState(false);
        button.onClick = [this, i] { bindSelectedSection(i); };
        addAndMakeVisible(button);
    }

    enabledButton.setClickingTogglesState(true);
    soloButton.setClickingTogglesState(true);
    addAndMakeVisible(enabledButton);
    addAndMakeVisible(soloButton);

    configureKnob(inputSlider, " dB", 0.0);
    configureKnob(outputSlider, " dB", 0.0);
    configureKnob(attackSlider, " ms", 10.0);
    configureKnob(releaseSlider, " ms", 180.0, 0.1);
    configureKnob(widthSlider, " %", 100.0, 0.1);
    configureKnob(onyxSlider, " %", 0.0, 0.1);

    addAndMakeVisible(inputSlider);
    addAndMakeVisible(outputSlider);
    addAndMakeVisible(attackSlider);
    addAndMakeVisible(releaseSlider);
    addAndMakeVisible(widthSlider);
    addAndMakeVisible(onyxSlider);

    configureLever();
    addAndMakeVisible(precisionLever);

    selectedSectionLabel.setColour(juce::Label::textColourId, kCyan);
    selectedSectionLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    addAndMakeVisible(selectedSectionLabel);

    statusLabel.setColour(juce::Label::textColourId, kMuted);
    statusLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    statusLabel.setText("fixed 36 dB/oct crossover territory  •  PRECISION lever = drag resolution",
                        juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    for (auto* label : { &inputMeterLabel, &grMeterLabel, &outputMeterLabel })
    {
        label->setColour(juce::Label::textColourId, kText);
        label->setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        label->setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(*label);
    }

    spectrumDisplay.onSectionSelected = [this](int section) { bindSelectedSection(section); };
    addAndMakeVisible(spectrumDisplay);

    bindSelectedSection(0);
    startTimerHz(30);
}

PRISMVSTAudioProcessorEditor::~PRISMVSTAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void PRISMVSTAudioProcessorEditor::configureKnob(juce::Slider& slider,
                                                  const juce::String& suffix,
                                                  double defaultValue,
                                                  double interval)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 96, 24);
    slider.setTextValueSuffix(suffix);
    slider.setNumDecimalPlacesToDisplay(interval < 0.1 ? 2 : 1);
    slider.setDoubleClickReturnValue(true, defaultValue);
    slider.setMouseDragSensitivity(320);
}

void PRISMVSTAudioProcessorEditor::configureLever()
{
    precisionLever.setSliderStyle(juce::Slider::LinearVertical);
    precisionLever.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    precisionLever.setRange(0.0, 1.0, 0.001);
    precisionLever.setValue(0.20, juce::dontSendNotification);
    precisionLever.onValueChange = [this] { updatePrecisionSensitivity(); };
}

void PRISMVSTAudioProcessorEditor::bindSelectedSection(int section)
{
    selectedSection = juce::jlimit(0, PRISMVSTAudioProcessor::numSections - 1, section);
    spectrumDisplay.setSelectedSection(selectedSection);

    for (int i = 0; i < PRISMVSTAudioProcessor::numSections; ++i)
        sectionButtons[(size_t)i]->setToggleState(i == selectedSection, juce::dontSendNotification);

    selectedSectionLabel.setText(kSectionNames[(size_t)selectedSection], juce::dontSendNotification);

    inputA.reset();
    outputA.reset();
    attackA.reset();
    releaseA.reset();
    widthA.reset();
    onyxA.reset();
    enabledA.reset();
    soloA.reset();

    const auto id = [this](const juce::String& suffix) {
        return PRISMVSTAudioProcessor::sectionId(selectedSection, suffix);
    };

    inputA = std::make_unique<SliderAttachment>(processor.apvts, id("input"), inputSlider);
    outputA = std::make_unique<SliderAttachment>(processor.apvts, id("output"), outputSlider);
    attackA = std::make_unique<SliderAttachment>(processor.apvts, id("attack"), attackSlider);
    releaseA = std::make_unique<SliderAttachment>(processor.apvts, id("release"), releaseSlider);
    widthA = std::make_unique<SliderAttachment>(processor.apvts, id("width"), widthSlider);
    onyxA = std::make_unique<SliderAttachment>(processor.apvts, id("onyx"), onyxSlider);
    enabledA = std::make_unique<ButtonAttachment>(processor.apvts, id("enabled"), enabledButton);
    soloA = std::make_unique<ButtonAttachment>(processor.apvts, id("solo"), soloButton);

    updatePrecisionSensitivity();
    repaint();
}

void PRISMVSTAudioProcessorEditor::updatePrecisionSensitivity()
{
    const double p = precisionLever.getValue();
    const int pixels = juce::roundToInt(260.0 + p * p * 5200.0);
    for (auto* slider : { &inputSlider, &outputSlider, &attackSlider, &releaseSlider, &widthSlider, &onyxSlider })
        slider->setMouseDragSensitivity(pixels);
}

void PRISMVSTAudioProcessorEditor::timerCallback()
{
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> spectrum {};
    processor.copySpectrum(spectrum);
    spectrumDisplay.pushSpectrum(spectrum);

    inputMeterLabel.setText("IN  " + juce::String(processor.getInputPeakDb(), 1) + " dBFS",
                            juce::dontSendNotification);
    grMeterLabel.setText("GR  " + juce::String(processor.getGainReductionDb(), 1) + " dB",
                         juce::dontSendNotification);
    outputMeterLabel.setText("OUT  " + juce::String(processor.getOutputPeakDb(), 1) + " dBFS",
                             juce::dontSendNotification);
    repaint();
}

void PRISMVSTAudioProcessorEditor::paintMetalPanel(juce::Graphics& g,
                                                    juce::Rectangle<float> r,
                                                    bool lighter) const
{
    const auto top = lighter ? juce::Colour(0xff252a2d) : juce::Colour(0xff181d20);
    const auto bottom = lighter ? juce::Colour(0xff121517) : juce::Colour(0xff0b0e10);
    juce::ColourGradient grad(top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, 6.0f);

    for (int y = juce::roundToInt(r.getY()) + 2; y < juce::roundToInt(r.getBottom()); y += 4)
    {
        g.setColour(juce::Colours::white.withAlpha((y % 8 == 0) ? 0.012f : 0.006f));
        g.drawHorizontalLine(y, r.getX() + 4.0f, r.getRight() - 4.0f);
    }

    g.setColour(kBorder.withAlpha(0.72f));
    g.drawRoundedRectangle(r, 6.0f, 1.0f);
}

void PRISMVSTAudioProcessorEditor::paintOutputMeter(juce::Graphics& g,
                                                     juce::Rectangle<float> r) const
{
    g.setColour(juce::Colour(0xff040607));
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(kBorder.withAlpha(0.75f));
    g.drawRoundedRectangle(r, 3.0f, 1.0f);

    const float db = juce::jlimit(-60.0f, 0.0f, processor.getOutputPeakDb());
    const float norm = (db + 60.0f) / 60.0f;
    auto fill = r.reduced(5.0f);
    const float fillBottom = fill.getBottom();
    fill.setY(fillBottom - fill.getHeight() * norm);
    fill.setHeight(fillBottom - fill.getY());

    juce::ColourGradient meter(kCyan.withAlpha(0.98f), fill.getCentreX(), fill.getBottom(),
                               kCyanSoft.withAlpha(0.98f), fill.getCentreX(), fill.getY(), false);
    g.setGradientFill(meter);
    g.fillRoundedRectangle(fill, 1.5f);
}

void PRISMVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBlack);

    const auto bounds = getLocalBounds().toFloat();
    const auto header = bounds.withHeight(52.0f).reduced(12.0f, 7.0f);
    paintMetalPanel(g, header, false);

    g.setColour(kText);
    g.setFont(juce::Font(juce::FontOptions(26.0f, juce::Font::bold)));
    g.drawText("PRISM", header.withTrimmedLeft(18.0f).withWidth(145.0f).toNearestInt(),
               juce::Justification::centredLeft);

    g.setColour(kCyan);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("ALPHA 001   v0.3.3", header.withTrimmedLeft(178.0f).withWidth(150.0f).toNearestInt(),
               juce::Justification::centredLeft);

    g.setColour(kMuted);
    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    g.drawText("ETHERTECH  /  7-SECTION SPECTRAL DYNAMICS  /  FIXED 36 dB/OCT",
               header.withTrimmedLeft(350.0f).withWidth(450.0f).toNearestInt(),
               juce::Justification::centredLeft);

    const auto tabsPanel = juce::Rectangle<float>(12.0f, 58.0f, bounds.getWidth() - 24.0f, 50.0f);
    paintMetalPanel(g, tabsPanel, false);

    const auto graphPanel = juce::Rectangle<float>(12.0f, 114.0f,
                                                    bounds.getWidth() - 98.0f,
                                                    bounds.getHeight() - 362.0f);
    paintMetalPanel(g, graphPanel, false);

    const auto meterPanel = juce::Rectangle<float>(bounds.getWidth() - 78.0f, 114.0f,
                                                    66.0f, bounds.getHeight() - 362.0f);
    paintMetalPanel(g, meterPanel, false);
    paintOutputMeter(g, meterPanel.reduced(18.0f, 34.0f));

    const auto infoPanel = juce::Rectangle<float>(12.0f, bounds.getHeight() - 240.0f,
                                                   bounds.getWidth() - 24.0f, 34.0f);
    paintMetalPanel(g, infoPanel, false);

    const auto controlPanel = juce::Rectangle<float>(12.0f, bounds.getHeight() - 200.0f,
                                                      bounds.getWidth() - 24.0f, 188.0f);
    paintMetalPanel(g, controlPanel, true);

    for (auto p : { juce::Point<float>(20.0f, 18.0f),
                    juce::Point<float>(bounds.getRight() - 20.0f, 18.0f),
                    juce::Point<float>(20.0f, bounds.getBottom() - 20.0f),
                    juce::Point<float>(bounds.getRight() - 20.0f, bounds.getBottom() - 20.0f) })
    {
        g.setColour(juce::Colour(0xff050607));
        g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colour(0xff737b80));
        g.drawEllipse(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f, 1.0f);
        g.drawLine(p.x - 2.5f, p.y, p.x + 2.5f, p.y, 1.0f);
    }

    const int controlY = getHeight() - 188;
    const int availableWidth = getWidth() - 170;
    const int cellWidth = availableWidth / 6;
    g.setColour(kText.withAlpha(0.88f));
    g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));

    for (int i = 0; i < 6; ++i)
    {
        const int x = 28 + i * cellWidth;
        g.drawText(kControlNames[(size_t)i], x, controlY, cellWidth - 10, 22,
                   juce::Justification::centred);

        if (i > 0)
        {
            g.setColour(juce::Colour(0xff59636a).withAlpha(0.36f));
            g.drawVerticalLine(x, (float)controlY + 18.0f, (float)getHeight() - 28.0f);
            g.setColour(kText.withAlpha(0.88f));
        }
    }

    g.drawText("PRECISION", getWidth() - 154, controlY, 126, 22,
               juce::Justification::centred);
}

void PRISMVSTAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    inputMeterLabel.setBounds(w - 390, 14, 120, 24);
    grMeterLabel.setBounds(w - 265, 14, 105, 24);
    outputMeterLabel.setBounds(w - 155, 14, 135, 24);

    const int tabX = 30;
    const int tabY = 68;
    const int tabGap = 6;
    const int tabWidth = (w - 60 - tabGap * 6) / 7;
    for (int i = 0; i < PRISMVSTAudioProcessor::numSections; ++i)
        sectionButtons[(size_t)i]->setBounds(tabX + i * (tabWidth + tabGap), tabY, tabWidth, 30);

    spectrumDisplay.setBounds(18, 120, w - 110, h - 374);

    selectedSectionLabel.setBounds(28, h - 235, 90, 26);
    statusLabel.setBounds(112, h - 235, w - 360, 26);
    enabledButton.setBounds(w - 168, h - 232, 72, 24);
    soloButton.setBounds(w - 90, h - 232, 62, 24);

    const int controlTop = h - 174;
    const int availableWidth = w - 170;
    const int cellWidth = availableWidth / 6;
    const int knobSize = juce::jmin(126, cellWidth - 18);

    std::array<juce::Slider*, 6> sliders {
        &inputSlider, &outputSlider, &attackSlider, &releaseSlider, &widthSlider, &onyxSlider
    };

    for (int i = 0; i < 6; ++i)
    {
        const int x = 28 + i * cellWidth + (cellWidth - knobSize) / 2;
        sliders[(size_t)i]->setBounds(x, controlTop, knobSize, 142);
    }

    precisionLever.setBounds(w - 126, controlTop + 8, 72, 126);
}
