#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr std::array<const char*, 8> kControlNames {
    "FREQ", "GAIN", "Q", "DYN", "THRESH", "RATIO", "ATTACK", "RELEASE"
};

constexpr std::array<float, 9> kSolfeggio {
    174.0f, 285.0f, 396.0f, 417.0f, 528.0f, 639.0f, 741.0f, 852.0f, 963.0f
};

float readParameter(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    if (auto* value = state.getRawParameterValue(id))
        return value->load();
    return 0.0f;
}
}

void PrecisionSlider::mouseDown(const juce::MouseEvent& e)
{
    dragStartValue = getValue();
    dragStartY = e.getScreenY();
    precisionDrag = e.mods.isCtrlDown();
    juce::Slider::mouseDown(e);
}

void PrecisionSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isCtrlDown())
    {
        if (!precisionDrag)
        {
            dragStartValue = getValue();
            dragStartY = e.getScreenY();
            precisionDrag = true;
        }

        const auto range = getRange();
        const double span = range.getLength();
        const double deltaPixels = static_cast<double>(dragStartY - e.getScreenY());
        const double newValue = dragStartValue + deltaPixels * span / precisionPixelsForFullRange;

        setValue(juce::jlimit(range.getStart(), range.getEnd(), newValue),
                 juce::sendNotificationSync);
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

SpectrumEQDisplay::SpectrumEQDisplay(PRISMVSTAudioProcessor& p)
    : processor(p)
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    latestSpectrum.fill(-100.0f);
    for (auto& row : heatHistory)
        row.fill(-100.0f);
}

juce::String SpectrumEQDisplay::id(int band, const juce::String& suffix) const
{
    return "band" + juce::String(band + 1) + "_" + suffix;
}

float SpectrumEQDisplay::parameter(const juce::String& parameterId) const
{
    return readParameter(processor.apvts, parameterId);
}

void SpectrumEQDisplay::setParameter(const juce::String& parameterId, float value)
{
    if (auto* p = processor.apvts.getParameter(parameterId))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}

juce::Rectangle<float> SpectrumEQDisplay::graphBounds() const
{
    return getLocalBounds().toFloat().reduced(18.0f, 18.0f);
}

float SpectrumEQDisplay::frequencyToX(float frequency) const
{
    const auto b = graphBounds();
    const float clamped = juce::jlimit(20.0f, 20000.0f, frequency);
    const float norm = std::log10(clamped / 20.0f) / std::log10(1000.0f);
    return b.getX() + norm * b.getWidth();
}

float SpectrumEQDisplay::xToFrequency(float x) const
{
    const auto b = graphBounds();
    const float norm = juce::jlimit(0.0f, 1.0f, (x - b.getX()) / b.getWidth());
    return 20.0f * std::pow(1000.0f, norm);
}

float SpectrumEQDisplay::gainToY(float gainDb) const
{
    const auto b = graphBounds();
    return juce::jmap(juce::jlimit(-18.0f, 18.0f, gainDb),
                      -18.0f, 18.0f, b.getBottom(), b.getY());
}

float SpectrumEQDisplay::yToGain(float y) const
{
    const auto b = graphBounds();
    return juce::jmap(juce::jlimit(b.getY(), b.getBottom(), y),
                      b.getBottom(), b.getY(), -18.0f, 18.0f);
}

juce::Point<float> SpectrumEQDisplay::nodePosition(int band) const
{
    return { frequencyToX(parameter(id(band, "freq"))),
             gainToY(parameter(id(band, "gain"))) };
}

int SpectrumEQDisplay::findNearestNode(juce::Point<float> point) const
{
    int best = -1;
    float bestDistance = 26.0f;

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        const float d = point.getDistanceFrom(nodePosition(i));
        if (d < bestDistance)
        {
            bestDistance = d;
            best = i;
        }
    }

    return best;
}

void SpectrumEQDisplay::setSelectedBand(int band)
{
    selectedBand = juce::jlimit(0, PRISMVSTAudioProcessor::numEqBands - 1, band);
    repaint();
}

void SpectrumEQDisplay::pushSpectrum(
    const std::array<float, PRISMVSTAudioProcessor::spectrumBins>& values)
{
    latestSpectrum = values;
    auto& row = heatHistory[(size_t)heatWriteRow];

    const double sampleRate = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sampleRate * 0.5;

    for (int x = 0; x < heatColumns; ++x)
    {
        const float norm = (float)x / (float)(heatColumns - 1);
        const float freq = 20.0f * std::pow(1000.0f, norm);
        const int bin = juce::jlimit(
            0,
            PRISMVSTAudioProcessor::spectrumBins - 1,
            juce::roundToInt((float)(freq / nyquist)
                             * (PRISMVSTAudioProcessor::spectrumBins - 1)));

        row[(size_t)x] = latestSpectrum[(size_t)bin];
    }

    heatWriteRow = (heatWriteRow + 1) % heatRows;
    repaint();
}

void SpectrumEQDisplay::paint(juce::Graphics& g)
{
    const auto b = graphBounds();

    g.setColour(juce::Colour::fromRGB(13, 16, 20));
    g.fillRoundedRectangle(b, 7.0f);

    for (int row = 0; row < heatRows; ++row)
    {
        const int historyIndex = (heatWriteRow + row) % heatRows;
        const float y0 = b.getY() + ((float)row / (float)heatRows) * b.getHeight();
        const float y1 = b.getY() + ((float)(row + 1) / (float)heatRows) * b.getHeight();

        for (int col = 0; col < heatColumns; ++col)
        {
            const float level = heatHistory[(size_t)historyIndex][(size_t)col];
            const float intensity = juce::jlimit(0.0f, 1.0f, (level + 82.0f) / 72.0f);
            if (intensity <= 0.015f)
                continue;

            const float x0 = b.getX() + ((float)col / (float)heatColumns) * b.getWidth();
            const float x1 = b.getX() + ((float)(col + 1) / (float)heatColumns) * b.getWidth();

            g.setColour(juce::Colour::fromFloatRGBA(
                0.12f + intensity * 0.10f,
                0.38f + intensity * 0.42f,
                0.52f + intensity * 0.38f,
                0.025f + intensity * 0.13f));

            g.fillRect(juce::Rectangle<float>(x0, y0, x1 - x0 + 1.0f, y1 - y0 + 1.0f));
        }
    }

    const std::array<float, 10> gridFreq {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    g.setFont(juce::Font(juce::FontOptions(10.0f)));

    for (float f : gridFreq)
    {
        const float x = frequencyToX(f);
        g.setColour(juce::Colour::fromRGB(49, 56, 65).withAlpha(0.65f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

        g.setColour(juce::Colour::fromRGB(134, 143, 154));
        const juce::String label = f >= 1000.0f
            ? juce::String(f / 1000.0f, f >= 10000.0f ? 0 : 1) + "k"
            : juce::String((int)f);

        g.drawText(label,
                   juce::roundToInt(x - 24.0f),
                   juce::roundToInt(b.getBottom() - 17.0f),
                   48, 14, juce::Justification::centred);
    }

    for (int dbValue : { -18, -12, -6, 0, 6, 12, 18 })
    {
        const float y = gainToY((float)dbValue);
        g.setColour(juce::Colour::fromRGB(49, 56, 65).withAlpha(dbValue == 0 ? 0.95f : 0.42f));
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());

        g.setColour(juce::Colour::fromRGB(134, 143, 154));
        g.drawText((dbValue > 0 ? "+" : "") + juce::String(dbValue),
                   juce::roundToInt(b.getX() + 4.0f),
                   juce::roundToInt(y - 8.0f),
                   34, 14, juce::Justification::centredLeft);
    }

    if (parameter("solfeggio_grid") > 0.5f)
    {
        for (float f : kSolfeggio)
        {
            const float x = frequencyToX(f);
            g.setColour(juce::Colour::fromRGB(173, 144, 91).withAlpha(0.46f));
            g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

            g.setColour(juce::Colour::fromRGB(196, 169, 113));
            g.drawText(juce::String((int)f),
                       juce::roundToInt(x - 22.0f),
                       juce::roundToInt(b.getY() + 5.0f),
                       44, 14, juce::Justification::centred);
        }
    }

    juce::Path spectrumPath;
    bool started = false;
    const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sr * 0.5;

    for (int px = 0; px <= juce::roundToInt(b.getWidth()); px += 2)
    {
        const float x = b.getX() + (float)px;
        const float freq = xToFrequency(x);
        const int bin = juce::jlimit(
            0,
            PRISMVSTAudioProcessor::spectrumBins - 1,
            juce::roundToInt((float)(freq / nyquist)
                             * (PRISMVSTAudioProcessor::spectrumBins - 1)));

        const float level = juce::jlimit(-90.0f, 0.0f, latestSpectrum[(size_t)bin]);
        const float y = juce::jmap(level, -90.0f, 0.0f, b.getBottom() - 4.0f, b.getY() + 10.0f);

        if (!started)
        {
            spectrumPath.startNewSubPath(x, y);
            started = true;
        }
        else
        {
            spectrumPath.lineTo(x, y);
        }
    }

    g.setColour(juce::Colour::fromRGB(66, 173, 213).withAlpha(0.78f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.4f));

    std::array<juce::dsp::IIR::Coefficients<float>::Ptr,
               PRISMVSTAudioProcessor::numEqBands> coeffs;

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        if (parameter(id(i, "enabled")) < 0.5f)
            continue;

        coeffs[(size_t)i] = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr,
            parameter(id(i, "freq")),
            parameter(id(i, "q")),
            juce::Decibels::decibelsToGain(parameter(id(i, "gain"))));
    }

    juce::Path curve;
    started = false;

    for (int px = 0; px <= juce::roundToInt(b.getWidth()); px += 2)
    {
        const float x = b.getX() + (float)px;
        const double freq = xToFrequency(x);
        double magnitude = 1.0;

        for (const auto& coeff : coeffs)
            if (coeff != nullptr)
                magnitude *= coeff->getMagnitudeForFrequency(freq, sr);

        const float curveDb = juce::Decibels::gainToDecibels((float)magnitude, -60.0f);
        const float y = gainToY(curveDb);

        if (!started)
        {
            curve.startNewSubPath(x, y);
            started = true;
        }
        else
        {
            curve.lineTo(x, y);
        }
    }

    g.setColour(juce::Colour::fromRGB(238, 241, 244));
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        const auto p = nodePosition(i);
        const bool selected = i == selectedBand;
        const bool enabled = parameter(id(i, "enabled")) > 0.5f;

        g.setColour(enabled
            ? (selected ? juce::Colour::fromRGB(90, 194, 232)
                        : juce::Colour::fromRGB(224, 230, 236))
            : juce::Colour::fromRGB(91, 96, 104));

        const float radius = selected ? 7.0f : 5.5f;
        g.fillEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colour::fromRGB(8, 10, 13));
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.drawText(juce::String(i + 1),
                   juce::roundToInt(p.x - 8.0f),
                   juce::roundToInt(p.y - 7.0f),
                   16, 14, juce::Justification::centred);
    }

    g.setColour(juce::Colour::fromRGB(91, 99, 111));
    g.drawRoundedRectangle(b, 7.0f, 1.0f);
}

void SpectrumEQDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (const int node = findNearestNode(e.position); node >= 0)
    {
        selectedBand = node;
        if (onBandSelected)
            onBandSelected(node);
        repaint();
    }
}

void SpectrumEQDisplay::mouseDrag(const juce::MouseEvent& e)
{
    const auto b = graphBounds();
    if (!b.contains(e.position))
        return;

    float frequency = xToFrequency(e.position.x);
    float gainDb = yToGain(e.position.y);

    if (e.mods.isCtrlDown())
    {
        const float currentFreq = parameter(id(selectedBand, "freq"));
        const float currentGain = parameter(id(selectedBand, "gain"));
        frequency = currentFreq + (frequency - currentFreq) * 0.12f;
        gainDb = currentGain + (gainDb - currentGain) * 0.12f;
    }

    setParameter(id(selectedBand, "freq"), frequency);
    setParameter(id(selectedBand, "gain"), gainDb);
    repaint();
}

void SpectrumEQDisplay::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const int node = findNearestNode(e.position); node >= 0)
    {
        const auto enabledId = id(node, "enabled");
        setParameter(enabledId, parameter(enabledId) > 0.5f ? 0.0f : 1.0f);
        repaint();
    }
}

PRISMVSTAudioProcessorEditor::PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      spectrumDisplay(p)
{
    setSize(1180, 720);
    setResizable(true, true);
    setResizeLimits(1000, 640, 1600, 980);

    addAndMakeVisible(spectrumDisplay);
    spectrumDisplay.onBandSelected = [this](int band) { bindSelectedBand(band); };

    selectedBandLabel.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    selectedBandLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(selectedBandLabel);

    const std::array<PrecisionSlider*, 8> sliders {
        &frequency, &gain, &q, &dynamicRange, &threshold, &ratio, &attack, &release
    };

    for (size_t i = 0; i < sliders.size(); ++i)
    {
        configureRotary(*sliders[i]);
        addAndMakeVisible(*sliders[i]);

        controlLabels[i].setText(kControlNames[i], juce::dontSendNotification);
        controlLabels[i].setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        controlLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(controlLabels[i]);
    }

    frequency.setTextValueSuffix(" Hz");
    gain.setTextValueSuffix(" dB");
    dynamicRange.setTextValueSuffix(" dB");
    threshold.setTextValueSuffix(" dB");
    ratio.setTextValueSuffix(":1");
    attack.setTextValueSuffix(" ms");
    release.setTextValueSuffix(" ms");

    for (auto* s : { &onyx, &onyxDrive, &masterTrim, &ceiling })
    {
        configureRotary(*s);
        addAndMakeVisible(*s);
    }

    onyxDrive.setTextValueSuffix(" dB");
    masterTrim.setTextValueSuffix(" dB");
    ceiling.setTextValueSuffix(" dB");

    onyxLabel.setText("ONYX", juce::dontSendNotification);
    driveLabel.setText("DRIVE", juce::dontSendNotification);
    trimLabel.setText("OUT", juce::dontSendNotification);
    ceilingLabel.setText("CEILING", juce::dontSendNotification);

    for (auto* l : { &onyxLabel, &driveLabel, &trimLabel, &ceilingLabel })
    {
        l->setJustificationType(juce::Justification::centred);
        l->setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        addAndMakeVisible(*l);
    }

    addAndMakeVisible(solfeggio);
    addAndMakeVisible(masterBypass);

    for (auto* l : { &peakLabel, &lufsShortLabel, &lufsIntLabel })
    {
        l->setJustificationType(juce::Justification::centredLeft);
        l->setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        addAndMakeVisible(*l);
    }

    onyxA = std::make_unique<SliderAttachment>(processor.apvts, "onyx", onyx);
    onyxDriveA = std::make_unique<SliderAttachment>(processor.apvts, "onyx_drive", onyxDrive);
    masterTrimA = std::make_unique<SliderAttachment>(processor.apvts, "master_trim", masterTrim);
    ceilingA = std::make_unique<SliderAttachment>(processor.apvts, "ceiling", ceiling);
    solfeggioA = std::make_unique<ButtonAttachment>(processor.apvts, "solfeggio_grid", solfeggio);
    masterBypassA = std::make_unique<ButtonAttachment>(processor.apvts, "master_bypass", masterBypass);

    bindSelectedBand(0);
    startTimerHz(30);
}

void PRISMVSTAudioProcessorEditor::configureRotary(juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 20);
    s.setTextValueSuffix(suffix);
}

void PRISMVSTAudioProcessorEditor::bindSelectedBand(int band)
{
    selectedBand = juce::jlimit(0, PRISMVSTAudioProcessor::numEqBands - 1, band);
    spectrumDisplay.setSelectedBand(selectedBand);

    const auto prefix = "band" + juce::String(selectedBand + 1) + "_";

    frequencyA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "freq", frequency);
    gainA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "gain", gain);
    qA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "q", q);
    dynamicRangeA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "dyn_range", dynamicRange);
    thresholdA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "threshold", threshold);
    ratioA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "ratio", ratio);
    attackA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "attack", attack);
    releaseA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "release", release);

    selectedBandLabel.setText(
        "NODE " + juce::String(selectedBand + 1)
        + "   •   CTRL+DRAG = FINE   •   DOUBLE-CLICK NODE = ENABLE/BYPASS",
        juce::dontSendNotification);
}

void PRISMVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(8, 10, 13));

    g.setColour(juce::Colour::fromRGB(238, 241, 244));
    g.setFont(juce::Font(juce::FontOptions(24.0f, juce::Font::bold)));
    g.drawText("PRISM", 20, 12, 160, 34, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(116, 126, 139));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("PARAMETRIC DYNAMICS • SPECTRAL HEAT • ONYX",
               155, 19, 430, 22, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(61, 68, 78));
    g.drawLine(20.0f, 50.0f, (float)getWidth() - 20.0f, 50.0f, 1.0f);

    g.setColour(juce::Colour::fromRGB(28, 32, 38));
    g.fillRoundedRectangle(18.0f, (float)getHeight() - 198.0f,
                           (float)getWidth() - 36.0f, 178.0f, 8.0f);
}

void PRISMVSTAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    const int masterW = 205;
    const int graphX = 18;
    const int graphY = 62;
    const int graphW = w - masterW - 36;
    const int graphH = h - 292;

    spectrumDisplay.setBounds(graphX, graphY, graphW, graphH);

    const int bottomY = graphY + graphH + 10;
    selectedBandLabel.setBounds(28, bottomY, graphW - 20, 24);

    const int controlsY = bottomY + 26;
    const int usable = graphW - 20;
    const int cellW = usable / 8;

    const std::array<PrecisionSlider*, 8> sliders {
        &frequency, &gain, &q, &dynamicRange, &threshold, &ratio, &attack, &release
    };

    for (int i = 0; i < 8; ++i)
    {
        const int x = 26 + i * cellW;
        controlLabels[(size_t)i].setBounds(x, controlsY, cellW - 4, 18);
        sliders[(size_t)i]->setBounds(x, controlsY + 16, cellW - 4, 105);
    }

    const int mx = w - masterW + 8;
    const int knobW = 84;

    onyxLabel.setBounds(mx, 84, knobW, 18);
    onyx.setBounds(mx, 100, knobW, 104);

    driveLabel.setBounds(mx + 92, 84, knobW, 18);
    onyxDrive.setBounds(mx + 92, 100, knobW, 104);

    solfeggio.setBounds(mx, 220, 176, 28);

    trimLabel.setBounds(mx, 270, knobW, 18);
    masterTrim.setBounds(mx, 286, knobW, 104);

    ceilingLabel.setBounds(mx + 92, 270, knobW, 18);
    ceiling.setBounds(mx + 92, 286, knobW, 104);

    peakLabel.setBounds(mx, 420, 176, 22);
    lufsShortLabel.setBounds(mx, 446, 176, 22);
    lufsIntLabel.setBounds(mx, 472, 176, 22);

    masterBypass.setBounds(mx, 510, 176, 32);
}

void PRISMVSTAudioProcessorEditor::timerCallback()
{
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> spectrum {};
    processor.copySpectrum(spectrum);
    spectrumDisplay.pushSpectrum(spectrum);

    peakLabel.setText("PEAK   " + juce::String(processor.getPeakDb(), 1) + " dBFS",
                      juce::dontSendNotification);
    lufsShortLabel.setText("LUFS-S " + juce::String(processor.getLufsShort(), 1),
                           juce::dontSendNotification);
    lufsIntLabel.setText("LUFS-I " + juce::String(processor.getLufsIntegrated(), 1),
                         juce::dontSendNotification);
}
