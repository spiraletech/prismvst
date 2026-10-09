#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour kBlack  { 0xff06080a };
const juce::Colour kPanel  { 0xff11161a };
const juce::Colour kBorder { 0xff465159 };
const juce::Colour kText   { 0xffedf1f3 };
const juce::Colour kMuted  { 0xff88949c };
const juce::Colour kCyan   { 0xff49c9ee };
const juce::Colour kCyan2  { 0xffa8ebff };

constexpr std::array<const char*, PRISMVSTAudioProcessor::numEqBands> kBandNames {
    "SUB", "KICK", "LOW", "MID", "HIGH", "HIGHER"
};

constexpr std::array<const char*, 6> kControlNames {
    "FREQ", "GAIN", "DYN", "ATTACK", "RELEASE", "ONYX"
};

float clamp01(float v)
{
    return juce::jlimit(0.0f, 1.0f, v);
}
}

PrismLookAndFeel::PrismLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, kText);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff090c0e));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff68747b));
    setColour(juce::TextButton::textColourOffId, kText);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void PrismLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height)
                            .reduced(8.0f);
    const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float>(size, size).withCentre(bounds.getCentre());
    const auto centre = knob.getCentre();
    const float radius = knob.getWidth() * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour(juce::Colour(0xff050708));
    g.fillEllipse(knob.expanded(4.0f));
    g.setColour(juce::Colour(0xff667077));
    g.drawEllipse(knob.expanded(1.0f), 1.0f);

    juce::ColourGradient metal(juce::Colour(0xff3b4145), knob.getX(), knob.getY(),
                               juce::Colour(0xff090b0d), knob.getRight(), knob.getBottom(), false);
    metal.addColour(0.48, juce::Colour(0xff171c1f));
    g.setGradientFill(metal);
    g.fillEllipse(knob);

    const auto arcBounds = knob.reduced(5.0f);
    juce::Path track;
    track.addCentredArc(centre.x, centre.y,
                        arcBounds.getWidth() * 0.5f,
                        arcBounds.getHeight() * 0.5f,
                        0.0f, startAngle, endAngle, true);
    g.setColour(juce::Colour(0xff313a40));
    g.strokePath(track, juce::PathStrokeType(3.0f,
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc(centre.x, centre.y,
                        arcBounds.getWidth() * 0.5f,
                        arcBounds.getHeight() * 0.5f,
                        0.0f, startAngle, angle, true);
    g.setColour(kCyan);
    g.strokePath(value, juce::PathStrokeType(3.2f,
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    const float pointerLength = radius * 0.62f;
    g.setColour(kText);
    g.drawLine(centre.x, centre.y,
               centre.x + std::sin(angle) * pointerLength,
               centre.y - std::cos(angle) * pointerLength,
               2.0f);

    g.setColour(juce::Colours::white.withAlpha(0.07f));
    g.drawEllipse(knob.reduced(7.0f), 1.0f);
}

void PrismLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&, bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();

    juce::ColourGradient grad(on ? juce::Colour(0xff173743) : juce::Colour(0xff1a1f22),
                              r.getX(), r.getY(),
                              on ? juce::Colour(0xff0a1a20) : juce::Colour(0xff090c0e),
                              r.getX(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, 4.0f);

    g.setColour(on ? kCyan.withAlpha(down ? 1.0f : 0.82f)
                   : juce::Colour(0xff566168).withAlpha(highlighted ? 0.95f : 0.65f));
    g.drawRoundedRectangle(r, 4.0f, on ? 1.5f : 1.0f);
}

void PrismLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool, bool)
{
    g.setColour(button.getToggleState() ? juce::Colours::white : kText.withAlpha(0.82f));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(5),
                     juce::Justification::centred, 1);
}

void PrismLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                               minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto b = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    const float cx = b.getCentreX();
    const float top = b.getY() + 8.0f;
    const float bottom = b.getBottom() - 8.0f;

    g.setColour(juce::Colour(0xff040607));
    g.fillRoundedRectangle(cx - 8.0f, top, 16.0f, bottom - top, 5.0f);
    g.setColour(juce::Colour(0xff505a60));
    g.drawRoundedRectangle(cx - 8.0f, top, 16.0f, bottom - top, 5.0f, 1.0f);

    g.setColour(kCyan.withAlpha(0.85f));
    g.fillRoundedRectangle(cx - 2.0f, sliderPos, 4.0f,
                           juce::jmax(1.0f, bottom - sliderPos), 2.0f);

    auto handle = juce::Rectangle<float>(23.0f, 17.0f).withCentre({ cx, sliderPos });
    juce::ColourGradient chrome(juce::Colour(0xfff0f0ec), handle.getX(), handle.getY(),
                                juce::Colour(0xff696d70), handle.getRight(), handle.getBottom(), false);
    g.setGradientFill(chrome);
    g.fillRoundedRectangle(handle, 5.0f);
    g.setColour(juce::Colour(0xff111416));
    g.drawRoundedRectangle(handle, 5.0f, 1.0f);
}

SpectrumDisplay::SpectrumDisplay(PRISMVSTAudioProcessor& p)
    : processor(p)
{
    latestSpectrum.fill(-100.0f);
    auraPersistence.fill(0.0f);
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
    const float clamped = juce::jlimit(20.0f, 20000.0f, frequency);
    const float norm = std::log10(clamped / 20.0f) / std::log10(1000.0f);
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
    return juce::jmap(juce::jlimit(-36.0f, 0.0f, db),
                      -36.0f, 0.0f, b.getBottom(), b.getY());
}

juce::String SpectrumDisplay::bandId(int band, const juce::String& suffix) const
{
    return "band" + juce::String(band + 1) + "_" + suffix;
}

float SpectrumDisplay::parameter(const juce::String& id) const
{
    if (auto* v = processor.apvts.getRawParameterValue(id))
        return v->load();
    return 0.0f;
}

float SpectrumDisplay::rawDbAtFrequency(float frequency) const
{
    const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sr * 0.5;
    const int bin = juce::jlimit(
        0,
        PRISMVSTAudioProcessor::spectrumBins - 1,
        juce::roundToInt((float)(frequency / nyquist)
                         * (PRISMVSTAudioProcessor::spectrumBins - 1)));
    return latestSpectrum[(size_t)bin];
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
    return juce::String(names[(size_t)(clamped % 12)]) + juce::String(clamped / 12 - 1);
}

void SpectrumDisplay::setSelectedBand(int band)
{
    selectedBand = juce::jlimit(0, PRISMVSTAudioProcessor::numEqBands - 1, band);
    repaint();
}

void SpectrumDisplay::pushSpectrum(
    const std::array<float, PRISMVSTAudioProcessor::spectrumBins>& values)
{
    latestSpectrum = values;

    const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sr * 0.5;

    for (int i = 0; i < (int)auraPersistence.size(); ++i)
    {
        const float norm = (float)i / (float)(auraPersistence.size() - 1);
        const float frequency = 20.0f * std::pow(1000.0f, norm);
        const int bin = juce::jlimit(
            0,
            PRISMVSTAudioProcessor::spectrumBins - 1,
            juce::roundToInt((float)(frequency / nyquist)
                             * (PRISMVSTAudioProcessor::spectrumBins - 1)));

        const float raw = latestSpectrum[(size_t)bin];
        const float displaySlope = 4.5f * std::log2(juce::jmax(20.0f, frequency) / 1000.0f);
        const float displayDb = raw + displaySlope;
        const float target = clamp01((displayDb + 36.0f) / 36.0f);

        auraPersistence[(size_t)i] =
            juce::jmax(target, auraPersistence[(size_t)i] * 0.955f);
    }

    repaint();
}

void SpectrumDisplay::paint(juce::Graphics& g)
{
    const auto b = graphBounds();

    g.setColour(kPanel);
    g.fillRoundedRectangle(b, 4.0f);

    const std::array<float, 10> gridFreq {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    g.setFont(juce::Font(juce::FontOptions(10.0f)));

    for (float f : gridFreq)
    {
        const float x = frequencyToX(f);
        g.setColour(juce::Colour(0xff334047).withAlpha(0.48f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

        g.setColour(kMuted);
        const juce::String label = f >= 1000.0f
            ? juce::String(f / 1000.0f, f >= 10000.0f ? 0 : 1) + "k"
            : juce::String((int)f);

        g.drawText(label,
                   juce::roundToInt(x - 24.0f),
                   juce::roundToInt(b.getBottom() - 16.0f),
                   48, 14, juce::Justification::centred);
    }

    for (int db : { 0, -6, -12, -18, -24, -30, -36 })
    {
        const float y = displayDbToY((float)db);
        g.setColour(juce::Colour(0xff334047).withAlpha(db == 0 ? 0.72f : 0.42f));
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());

        g.setColour(kMuted);
        g.drawText(juce::String(db),
                   juce::roundToInt(b.getX() + 4.0f),
                   juce::roundToInt(y - 8.0f),
                   34, 14, juce::Justification::centredLeft);
    }

    // Stationary aura: persistent energy produces a radial field instead of a time conveyor.
    float weightedX = 0.0f;
    float weightSum = 0.0f;
    float maxEnergy = 0.0f;

    for (int i = 0; i < (int)auraPersistence.size(); ++i)
    {
        const float energy = auraPersistence[(size_t)i];
        const float w = energy * energy;
        const float x = b.getX() + ((float)i / (float)(auraPersistence.size() - 1)) * b.getWidth();
        weightedX += x * w;
        weightSum += w;
        maxEnergy = juce::jmax(maxEnergy, energy);
    }

    const float auraX = weightSum > 0.001f ? weightedX / weightSum : b.getCentreX();
    const float auraY = b.getCentreY() + b.getHeight() * 0.08f;

    for (int ring = 7; ring >= 1; --ring)
    {
        const float scale = (float)ring / 7.0f;
        const float rx = b.getWidth() * (0.06f + scale * 0.18f);
        const float ry = b.getHeight() * (0.08f + scale * 0.25f);
        g.setColour(kCyan.withAlpha((0.010f + (1.0f - scale) * 0.020f) * maxEnergy));
        g.drawEllipse(auraX - rx, auraY - ry, rx * 2.0f, ry * 2.0f, 1.0f);
    }

    // Current band centres are shown as restrained hardware-style frequency rails, never EQ nodes.
    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        const float frequency = parameter(bandId(i, "freq"));
        const float x = frequencyToX(frequency);

        g.setColour((i == selectedBand ? kCyan : kCyan.withAlpha(0.34f)));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

        if (i == selectedBand)
        {
            juce::String freqText = frequency >= 1000.0f
                ? juce::String(frequency / 1000.0f, 2) + " kHz"
                : juce::String(frequency, 1) + " Hz";

            auto pill = juce::Rectangle<float>(72.0f, 19.0f)
                            .withCentre({ x, b.getY() + 27.0f });

            g.setColour(juce::Colour(0xff05080a).withAlpha(0.95f));
            g.fillRoundedRectangle(pill, 3.0f);
            g.setColour(kText);
            g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
            g.drawFittedText(freqText, pill.toNearestInt(), juce::Justification::centred, 1);
        }
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
        const int bin = juce::jlimit(
            0,
            PRISMVSTAudioProcessor::spectrumBins - 1,
            juce::roundToInt((float)(frequency / nyquist)
                             * (PRISMVSTAudioProcessor::spectrumBins - 1)));

        const float raw = latestSpectrum[(size_t)bin];
        const float displaySlope = 4.5f * std::log2(juce::jmax(20.0f, frequency) / 1000.0f);
        const float displayDb = juce::jlimit(-36.0f, 0.0f, raw + displaySlope);
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

    juce::ColourGradient fill(kCyan.withAlpha(0.23f), b.getCentreX(), b.getY(),
                              kCyan.withAlpha(0.012f), b.getCentreX(), b.getBottom(), false);
    g.setGradientFill(fill);
    g.fillPath(fillPath);

    g.setColour(kCyan.withAlpha(0.10f));
    g.strokePath(spectrumPath,
                 juce::PathStrokeType(7.0f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    g.setColour(kCyan2.withAlpha(0.93f));
    g.strokePath(spectrumPath,
                 juce::PathStrokeType(1.45f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    g.setColour(kBorder.withAlpha(0.76f));
    g.drawRoundedRectangle(b, 4.0f, 1.0f);

    if (hovering && b.contains(hoverPoint))
    {
        const float frequency = xToFrequency(hoverPoint.x);
        const float rawDb = rawDbAtFrequency(frequency);

        g.setColour(kCyan.withAlpha(0.48f));
        g.drawVerticalLine(juce::roundToInt(hoverPoint.x), b.getY(), b.getBottom());
        g.drawHorizontalLine(juce::roundToInt(hoverPoint.y), b.getX(), b.getRight());

        juce::String text = frequency >= 1000.0f
            ? juce::String(frequency / 1000.0f, 2) + " kHz"
            : juce::String(frequency, 1) + " Hz";

        text << "   " << juce::String(rawDb, 1) << " dBFS   "
             << noteForFrequency(frequency);

        auto box = juce::Rectangle<float>(202.0f, 24.0f);
        box.setPosition(
            juce::jlimit(b.getX(), b.getRight() - box.getWidth(), hoverPoint.x + 10.0f),
            juce::jlimit(b.getY(), b.getBottom() - box.getHeight(), hoverPoint.y - 30.0f));

        g.setColour(juce::Colour(0xff05080a).withAlpha(0.96f));
        g.fillRoundedRectangle(box, 3.0f);
        g.setColour(kText);
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(text, box.toNearestInt().reduced(7, 2), juce::Justification::centredLeft);
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

juce::String PRISMVSTAudioProcessorEditor::bandId(int band, const juce::String& suffix) const
{
    return "band" + juce::String(band + 1) + "_" + suffix;
}

PRISMVSTAudioProcessorEditor::PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      spectrumDisplay(p)
{
    setLookAndFeel(&lookAndFeel);
    setOpaque(true);
    setResizable(true, true);
    setResizeLimits(1080, 700, 1800, 1200);
    setSize(1360, 880);

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        bandButtons[(size_t)i] = std::make_unique<juce::TextButton>(kBandNames[(size_t)i]);
        auto& button = *bandButtons[(size_t)i];
        button.setClickingTogglesState(false);
        button.onClick = [this, i] { bindSelectedBand(i); };
        addAndMakeVisible(button);
    }

    enabledButton.setClickingTogglesState(true);
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(enabledButton);
    addAndMakeVisible(bypassButton);

    configureKnob(frequencySlider, " Hz", 60.0, 2);
    configureKnob(gainSlider, " dB", 0.0, 2);
    configureKnob(dynamicSlider, " dB", 0.0, 2);
    configureKnob(attackSlider, " ms", 10.0, 2);
    configureKnob(releaseSlider, " ms", 120.0, 1);
    configureKnob(onyxSlider, " %", 0.0, 1);

    addAndMakeVisible(frequencySlider);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(dynamicSlider);
    addAndMakeVisible(attackSlider);
    addAndMakeVisible(releaseSlider);
    addAndMakeVisible(onyxSlider);

    configureLever();
    addAndMakeVisible(precisionLever);

    selectedBandLabel.setColour(juce::Label::textColourId, kCyan);
    selectedBandLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    addAndMakeVisible(selectedBandLabel);

    statusLabel.setColour(juce::Label::textColourId, kMuted);
    statusLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    statusLabel.setText("stable DSP  •  analyzer 0 to -36 dBFS  •  4.5 dB/oct display slope  •  no scrolling",
                        juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    for (auto* label : { &peakLabel, &lufsShortLabel, &lufsIntegratedLabel })
    {
        label->setColour(juce::Label::textColourId, kText);
        label->setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        label->setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(*label);
    }

    addAndMakeVisible(spectrumDisplay);

    onyxA = std::make_unique<SliderAttachment>(processor.apvts, "onyx", onyxSlider);
    bypassA = std::make_unique<ButtonAttachment>(processor.apvts, "master_bypass", bypassButton);

    bindSelectedBand(0);
    startTimerHz(30);
}

PRISMVSTAudioProcessorEditor::~PRISMVSTAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void PRISMVSTAudioProcessorEditor::configureKnob(juce::Slider& slider,
                                                  const juce::String& suffix,
                                                  double defaultValue,
                                                  int decimals)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 98, 24);
    slider.setTextValueSuffix(suffix);
    slider.setNumDecimalPlacesToDisplay(decimals);
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

void PRISMVSTAudioProcessorEditor::bindSelectedBand(int band)
{
    selectedBand = juce::jlimit(0, PRISMVSTAudioProcessor::numEqBands - 1, band);
    spectrumDisplay.setSelectedBand(selectedBand);

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
        bandButtons[(size_t)i]->setToggleState(i == selectedBand, juce::dontSendNotification);

    selectedBandLabel.setText(kBandNames[(size_t)selectedBand], juce::dontSendNotification);

    frequencyA.reset();
    gainA.reset();
    dynamicA.reset();
    attackA.reset();
    releaseA.reset();
    enabledA.reset();

    frequencyA = std::make_unique<SliderAttachment>(
        processor.apvts, bandId(selectedBand, "freq"), frequencySlider);
    gainA = std::make_unique<SliderAttachment>(
        processor.apvts, bandId(selectedBand, "gain"), gainSlider);
    dynamicA = std::make_unique<SliderAttachment>(
        processor.apvts, bandId(selectedBand, "dyn_range"), dynamicSlider);
    attackA = std::make_unique<SliderAttachment>(
        processor.apvts, bandId(selectedBand, "attack"), attackSlider);
    releaseA = std::make_unique<SliderAttachment>(
        processor.apvts, bandId(selectedBand, "release"), releaseSlider);
    enabledA = std::make_unique<ButtonAttachment>(
        processor.apvts, bandId(selectedBand, "enabled"), enabledButton);

    updatePrecisionSensitivity();
    repaint();
}

void PRISMVSTAudioProcessorEditor::updatePrecisionSensitivity()
{
    const double p = precisionLever.getValue();
    const int pixels = juce::roundToInt(260.0 + p * p * 5200.0);

    for (auto* slider : {
             &frequencySlider, &gainSlider, &dynamicSlider,
             &attackSlider, &releaseSlider, &onyxSlider })
    {
        slider->setMouseDragSensitivity(pixels);
    }
}

void PRISMVSTAudioProcessorEditor::timerCallback()
{
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> spectrum {};
    processor.copySpectrum(spectrum);
    spectrumDisplay.pushSpectrum(spectrum);

    peakLabel.setText("PEAK  " + juce::String(processor.getPeakDb(), 1) + " dBFS",
                      juce::dontSendNotification);
    lufsShortLabel.setText("LUFS-S  " + juce::String(processor.getLufsShort(), 1),
                           juce::dontSendNotification);
    lufsIntegratedLabel.setText("LUFS-I  " + juce::String(processor.getLufsIntegrated(), 1),
                                juce::dontSendNotification);

    repaint();
}

void PRISMVSTAudioProcessorEditor::paintMetalPanel(juce::Graphics& g,
                                                    juce::Rectangle<float> r,
                                                    bool lighter) const
{
    const auto top = lighter ? juce::Colour(0xff262b2e) : juce::Colour(0xff181d20);
    const auto bottom = lighter ? juce::Colour(0xff111416) : juce::Colour(0xff0a0d0f);

    juce::ColourGradient grad(top, r.getX(), r.getY(),
                              bottom, r.getX(), r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, 6.0f);

    for (int y = juce::roundToInt(r.getY()) + 2;
         y < juce::roundToInt(r.getBottom()); y += 4)
    {
        g.setColour(juce::Colours::white.withAlpha((y % 8 == 0) ? 0.012f : 0.006f));
        g.drawHorizontalLine(y, r.getX() + 4.0f, r.getRight() - 4.0f);
    }

    g.setColour(kBorder.withAlpha(0.76f));
    g.drawRoundedRectangle(r, 6.0f, 1.0f);
}

void PRISMVSTAudioProcessorEditor::paintMeter(juce::Graphics& g,
                                               juce::Rectangle<float> r,
                                               float db) const
{
    g.setColour(juce::Colour(0xff040607));
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(kBorder.withAlpha(0.75f));
    g.drawRoundedRectangle(r, 3.0f, 1.0f);

    const float clamped = juce::jlimit(-60.0f, 0.0f, db);
    const float norm = (clamped + 60.0f) / 60.0f;

    auto fill = r.reduced(5.0f);
    const float bottom = fill.getBottom();
    fill.setY(bottom - fill.getHeight() * norm);
    fill.setHeight(bottom - fill.getY());

    juce::ColourGradient meter(kCyan.withAlpha(0.98f), fill.getCentreX(), fill.getBottom(),
                               kCyan2.withAlpha(0.98f), fill.getCentreX(), fill.getY(), false);
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
    g.drawText("PRISM",
               header.withTrimmedLeft(18.0f).withWidth(145.0f).toNearestInt(),
               juce::Justification::centredLeft);

    g.setColour(kCyan);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("RECOVERY 001   v0.3.4",
               header.withTrimmedLeft(178.0f).withWidth(175.0f).toNearestInt(),
               juce::Justification::centredLeft);

    g.setColour(kMuted);
    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    g.drawText("ETHERTECH  /  STABLE DYNAMIC EQ CORE  /  HARDWARE SHELF UI",
               header.withTrimmedLeft(370.0f).withWidth(470.0f).toNearestInt(),
               juce::Justification::centredLeft);

    const auto tabsPanel = juce::Rectangle<float>(
        12.0f, 58.0f, bounds.getWidth() - 24.0f, 50.0f);
    paintMetalPanel(g, tabsPanel, false);

    const auto graphPanel = juce::Rectangle<float>(
        12.0f, 114.0f, bounds.getWidth() - 98.0f, bounds.getHeight() - 362.0f);
    paintMetalPanel(g, graphPanel, false);

    const auto meterPanel = juce::Rectangle<float>(
        bounds.getWidth() - 78.0f, 114.0f, 66.0f, bounds.getHeight() - 362.0f);
    paintMetalPanel(g, meterPanel, false);
    paintMeter(g, meterPanel.reduced(18.0f, 34.0f), processor.getPeakDb());

    const auto infoPanel = juce::Rectangle<float>(
        12.0f, bounds.getHeight() - 240.0f, bounds.getWidth() - 24.0f, 34.0f);
    paintMetalPanel(g, infoPanel, false);

    const auto controlPanel = juce::Rectangle<float>(
        12.0f, bounds.getHeight() - 200.0f, bounds.getWidth() - 24.0f, 188.0f);
    paintMetalPanel(g, controlPanel, true);

    for (auto p : {
             juce::Point<float>(20.0f, 18.0f),
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

        g.drawText(kControlNames[(size_t)i],
                   x, controlY, cellWidth - 10, 22,
                   juce::Justification::centred);

        if (i > 0)
        {
            g.setColour(juce::Colour(0xff59636a).withAlpha(0.36f));
            g.drawVerticalLine(x,
                               (float)controlY + 18.0f,
                               (float)getHeight() - 28.0f);
            g.setColour(kText.withAlpha(0.88f));
        }
    }

    g.drawText("PRECISION",
               getWidth() - 154, controlY, 126, 22,
               juce::Justification::centred);
}

void PRISMVSTAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    peakLabel.setBounds(w - 390, 14, 120, 24);
    lufsShortLabel.setBounds(w - 270, 14, 125, 24);
    lufsIntegratedLabel.setBounds(w - 145, 14, 125, 24);

    const int tabX = 30;
    const int tabY = 68;
    const int tabGap = 6;
    const int tabWidth = (w - 60 - tabGap * (PRISMVSTAudioProcessor::numEqBands - 1))
                         / PRISMVSTAudioProcessor::numEqBands;

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        bandButtons[(size_t)i]->setBounds(
            tabX + i * (tabWidth + tabGap),
            tabY,
            tabWidth,
            30);
    }

    spectrumDisplay.setBounds(18, 120, w - 110, h - 374);

    selectedBandLabel.setBounds(28, h - 235, 90, 26);
    statusLabel.setBounds(112, h - 235, w - 390, 26);
    enabledButton.setBounds(w - 176, h - 232, 72, 24);
    bypassButton.setBounds(w - 98, h - 232, 70, 24);

    const int controlTop = h - 174;
    const int availableWidth = w - 170;
    const int cellWidth = availableWidth / 6;
    const int knobSize = juce::jmin(126, cellWidth - 18);

    std::array<juce::Slider*, 6> sliders {
        &frequencySlider, &gainSlider, &dynamicSlider,
        &attackSlider, &releaseSlider, &onyxSlider
    };

    for (int i = 0; i < 6; ++i)
    {
        const int x = 28 + i * cellWidth + (cellWidth - knobSize) / 2;
        sliders[(size_t)i]->setBounds(x, controlTop, knobSize, 142);
    }

    precisionLever.setBounds(w - 126, controlTop + 8, 72, 126);
}
