#include "PluginEditor.h"
#include <cmath>
#include <limits>

namespace
{
constexpr std::array<const char*, 6> kControlNames {
    "CENTER", "TRIM", "WIDTH", "DEPTH", "ATTACK", "RELEASE"
};

constexpr std::array<const char*, PRISMVSTAudioProcessor::numEqBands> kDomainNames {
    "SUB", "KICK", "LOW", "MID", "HIGH"
};

constexpr std::array<float, 9> kSolfeggio {
    174.0f, 285.0f, 396.0f, 417.0f, 528.0f, 639.0f, 741.0f, 852.0f, 963.0f
};

const std::array<juce::Colour, 9> kSolfeggioColours {
    juce::Colour::fromRGB(170, 38, 52),    // 174 deep red
    juce::Colour::fromRGB(220, 91, 34),    // 285 orange
    juce::Colour::fromRGB(221, 156, 35),   // 396 amber / gold
    juce::Colour::fromRGB(176, 196, 51),   // 417 yellow-green
    juce::Colour::fromRGB(43, 181, 105),   // 528 emerald
    juce::Colour::fromRGB(38, 180, 177),   // 639 cyan / teal
    juce::Colour::fromRGB(55, 115, 207),   // 741 blue
    juce::Colour::fromRGB(84, 71, 180),    // 852 indigo
    juce::Colour::fromRGB(181, 64, 177)    // 963 violet / magenta
};

float readParameter(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    if (auto* value = state.getRawParameterValue(id))
        return value->load();
    return 0.0f;
}

juce::Colour panelColour()
{
    return juce::Colour::fromRGB(14, 17, 21);
}

juce::Colour lineColour()
{
    return juce::Colour::fromRGB(61, 68, 78);
}

juce::Colour textMuted()
{
    return juce::Colour::fromRGB(132, 142, 154);
}
}

//==============================================================================
// Slow, repeatable, hardware-like control movement. Cursor velocity never
// changes sensitivity. Global precision mode can only make the control slower.
void PrecisionSlider::mouseDown(const juce::MouseEvent& e)
{
    dragStartProportion = valueToProportionOfLength(getValue());
    dragStartY = e.getScreenY();
    juce::Slider::mouseDown(e);
}

void PrecisionSlider::mouseDrag(const juce::MouseEvent& e)
{
    int mode = precisionMode;
    if (e.mods.isShiftDown())
        mode = juce::jmax(mode, 1);
    if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        mode = 2;

    const double pixelsForFullRange = mode == 0 ? normalPixelsForFullRange
                                                : (mode == 1 ? finePixelsForFullRange
                                                             : microPixelsForFullRange);

    const double deltaPixels = static_cast<double>(dragStartY - e.getScreenY());
    const double proportion = juce::jlimit(0.0, 1.0,
                                           dragStartProportion + deltaPixels / pixelsForFullRange);

    setValue(proportionOfLengthToValue(proportion), juce::sendNotificationSync);
}

void PrecisionSlider::mouseUp(const juce::MouseEvent& e)
{
    juce::Slider::mouseUp(e);
}

//==============================================================================
void EtherTechLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                             int x, int y, int width, int height,
                                             float sliderPosProportional,
                                             float rotaryStartAngle,
                                             float rotaryEndAngle,
                                             juce::Slider&)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height)
                            .reduced(8.0f, 6.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto knob = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());
    knob = knob.reduced(4.0f);

    const float angle = juce::jmap(sliderPosProportional, 0.0f, 1.0f,
                                   rotaryStartAngle, rotaryEndAngle);

    g.setColour(juce::Colour::fromRGB(7, 9, 12));
    g.fillEllipse(knob);

    g.setColour(juce::Colour::fromRGB(46, 51, 58));
    g.drawEllipse(knob, 2.0f);

    auto arcBounds = knob.expanded(3.0f);
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(arcBounds.getCentreX(), arcBounds.getCentreY(),
                                arcBounds.getWidth() * 0.5f, arcBounds.getHeight() * 0.5f,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour::fromRGB(54, 61, 70));
    g.strokePath(backgroundArc, juce::PathStrokeType(2.2f));

    juce::Path valueArc;
    valueArc.addCentredArc(arcBounds.getCentreX(), arcBounds.getCentreY(),
                           arcBounds.getWidth() * 0.5f, arcBounds.getHeight() * 0.5f,
                           0.0f, rotaryStartAngle, angle, true);
    g.setColour(juce::Colour::fromRGB(205, 213, 220));
    g.strokePath(valueArc, juce::PathStrokeType(2.4f));

    juce::Path pointer;
    const float pointerLength = knob.getHeight() * 0.31f;
    const float pointerThickness = 2.1f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f,
                                -knob.getHeight() * 0.34f,
                                pointerThickness, pointerLength, 1.0f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle)
                               .translated(knob.getCentreX(), knob.getCentreY()));
    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.fillPath(pointer);

    const float cap = knob.getWidth() * 0.12f;
    g.setColour(juce::Colour::fromRGB(92, 101, 112));
    g.fillEllipse(knob.getCentreX() - cap * 0.5f,
                  knob.getCentreY() - cap * 0.5f, cap, cap);
}

//==============================================================================
// Stationary Heat Aura analyzer. No waterfall / side-scrolling history.
SpectrumAuraDisplay::SpectrumAuraDisplay(PRISMVSTAudioProcessor& p)
    : processor(p)
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
    setInterceptsMouseClicks(true, true);
    latestSpectrum.fill(-144.0f);
    auraEnergy.fill(0.0f);
}

float SpectrumAuraDisplay::parameter(const juce::String& parameterId) const
{
    return readParameter(processor.apvts, parameterId);
}

juce::Rectangle<float> SpectrumAuraDisplay::graphBounds() const
{
    return getLocalBounds().toFloat().reduced(20.0f, 18.0f);
}

float SpectrumAuraDisplay::frequencyToX(float frequency) const
{
    const auto b = graphBounds();
    const float clamped = juce::jlimit(20.0f, 20000.0f, frequency);
    const float norm = std::log10(clamped / 20.0f) / std::log10(1000.0f);
    return b.getX() + norm * b.getWidth();
}

float SpectrumAuraDisplay::xToFrequency(float x) const
{
    const auto b = graphBounds();
    const float norm = juce::jlimit(0.0f, 1.0f, (x - b.getX()) / b.getWidth());
    return 20.0f * std::pow(1000.0f, norm);
}

float SpectrumAuraDisplay::displayFloorDb() const
{
    const int mode = juce::roundToInt(parameter("analyzer_depth"));
    if (mode == 1) return -72.0f;
    if (mode >= 2) return -120.0f;
    return -36.0f;
}

float SpectrumAuraDisplay::levelToY(float db) const
{
    const auto b = graphBounds();
    const float floor = displayFloorDb();
    return juce::jmap(juce::jlimit(floor, 0.0f, db),
                      floor, 0.0f, b.getBottom(), b.getY());
}

float SpectrumAuraDisplay::spectrumDbAt(float frequency) const
{
    const double sr = processor.getSampleRate() > 1.0 ? processor.getSampleRate() : 48000.0;
    const double nyquist = sr * 0.5;
    const int bin = juce::jlimit(
        0, PRISMVSTAudioProcessor::spectrumBins - 1,
        juce::roundToInt((float)(frequency / nyquist)
                         * (PRISMVSTAudioProcessor::spectrumBins - 1)));
    return latestSpectrum[(size_t)bin];
}

float SpectrumAuraDisplay::displayedSpectrumDbAt(float frequency) const
{
    const float raw = spectrumDbAt(frequency);
    const float slope = parameter("analyzer_slope");
    const float octavesFrom1k = std::log2(juce::jmax(20.0f, frequency) / 1000.0f);
    return raw + slope * octavesFrom1k;
}

float SpectrumAuraDisplay::domainBumperFrequency(int leftDomain) const
{
    if (leftDomain < 0 || leftDomain >= PRISMVSTAudioProcessor::numEqBands - 1)
        return 20000.0f;

    const auto idA = "band" + juce::String(leftDomain + 1) + "_freq";
    const auto idB = "band" + juce::String(leftDomain + 2) + "_freq";
    const float a = juce::jmax(20.0f, parameter(idA));
    const float b = juce::jmax(a + 1.0f, parameter(idB));
    return std::sqrt(a * b);
}

int SpectrumAuraDisplay::domainForFrequency(float frequency) const
{
    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands - 1; ++i)
        if (frequency < domainBumperFrequency(i))
            return i;
    return PRISMVSTAudioProcessor::numEqBands - 1;
}

juce::Colour SpectrumAuraDisplay::auraColourFor(float frequency,
                                                 float& affinity,
                                                 float& referenceHz) const
{
    const float strictness = juce::jlimit(0.0f, 1.0f, parameter("color_affinity"));

    int nearest = 0;
    float nearestDistance = std::numeric_limits<float>::max();
    for (int i = 0; i < (int)kSolfeggio.size(); ++i)
    {
        const float d = std::abs(std::log2(juce::jmax(1.0f, frequency) / kSolfeggio[(size_t)i]));
        if (d < nearestDistance)
        {
            nearestDistance = d;
            nearest = i;
        }
    }

    referenceHz = kSolfeggio[(size_t)nearest];
    const float sigmaOctaves = juce::jmap(strictness, 0.0f, 1.0f, 0.48f, 0.10f);
    affinity = std::exp(-0.5f * (nearestDistance * nearestDistance)
                        / (sigmaOctaves * sigmaOctaves));

    if (frequency <= kSolfeggio.front())
        return kSolfeggioColours.front();
    if (frequency >= kSolfeggio.back())
        return kSolfeggioColours.back();

    for (int i = 0; i < (int)kSolfeggio.size() - 1; ++i)
    {
        const float a = kSolfeggio[(size_t)i];
        const float b = kSolfeggio[(size_t)i + 1];
        if (frequency >= a && frequency <= b)
        {
            const float t = (frequency - a) / (b - a);
            return kSolfeggioColours[(size_t)i]
                .interpolatedWith(kSolfeggioColours[(size_t)i + 1], t);
        }
    }

    return kSolfeggioColours[(size_t)nearest];
}

void SpectrumAuraDisplay::pushSpectrum(
    const std::array<float, PRISMVSTAudioProcessor::spectrumBins>& values)
{
    latestSpectrum = values;

    const float memorySeconds = juce::jlimit(0.05f, 10.0f, parameter("aura_memory"));
    const float decay = std::exp(-1.0f / (30.0f * memorySeconds));
    const float floor = displayFloorDb();

    for (int i = 0; i < auraColumns; ++i)
    {
        const float norm = (float)i / (float)(auraColumns - 1);
        const float freq = 20.0f * std::pow(1000.0f, norm);
        const float db = juce::jlimit(floor, 0.0f, displayedSpectrumDbAt(freq));
        const float instant = juce::jlimit(0.0f, 1.0f, (db - floor) / -floor);

        // Fast rise + slow stationary persistence. Recurring energy therefore
        // develops a hot zone instead of scrolling away.
        auraEnergy[(size_t)i] = juce::jmax(instant,
                                           auraEnergy[(size_t)i] * decay);
    }

    repaint();
}

void SpectrumAuraDisplay::setSelectedBand(int band)
{
    selectedBand = juce::jlimit(0, PRISMVSTAudioProcessor::numEqBands - 1, band);
    repaint();
}

void SpectrumAuraDisplay::paint(juce::Graphics& g)
{
    const auto b = graphBounds();
    const float floor = displayFloorDb();

    g.setColour(panelColour());
    g.fillRoundedRectangle(b, 8.0f);

    // Selected semantic territory.
    const float leftFreq = selectedBand == 0 ? 20.0f : domainBumperFrequency(selectedBand - 1);
    const float rightFreq = selectedBand == PRISMVSTAudioProcessor::numEqBands - 1
                                ? 20000.0f : domainBumperFrequency(selectedBand);
    const float leftX = frequencyToX(leftFreq);
    const float rightX = frequencyToX(rightFreq);
    g.setColour(juce::Colour::fromRGB(226, 232, 238).withAlpha(0.035f));
    g.fillRect(juce::Rectangle<float>(leftX, b.getY(),
                                      juce::jmax(0.0f, rightX - leftX), b.getHeight()));

    const std::array<float, 10> gridFreq {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    for (float f : gridFreq)
    {
        const float x = frequencyToX(f);
        g.setColour(lineColour().withAlpha(0.62f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

        g.setColour(textMuted());
        const juce::String label = f >= 1000.0f
            ? juce::String(f / 1000.0f, f >= 10000.0f ? 0 : 1) + "k"
            : juce::String((int)f);

        g.drawText(label, juce::roundToInt(x - 24.0f),
                   juce::roundToInt(b.getBottom() - 16.0f),
                   48, 14, juce::Justification::centred);
    }

    const int divisions = floor <= -100.0f ? 6 : (floor <= -70.0f ? 6 : 6);
    for (int i = 0; i <= divisions; ++i)
    {
        const float db = floor + (-floor * ((float)i / (float)divisions));
        const float y = levelToY(db);
        g.setColour(lineColour().withAlpha(i == divisions ? 0.88f : 0.38f));
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());

        g.setColour(textMuted());
        g.drawText(juce::String(db, 0),
                   juce::roundToInt(b.getX() + 4.0f),
                   juce::roundToInt(y - 8.0f),
                   38, 14, juce::Justification::centredLeft);
    }

    // Solfeggio reference colour canon: fixed ticks, smooth aura interpolation.
    if (parameter("solfeggio_grid") > 0.5f)
    {
        for (int i = 0; i < (int)kSolfeggio.size(); ++i)
        {
            const float x = frequencyToX(kSolfeggio[(size_t)i]);
            g.setColour(kSolfeggioColours[(size_t)i].withAlpha(0.82f));
            g.fillRect(juce::Rectangle<float>(x - 1.0f, b.getY() + 2.0f, 2.0f, 8.0f));
        }
    }

    // Stationary Heat Aura.
    for (int i = 0; i < auraColumns; ++i)
    {
        const float energy = auraEnergy[(size_t)i];
        if (energy < 0.015f)
            continue;

        const float norm = (float)i / (float)(auraColumns - 1);
        const float freq = 20.0f * std::pow(1000.0f, norm);
        const float db = juce::jlimit(floor, 0.0f, displayedSpectrumDbAt(freq));
        const float x = frequencyToX(freq);
        const float y = levelToY(db);

        float affinity = 0.0f;
        float ref = 0.0f;
        auto c = parameter("solfeggio_grid") > 0.5f
            ? auraColourFor(freq, affinity, ref)
            : juce::Colour::fromRGB(70, 169, 205);

        const float alpha = juce::jlimit(0.0f, 0.42f,
                                         0.035f + energy * 0.27f
                                         * (0.45f + 0.55f * affinity));
        const float width = 8.0f + energy * 18.0f;
        const float height = 8.0f + energy * 48.0f;

        g.setColour(c.withAlpha(alpha * 0.35f));
        g.fillEllipse(x - width, y - height, width * 2.0f, height * 2.0f);

        g.setColour(c.withAlpha(alpha));
        g.fillEllipse(x - width * 0.38f, y - height * 0.38f,
                      width * 0.76f, height * 0.76f);
    }

    // Actual spectrum trace remains a measurement layer separate from aura.
    juce::Path spectrumPath;
    bool started = false;
    for (int px = 0; px <= juce::roundToInt(b.getWidth()); px += 2)
    {
        const float x = b.getX() + (float)px;
        const float freq = xToFrequency(x);
        const float db = juce::jlimit(floor, 0.0f, displayedSpectrumDbAt(freq));
        const float y = levelToY(db);

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

    g.setColour(juce::Colour::fromRGB(224, 231, 236).withAlpha(0.72f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.2f));

    // Semantic bumpers / guardrails. They are not EQ nodes.
    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands - 1; ++i)
    {
        const float f = domainBumperFrequency(i);
        const float x = frequencyToX(f);
        const float localDb = juce::jlimit(floor, 0.0f, displayedSpectrumDbAt(f));
        const float pressure = juce::jlimit(0.0f, 1.0f, (localDb - floor) / -floor);

        g.setColour(juce::Colour::fromRGB(194, 201, 208)
                        .withAlpha(0.18f + pressure * 0.44f));
        g.drawLine(x, b.getY() + 12.0f, x, b.getBottom() - 18.0f,
                   1.0f + pressure * 1.4f);
    }

    // Domain names stay subtle.
    for (int d = 0; d < PRISMVSTAudioProcessor::numEqBands; ++d)
    {
        const float lo = d == 0 ? 20.0f : domainBumperFrequency(d - 1);
        const float hi = d == PRISMVSTAudioProcessor::numEqBands - 1
                           ? 20000.0f : domainBumperFrequency(d);
        const float cx = (frequencyToX(lo) + frequencyToX(hi)) * 0.5f;
        g.setColour(juce::Colour::fromRGB(170, 178, 188)
                        .withAlpha(d == selectedBand ? 0.78f : 0.40f));
        g.setFont(juce::Font(juce::FontOptions(10.0f,
                    d == selectedBand ? juce::Font::bold : juce::Font::plain)));
        g.drawText(kDomainNames[(size_t)d],
                   juce::roundToInt(cx - 30.0f), juce::roundToInt(b.getY() + 12.0f),
                   60, 14, juce::Justification::centred);
    }

    if (hasHover && b.contains(hoverPoint))
    {
        const float f = xToFrequency(hoverPoint.x);
        const float rawDb = spectrumDbAt(f);
        const float wavelength = 343.0f / juce::jmax(1.0f, f);

        float affinity = 0.0f;
        float ref = 0.0f;
        const auto aura = auraColourFor(f, affinity, ref);

        juce::String line1 = juce::String(f, f < 1000.0f ? 1 : 0) + " Hz   "
                           + juce::String(rawDb, 1) + " dBFS";
        juce::String line2 = "lambda " + juce::String(wavelength, wavelength < 1.0f ? 2 : 1) + " m   "
                           + juce::String((int)ref) + " aura   "
                           + juce::String(affinity * 100.0f, 0) + "%";

        const float boxW = 218.0f;
        const float boxH = 42.0f;
        float bx = juce::jlimit(b.getX(), b.getRight() - boxW, hoverPoint.x + 12.0f);
        float by = juce::jlimit(b.getY(), b.getBottom() - boxH, hoverPoint.y - 50.0f);

        g.setColour(juce::Colour::fromRGB(6, 8, 11).withAlpha(0.94f));
        g.fillRoundedRectangle(bx, by, boxW, boxH, 5.0f);
        g.setColour(aura.withAlpha(0.82f));
        g.drawRoundedRectangle(bx, by, boxW, boxH, 5.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.93f));
        g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        g.drawText(line1, juce::roundToInt(bx + 8.0f), juce::roundToInt(by + 5.0f),
                   (int)boxW - 16, 14, juce::Justification::centredLeft);
        g.setColour(textMuted());
        g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.drawText(line2, juce::roundToInt(bx + 8.0f), juce::roundToInt(by + 22.0f),
                   (int)boxW - 16, 14, juce::Justification::centredLeft);
    }

    g.setColour(juce::Colour::fromRGB(85, 94, 105));
    g.drawRoundedRectangle(b, 8.0f, 1.0f);
}

void SpectrumAuraDisplay::mouseDown(const juce::MouseEvent& e)
{
    if (!graphBounds().contains(e.position))
        return;

    const int domain = domainForFrequency(xToFrequency(e.position.x));
    setSelectedBand(domain);
    if (onBandSelected)
        onBandSelected(domain);
}

void SpectrumAuraDisplay::mouseMove(const juce::MouseEvent& e)
{
    hasHover = graphBounds().contains(e.position);
    hoverPoint = e.position;
    repaint();
}

void SpectrumAuraDisplay::mouseExit(const juce::MouseEvent&)
{
    hasHover = false;
    repaint();
}

//==============================================================================
// Maximus-derived input -> output transfer map. Threshold and ratio are geometry,
// not duplicate front-panel knobs.
DynamicsTransferDisplay::DynamicsTransferDisplay(PRISMVSTAudioProcessor& p)
    : processor(p)
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> DynamicsTransferDisplay::graphBounds() const
{
    return getLocalBounds().toFloat().reduced(18.0f, 18.0f);
}

juce::String DynamicsTransferDisplay::id(const juce::String& suffix) const
{
    return "band" + juce::String(selectedBand + 1) + "_" + suffix;
}

float DynamicsTransferDisplay::parameter(const juce::String& suffix) const
{
    return readParameter(processor.apvts, id(suffix));
}

juce::RangedAudioParameter* DynamicsTransferDisplay::rangedParameter(
    const juce::String& suffix) const
{
    return processor.apvts.getParameter(id(suffix));
}

void DynamicsTransferDisplay::setParameter(const juce::String& suffix, float value)
{
    if (auto* p = rangedParameter(suffix))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}

float DynamicsTransferDisplay::dbToX(float db) const
{
    const auto b = graphBounds();
    return juce::jmap(juce::jlimit(-60.0f, 0.0f, db),
                      -60.0f, 0.0f, b.getX(), b.getRight());
}

float DynamicsTransferDisplay::dbToY(float db) const
{
    const auto b = graphBounds();
    return juce::jmap(juce::jlimit(-60.0f, 0.0f, db),
                      -60.0f, 0.0f, b.getBottom(), b.getY());
}

float DynamicsTransferDisplay::xToDb(float x) const
{
    const auto b = graphBounds();
    return juce::jmap(juce::jlimit(b.getX(), b.getRight(), x),
                      b.getX(), b.getRight(), -60.0f, 0.0f);
}

float DynamicsTransferDisplay::yToDb(float y) const
{
    const auto b = graphBounds();
    return juce::jmap(juce::jlimit(b.getY(), b.getBottom(), y),
                      b.getBottom(), b.getY(), -60.0f, 0.0f);
}

juce::Point<float> DynamicsTransferDisplay::thresholdPoint() const
{
    const float t = parameter("threshold");
    return { dbToX(t), dbToY(t) };
}

juce::Point<float> DynamicsTransferDisplay::ratioPoint() const
{
    const float threshold = parameter("threshold");
    const float ratio = juce::jmax(1.0f, parameter("ratio"));
    const float maxReduction = parameter("dyn_range");
    const float input = juce::jlimit(threshold + 6.0f, 0.0f, threshold + 18.0f);
    const float compressed = threshold + (input - threshold) / ratio;
    const float output = juce::jmax(input - maxReduction, compressed);
    return { dbToX(input), dbToY(output) };
}

float DynamicsTransferDisplay::dragScale(const juce::ModifierKeys& mods) const
{
    int mode = juce::roundToInt(readParameter(processor.apvts, "precision_mode"));
    if (mods.isShiftDown())
        mode = juce::jmax(mode, 1);
    if (mods.isCtrlDown() || mods.isCommandDown())
        mode = 2;
    return mode == 0 ? 700.0f : (mode == 1 ? 3000.0f : 6000.0f);
}

void DynamicsTransferDisplay::setSelectedBand(int band)
{
    selectedBand = juce::jlimit(0, PRISMVSTAudioProcessor::numEqBands - 1, band);
    repaint();
}

void DynamicsTransferDisplay::paint(juce::Graphics& g)
{
    const auto b = graphBounds();
    g.setColour(panelColour());
    g.fillRoundedRectangle(b, 7.0f);

    for (int db : { -60, -48, -36, -24, -12, 0 })
    {
        const float x = dbToX((float)db);
        const float y = dbToY((float)db);
        g.setColour(lineColour().withAlpha(0.36f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());
    }

    g.setColour(juce::Colour::fromRGB(102, 112, 124).withAlpha(0.58f));
    g.drawLine(b.getX(), b.getBottom(), b.getRight(), b.getY(), 1.0f);

    const float threshold = parameter("threshold");
    const float ratio = juce::jmax(1.0f, parameter("ratio"));
    const float maxReduction = parameter("dyn_range");

    juce::Path curve;
    bool started = false;
    for (int px = 0; px <= juce::roundToInt(b.getWidth()); px += 2)
    {
        const float input = xToDb(b.getX() + (float)px);
        float output = input;

        if (input > threshold)
        {
            const float compressed = threshold + (input - threshold) / ratio;
            output = juce::jmax(input - maxReduction, compressed);
        }

        const float x = dbToX(input);
        const float y = dbToY(output);
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

    g.setColour(juce::Colour::fromRGB(229, 234, 239));
    g.strokePath(curve, juce::PathStrokeType(2.2f));

    const auto tp = thresholdPoint();
    const auto rp = ratioPoint();

    g.setColour(juce::Colour::fromRGB(84, 183, 217));
    g.fillEllipse(tp.x - 6.0f, tp.y - 6.0f, 12.0f, 12.0f);

    g.setColour(juce::Colour::fromRGB(215, 221, 228));
    g.fillEllipse(rp.x - 5.5f, rp.y - 5.5f, 11.0f, 11.0f);

    g.setColour(textMuted());
    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    g.drawText("INPUT dB", juce::roundToInt(b.getRight() - 72.0f),
               juce::roundToInt(b.getBottom() - 16.0f),
               68, 14, juce::Justification::centredRight);
    g.drawText("OUTPUT", juce::roundToInt(b.getX() + 4.0f),
               juce::roundToInt(b.getY() + 4.0f),
               62, 14, juce::Justification::centredLeft);

    g.drawText("THR " + juce::String(threshold, 1) + " dB   R "
                   + juce::String(ratio, 2) + ":1   MAX "
                   + juce::String(maxReduction, 1) + " dB",
               juce::roundToInt(b.getX() + 72.0f),
               juce::roundToInt(b.getY() + 4.0f),
               juce::roundToInt(b.getWidth() - 148.0f), 14,
               juce::Justification::centred);

    g.setColour(juce::Colour::fromRGB(85, 94, 105));
    g.drawRoundedRectangle(b, 7.0f, 1.0f);
}

void DynamicsTransferDisplay::mouseDown(const juce::MouseEvent& e)
{
    const float dThreshold = e.position.getDistanceFrom(thresholdPoint());
    const float dRatio = e.position.getDistanceFrom(ratioPoint());

    if (dThreshold <= 18.0f || dThreshold <= dRatio)
    {
        dragTarget = DragTarget::threshold;
        dragStartValue = parameter("threshold");
        dragStartPixel = e.position.x;
        if (auto* p = rangedParameter("threshold"))
            p->beginChangeGesture();
    }
    else
    {
        dragTarget = DragTarget::ratio;
        dragStartValue = parameter("ratio");
        dragStartPixel = e.position.y;
        if (auto* p = rangedParameter("ratio"))
            p->beginChangeGesture();
    }
}

void DynamicsTransferDisplay::mouseDrag(const juce::MouseEvent& e)
{
    const float pixels = dragScale(e.mods);

    if (dragTarget == DragTarget::threshold)
    {
        const float delta = (e.position.x - dragStartPixel) * 60.0f / pixels;
        setParameter("threshold", juce::jlimit(-60.0f, 0.0f, dragStartValue + delta));
    }
    else if (dragTarget == DragTarget::ratio)
    {
        const float delta = (dragStartPixel - e.position.y) * 19.0f / pixels;
        setParameter("ratio", juce::jlimit(1.0f, 20.0f, dragStartValue + delta));
    }

    repaint();
}

void DynamicsTransferDisplay::mouseUp(const juce::MouseEvent&)
{
    if (dragTarget == DragTarget::threshold)
    {
        if (auto* p = rangedParameter("threshold"))
            p->endChangeGesture();
    }
    else if (dragTarget == DragTarget::ratio)
    {
        if (auto* p = rangedParameter("ratio"))
            p->endChangeGesture();
    }

    dragTarget = DragTarget::none;
}

void DynamicsTransferDisplay::mouseDoubleClick(const juce::MouseEvent& e)
{
    const float dThreshold = e.position.getDistanceFrom(thresholdPoint());
    const float dRatio = e.position.getDistanceFrom(ratioPoint());

    if (dThreshold <= dRatio)
        setParameter("threshold", -18.0f);
    else
        setParameter("ratio", 2.0f);

    repaint();
}

//==============================================================================
PRISMVSTAudioProcessorEditor::PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      spectrumDisplay(p),
      transferDisplay(p)
{
    setSize(1240, 820);
    setResizable(true, true);
    setResizeLimits(1080, 720, 1680, 1080);

    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(spectrumDisplay);
    addAndMakeVisible(transferDisplay);

    spectrumDisplay.onBandSelected = [this](int band) { bindSelectedBand(band); };

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        auto& button = domainButtons[(size_t)i];
        button.setButtonText(kDomainNames[(size_t)i]);
        button.setClickingTogglesState(true);
        button.setRadioGroupId(101);
        button.onClick = [this, i] { bindSelectedBand(i); };
        addAndMakeVisible(button);
    }

    selectedBandLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    selectedBandLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(selectedBandLabel);

    const std::array<PrecisionSlider*, 6> domainSliders {
        &frequency, &gain, &q, &dynamicRange, &attack, &release
    };

    for (size_t i = 0; i < domainSliders.size(); ++i)
    {
        configureRotary(*domainSliders[i]);
        addAndMakeVisible(*domainSliders[i]);

        controlLabels[i].setText(kControlNames[i], juce::dontSendNotification);
        controlLabels[i].setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
        controlLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(controlLabels[i]);
    }

    frequency.setTextValueSuffix(" Hz");
    gain.setTextValueSuffix(" dB");
    dynamicRange.setTextValueSuffix(" dB");
    attack.setTextValueSuffix(" ms");
    release.setTextValueSuffix(" ms");

    for (auto* s : { &onyx, &onyxDrive, &masterTrim, &ceiling,
                     &analyzerSlope, &auraMemory, &colorAffinity })
    {
        configureRotary(*s);
        addAndMakeVisible(*s);
    }

    onyxDrive.setTextValueSuffix(" dB");
    masterTrim.setTextValueSuffix(" dB");
    ceiling.setTextValueSuffix(" dBTP");
    analyzerSlope.setTextValueSuffix(" dB/oct");
    auraMemory.setTextValueSuffix(" s");

    colorAffinity.textFromValueFunction = [] (double value)
    {
        return juce::String(value * 100.0, 0) + " %";
    };

    onyxLabel.setText("ONYX", juce::dontSendNotification);
    driveLabel.setText("DRIVE", juce::dontSendNotification);
    trimLabel.setText("OUT", juce::dontSendNotification);
    ceilingLabel.setText("CEILING", juce::dontSendNotification);
    analyzerSlopeLabel.setText("SLOPE", juce::dontSendNotification);
    auraMemoryLabel.setText("AURA MEMORY", juce::dontSendNotification);
    affinityLabel.setText("COLOR AFFINITY", juce::dontSendNotification);
    depthLabel.setText("ANALYZER", juce::dontSendNotification);
    precisionLabel.setText("PRECISION", juce::dontSendNotification);

    for (auto* l : { &onyxLabel, &driveLabel, &trimLabel, &ceilingLabel,
                     &analyzerSlopeLabel, &auraMemoryLabel, &affinityLabel,
                     &depthLabel, &precisionLabel })
    {
        l->setJustificationType(juce::Justification::centred);
        l->setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        addAndMakeVisible(*l);
    }

    analyzerDepth.addItem("MIX 0 -> -36 dB", 1);
    analyzerDepth.addItem("DEEP 0 -> -72 dB", 2);
    analyzerDepth.addItem("FORENSIC 0 -> -120 dB", 3);
    addAndMakeVisible(analyzerDepth);

    precisionMode.addItem("NORMAL", 1);
    precisionMode.addItem("FINE", 2);
    precisionMode.addItem("MICRO", 3);
    addAndMakeVisible(precisionMode);

    addAndMakeVisible(solfeggio);
    addAndMakeVisible(masterBypass);

    for (auto* l : { &peakLabel, &lufsShortLabel, &lufsIntLabel })
    {
        l->setJustificationType(juce::Justification::centredLeft);
        l->setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        addAndMakeVisible(*l);
    }

    onyxA = std::make_unique<SliderAttachment>(processor.apvts, "onyx", onyx);
    onyxDriveA = std::make_unique<SliderAttachment>(processor.apvts, "onyx_drive", onyxDrive);
    masterTrimA = std::make_unique<SliderAttachment>(processor.apvts, "master_trim", masterTrim);
    ceilingA = std::make_unique<SliderAttachment>(processor.apvts, "ceiling", ceiling);

    analyzerSlopeA = std::make_unique<SliderAttachment>(processor.apvts, "analyzer_slope", analyzerSlope);
    auraMemoryA = std::make_unique<SliderAttachment>(processor.apvts, "aura_memory", auraMemory);
    colorAffinityA = std::make_unique<SliderAttachment>(processor.apvts, "color_affinity", colorAffinity);
    analyzerDepthA = std::make_unique<ComboBoxAttachment>(processor.apvts, "analyzer_depth", analyzerDepth);
    precisionModeA = std::make_unique<ComboBoxAttachment>(processor.apvts, "precision_mode", precisionMode);

    solfeggioA = std::make_unique<ButtonAttachment>(processor.apvts, "solfeggio_grid", solfeggio);
    masterBypassA = std::make_unique<ButtonAttachment>(processor.apvts, "master_bypass", masterBypass);

    enableDefaultReset(onyx, "onyx");
    enableDefaultReset(onyxDrive, "onyx_drive");
    enableDefaultReset(masterTrim, "master_trim");
    enableDefaultReset(ceiling, "ceiling");
    enableDefaultReset(analyzerSlope, "analyzer_slope");
    enableDefaultReset(auraMemory, "aura_memory");
    enableDefaultReset(colorAffinity, "color_affinity");

    auto applyPrecision = [this]
    {
        const int mode = juce::jlimit(0, 2, precisionMode.getSelectedId() - 1);
        for (auto* s : { &frequency, &gain, &q, &dynamicRange, &attack, &release,
                         &onyx, &onyxDrive, &masterTrim, &ceiling,
                         &analyzerSlope, &auraMemory, &colorAffinity })
            s->setPrecisionMode(mode);
    };

    precisionMode.onChange = applyPrecision;

    bindSelectedBand(0);
    domainButtons[0].setToggleState(true, juce::dontSendNotification);
    applyPrecision();

    startTimerHz(30);
}

PRISMVSTAudioProcessorEditor::~PRISMVSTAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void PRISMVSTAudioProcessorEditor::configureRotary(juce::Slider& s,
                                                    const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                          juce::MathConstants<float>::pi * 2.8f,
                          true);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 19);
    s.setTextValueSuffix(suffix);
    s.setVelocityBasedMode(false);
    s.setScrollWheelEnabled(true);
}

void PRISMVSTAudioProcessorEditor::enableDefaultReset(juce::Slider& slider,
                                                       const juce::String& parameterId)
{
    if (auto* p = processor.apvts.getParameter(parameterId))
        slider.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
}

void PRISMVSTAudioProcessorEditor::bindSelectedBand(int band)
{
    selectedBand = juce::jlimit(0, PRISMVSTAudioProcessor::numEqBands - 1, band);
    spectrumDisplay.setSelectedBand(selectedBand);
    transferDisplay.setSelectedBand(selectedBand);

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
        domainButtons[(size_t)i].setToggleState(i == selectedBand,
                                               juce::dontSendNotification);

    const auto prefix = "band" + juce::String(selectedBand + 1) + "_";

    frequencyA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "freq", frequency);
    gainA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "gain", gain);
    qA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "q", q);
    dynamicRangeA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "dyn_range", dynamicRange);
    attackA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "attack", attack);
    releaseA = std::make_unique<SliderAttachment>(processor.apvts, prefix + "release", release);

    enableDefaultReset(frequency, prefix + "freq");
    enableDefaultReset(gain, prefix + "gain");
    enableDefaultReset(q, prefix + "q");
    enableDefaultReset(dynamicRange, prefix + "dyn_range");
    enableDefaultReset(attack, prefix + "attack");
    enableDefaultReset(release, prefix + "release");

    selectedBandLabel.setText(
        juce::String(kDomainNames[(size_t)selectedBand])
        + " DOMAIN   |   transfer map = threshold / ratio geometry   |   "
          "SHIFT = fine   CTRL/CMD = micro",
        juce::dontSendNotification);
}

void PRISMVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(7, 9, 12));

    g.setColour(juce::Colour::fromRGB(239, 242, 245));
    g.setFont(juce::Font(juce::FontOptions(25.0f, juce::Font::bold)));
    g.drawText("PRISM", 20, 10, 150, 34, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(121, 132, 145));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("ETHERTECH  |  SPECTRAL DYNAMICS  |  STATIONARY HEAT AURA",
               150, 17, 490, 20, juce::Justification::centredLeft);

    g.setColour(lineColour());
    g.drawLine(20.0f, 50.0f, (float)getWidth() - 20.0f, 50.0f, 1.0f);

    const int railW = 238;
    g.setColour(juce::Colour::fromRGB(11, 14, 18));
    g.fillRoundedRectangle((float)getWidth() - railW - 14.0f, 62.0f,
                           (float)railW, (float)getHeight() - 78.0f, 9.0f);

    g.setColour(juce::Colour::fromRGB(38, 44, 52));
    g.drawRoundedRectangle((float)getWidth() - railW - 14.0f, 62.0f,
                           (float)railW, (float)getHeight() - 78.0f, 9.0f, 1.0f);

    g.setColour(textMuted());
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
    g.drawText("MASTER / ANALYZER", getWidth() - railW, 68, railW - 28, 18,
               juce::Justification::centred);
}

void PRISMVSTAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    const int railW = 238;
    const int left = 18;
    const int graphRight = w - railW - 30;
    const int graphW = graphRight - left;

    const int tabsY = 58;
    const int tabGap = 5;
    const int tabW = (graphW - tabGap * (PRISMVSTAudioProcessor::numEqBands - 1))
                     / PRISMVSTAudioProcessor::numEqBands;

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
        domainButtons[(size_t)i].setBounds(left + i * (tabW + tabGap), tabsY, tabW, 27);

    const int analyzerY = 92;
    const int analyzerH = juce::jmax(270, h - 430);
    spectrumDisplay.setBounds(left, analyzerY, graphW, analyzerH);

    const int transferY = analyzerY + analyzerH + 8;
    const int transferH = 154;
    transferDisplay.setBounds(left, transferY, graphW, transferH);

    const int labelY = transferY + transferH + 2;
    selectedBandLabel.setBounds(left + 8, labelY, graphW - 16, 20);

    const int controlsY = labelY + 20;
    const int controlH = juce::jmax(88, h - controlsY - 12);
    const int cellW = graphW / 6;

    const std::array<PrecisionSlider*, 6> sliders {
        &frequency, &gain, &q, &dynamicRange, &attack, &release
    };

    for (int i = 0; i < 6; ++i)
    {
        const int x = left + i * cellW;
        controlLabels[(size_t)i].setBounds(x, controlsY, cellW, 16);
        sliders[(size_t)i]->setBounds(x + 4, controlsY + 14,
                                      cellW - 8, controlH - 14);
    }

    const int rx = w - railW - 2;
    const int knobW = 98;
    const int rowGap = 108;

    onyxLabel.setBounds(rx, 92, knobW, 16);
    onyx.setBounds(rx, 106, knobW, 92);
    driveLabel.setBounds(rx + 106, 92, knobW, 16);
    onyxDrive.setBounds(rx + 106, 106, knobW, 92);

    trimLabel.setBounds(rx, 92 + rowGap, knobW, 16);
    masterTrim.setBounds(rx, 106 + rowGap, knobW, 92);
    ceilingLabel.setBounds(rx + 106, 92 + rowGap, knobW, 16);
    ceiling.setBounds(rx + 106, 106 + rowGap, knobW, 92);

    int y = 314;
    peakLabel.setBounds(rx + 6, y, railW - 24, 20); y += 23;
    lufsShortLabel.setBounds(rx + 6, y, railW - 24, 20); y += 23;
    lufsIntLabel.setBounds(rx + 6, y, railW - 24, 20); y += 34;

    depthLabel.setBounds(rx + 4, y, railW - 20, 15); y += 16;
    analyzerDepth.setBounds(rx + 12, y, railW - 36, 25); y += 33;

    precisionLabel.setBounds(rx + 4, y, railW - 20, 15); y += 16;
    precisionMode.setBounds(rx + 12, y, railW - 36, 25); y += 34;

    const int smallW = 72;
    analyzerSlopeLabel.setBounds(rx + 2, y, smallW, 15);
    auraMemoryLabel.setBounds(rx + 76, y, smallW + 8, 15);
    affinityLabel.setBounds(rx + 156, y, smallW, 15);

    analyzerSlope.setBounds(rx, y + 13, smallW + 4, 86);
    auraMemory.setBounds(rx + 77, y + 13, smallW + 4, 86);
    colorAffinity.setBounds(rx + 155, y + 13, smallW + 4, 86);

    y += 102;
    solfeggio.setBounds(rx + 12, y, railW - 36, 28);
    masterBypass.setBounds(rx + 12, y + 34, railW - 36, 28);
}

void PRISMVSTAudioProcessorEditor::timerCallback()
{
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> spectrum {};
    processor.copySpectrum(spectrum);
    spectrumDisplay.pushSpectrum(spectrum);
    transferDisplay.repaint();

    peakLabel.setText("PEAK      " + juce::String(processor.getPeakDb(), 1) + " dBFS",
                      juce::dontSendNotification);
    lufsShortLabel.setText("LUFS-S    " + juce::String(processor.getLufsShort(), 1),
                           juce::dontSendNotification);
    lufsIntLabel.setText("LUFS-I    " + juce::String(processor.getLufsIntegrated(), 1),
                         juce::dontSendNotification);
}
