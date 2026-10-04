#include "PluginEditor.h"
#include <cmath>
#include <limits>

namespace
{
constexpr int kNodeControlCount = 12;
constexpr std::array<const char*, kNodeControlCount> kControlNames {
    "CENTER", "TRIM", "WIDTH / Q", "DEPTH", "DYN SLOPE", "ATTACK",
    "CURVE", "REL A", "REL B", "REL MIX", "SUSTAIN", "PEAK / RMS"
};

constexpr std::array<float, 9> kSolfeggio {
    174.0f, 285.0f, 396.0f, 417.0f, 528.0f, 639.0f, 741.0f, 852.0f, 963.0f
};

const std::array<juce::Colour, 9> kSolfeggioColours {
    juce::Colour::fromRGB(181, 36, 49),
    juce::Colour::fromRGB(232, 91, 33),
    juce::Colour::fromRGB(237, 161, 32),
    juce::Colour::fromRGB(187, 207, 46),
    juce::Colour::fromRGB(42, 194, 106),
    juce::Colour::fromRGB(31, 196, 189),
    juce::Colour::fromRGB(52, 126, 225),
    juce::Colour::fromRGB(92, 74, 205),
    juce::Colour::fromRGB(201, 64, 194)
};

float readParameter(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    if (auto* value = state.getRawParameterValue(id))
        return value->load();
    return 0.0f;
}

juce::Colour panelColour()
{
    return juce::Colour::fromRGB(13, 16, 20);
}

juce::Colour panelRaised()
{
    return juce::Colour::fromRGB(18, 22, 27);
}

juce::Colour lineColour()
{
    return juce::Colour::fromRGB(56, 64, 74);
}

juce::Colour textMuted()
{
    return juce::Colour::fromRGB(130, 141, 154);
}

juce::String formatFrequency(float f)
{
    if (f >= 10000.0f)
        return juce::String(f / 1000.0f, 1) + "k";
    if (f >= 1000.0f)
        return juce::String(f / 1000.0f, 2) + "k";
    return juce::String(f, f < 100.0f ? 1 : 0);
}

juce::Colour referenceColourForFrequency(float frequency,
                                         float strictness,
                                         float& affinity,
                                         float& familyFrequency)
{
    int nearest = 0;
    float nearestDistance = std::numeric_limits<float>::max();
    float nearestFamily = kSolfeggio.front();

    for (int i = 0; i < (int)kSolfeggio.size(); ++i)
    {
        const float base = kSolfeggio[(size_t)i];

        for (int octave = -5; octave <= 6; ++octave)
        {
            const float family = base * std::pow(2.0f, (float)octave);
            if (family < 10.0f || family > 40000.0f)
                continue;

            const float distance = std::abs(std::log2(
                juce::jmax(1.0f, frequency) / family));

            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = i;
                nearestFamily = family;
            }
        }
    }

    familyFrequency = nearestFamily;
    const float sigmaOctaves = juce::jmap(
        juce::jlimit(0.0f, 1.0f, strictness),
        0.0f, 1.0f, 0.30f, 0.055f);

    affinity = std::exp(-0.5f * nearestDistance * nearestDistance
                        / (sigmaOctaves * sigmaOctaves));

    return kSolfeggioColours[(size_t)nearest];
}

float meterNormalised(float db)
{
    return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
}
}

//==============================================================================
// Deliberate, acceleration-free studio control.
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
    const double proportion = juce::jlimit(
        0.0, 1.0, dragStartProportion + deltaPixels / pixelsForFullRange);

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
    const auto bounds = juce::Rectangle<float>((float)x, (float)y,
                                                (float)width, (float)height)
                            .reduced(9.0f, 7.0f);

    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto knob = juce::Rectangle<float>(diameter, diameter).withCentre(bounds.getCentre());
    knob = knob.reduced(5.0f);

    const float angle = juce::jmap(sliderPosProportional, 0.0f, 1.0f,
                                   rotaryStartAngle, rotaryEndAngle);

    // Physical body.
    g.setColour(juce::Colour::fromRGB(5, 7, 10));
    g.fillEllipse(knob);

    g.setColour(juce::Colour::fromRGB(32, 38, 45));
    g.drawEllipse(knob, 2.0f);

    auto inner = knob.reduced(knob.getWidth() * 0.13f);
    juce::ColourGradient body(
        juce::Colour::fromRGB(35, 40, 47), inner.getX(), inner.getY(),
        juce::Colour::fromRGB(10, 13, 17), inner.getRight(), inner.getBottom(), false);
    g.setGradientFill(body);
    g.fillEllipse(inner);

    // Restraint arc.
    auto arcBounds = knob.expanded(3.5f);
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(arcBounds.getCentreX(), arcBounds.getCentreY(),
                                arcBounds.getWidth() * 0.5f,
                                arcBounds.getHeight() * 0.5f,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour::fromRGB(58, 66, 77));
    g.strokePath(backgroundArc, juce::PathStrokeType(2.1f));

    juce::Path valueArc;
    valueArc.addCentredArc(arcBounds.getCentreX(), arcBounds.getCentreY(),
                           arcBounds.getWidth() * 0.5f,
                           arcBounds.getHeight() * 0.5f,
                           0.0f, rotaryStartAngle, angle, true);
    g.setColour(juce::Colour::fromRGB(213, 220, 227));
    g.strokePath(valueArc, juce::PathStrokeType(2.4f));

    // Needle.
    juce::Path pointer;
    const float pointerLength = knob.getHeight() * 0.30f;
    pointer.addRoundedRectangle(-1.1f, -knob.getHeight() * 0.34f,
                                2.2f, pointerLength, 1.0f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle)
                               .translated(knob.getCentreX(), knob.getCentreY()));
    g.setColour(juce::Colours::white.withAlpha(0.94f));
    g.fillPath(pointer);

    const float cap = knob.getWidth() * 0.13f;
    g.setColour(juce::Colour::fromRGB(100, 109, 121));
    g.fillEllipse(knob.getCentreX() - cap * 0.5f,
                  knob.getCentreY() - cap * 0.5f, cap, cap);
}

void EtherTechLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                juce::Button& button,
                                                const juce::Colour&,
                                                bool highlighted,
                                                bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool selected = button.getToggleState();

    auto fill = selected ? juce::Colour::fromRGB(37, 48, 58)
                         : juce::Colour::fromRGB(15, 19, 24);

    if (highlighted)
        fill = fill.brighter(0.08f);
    if (down)
        fill = fill.darker(0.12f);

    g.setColour(fill);
    g.fillRoundedRectangle(b, 5.0f);

    g.setColour(selected ? juce::Colour::fromRGB(103, 188, 220)
                         : juce::Colour::fromRGB(55, 63, 73));
    g.drawRoundedRectangle(b, 5.0f, selected ? 1.5f : 1.0f);
}

void EtherTechLookAndFeel::drawButtonText(juce::Graphics& g,
                                          juce::TextButton& button,
                                          bool,
                                          bool)
{
    g.setColour(button.getToggleState()
                    ? juce::Colour::fromRGB(241, 245, 248)
                    : juce::Colour::fromRGB(157, 168, 180));
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(7, 2),
                     juce::Justification::centred, 1);
}

void EtherTechLookAndFeel::drawToggleButton(juce::Graphics& g,
                                            juce::ToggleButton& button,
                                            bool highlighted,
                                            bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool on = button.getToggleState();

    auto fill = on ? juce::Colour::fromRGB(35, 55, 64)
                   : juce::Colour::fromRGB(15, 19, 24);
    if (highlighted)
        fill = fill.brighter(0.06f);
    if (down)
        fill = fill.darker(0.10f);

    g.setColour(fill);
    g.fillRoundedRectangle(b, 5.0f);
    g.setColour(on ? juce::Colour::fromRGB(79, 176, 210)
                   : juce::Colour::fromRGB(57, 64, 74));
    g.drawRoundedRectangle(b, 5.0f, 1.0f);

    g.setColour(on ? juce::Colours::white.withAlpha(0.94f) : textMuted());
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(7, 2),
                     juce::Justification::centred, 1);
}

void EtherTechLookAndFeel::drawComboBox(juce::Graphics& g,
                                        int width, int height,
                                        bool,
                                        int, int, int, int,
                                        juce::ComboBox&)
{
    auto b = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(0.5f);
    g.setColour(juce::Colour::fromRGB(14, 18, 23));
    g.fillRoundedRectangle(b, 5.0f);
    g.setColour(juce::Colour::fromRGB(58, 66, 76));
    g.drawRoundedRectangle(b, 5.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float)width - 14.0f;
    const float cy = (float)height * 0.5f;
    arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
    arrow.lineTo(cx, cy + 2.0f);
    arrow.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(juce::Colour::fromRGB(160, 171, 182));
    g.strokePath(arrow, juce::PathStrokeType(1.5f));
}

//==============================================================================
// Stationary Heat Aura analyzer: no row history, no waterfall, no scrolling.
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

void SpectrumAuraDisplay::setParameter(const juce::String& parameterId, float value)
{
    if (auto* p = processor.apvts.getParameter(parameterId))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}

juce::Rectangle<float> SpectrumAuraDisplay::graphBounds() const
{
    return getLocalBounds().toFloat().reduced(18.0f).withTrimmedBottom(40.0f);
}

juce::Rectangle<float> SpectrumAuraDisplay::nodeRailBounds() const
{
    auto b = getLocalBounds().toFloat().reduced(18.0f);
    return b.removeFromBottom(30.0f);
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

juce::Colour SpectrumAuraDisplay::auraColourFor(float frequency,
                                                 float& affinity,
                                                 float& referenceHz) const
{
    return referenceColourForFrequency(
        frequency,
        parameter("color_affinity"),
        affinity,
        referenceHz);
}

juce::Point<float> SpectrumAuraDisplay::nodePosition(int node) const
{
    const auto rail = nodeRailBounds();
    const auto id = "band" + juce::String(node + 1) + "_freq";
    return { frequencyToX(parameter(id)), rail.getCentreY() };
}

int SpectrumAuraDisplay::findNodeAt(juce::Point<float> point) const
{
    int hit = -1;
    float bestDistance = 18.0f;

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        const float distance = point.getDistanceFrom(nodePosition(i));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            hit = i;
        }
    }

    return hit;
}

float SpectrumAuraDisplay::clampedNodeFrequency(int node, float frequency) const
{
    constexpr float minimumRatio = 1.059463094f; // one semitone minimum spacing

    float low = 20.0f;
    float high = 20000.0f;

    if (node > 0)
    {
        const auto prevId = "band" + juce::String(node) + "_freq";
        low = juce::jmax(low, parameter(prevId) * minimumRatio);
    }

    if (node < PRISMVSTAudioProcessor::numEqBands - 1)
    {
        const auto nextId = "band" + juce::String(node + 2) + "_freq";
        high = juce::jmin(high, parameter(nextId) / minimumRatio);
    }

    if (low > high)
        return juce::jlimit(20.0f, 20000.0f, frequency);

    return juce::jlimit(low, high, frequency);
}

void SpectrumAuraDisplay::pushSpectrum(
    const std::array<float, PRISMVSTAudioProcessor::spectrumBins>& values)
{
    latestSpectrum = values;

    const float memorySeconds = juce::jlimit(0.05f, 10.0f, parameter("aura_memory"));
    const float decay = std::exp(-1.0f / (60.0f * memorySeconds));
    const float floor = displayFloorDb();

    for (int i = 0; i < auraColumns; ++i)
    {
        const float norm = (float)i / (float)(auraColumns - 1);
        const float freq = 20.0f * std::pow(1000.0f, norm);
        const float db = juce::jlimit(floor, 0.0f, displayedSpectrumDbAt(freq));
        const float instant = juce::jlimit(0.0f, 1.0f, (db - floor) / -floor);

        auraEnergy[(size_t)i] = juce::jmax(
            instant,
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
    const auto rail = nodeRailBounds();
    const float floor = displayFloorDb();

    g.setColour(panelColour());
    g.fillRoundedRectangle(b, 8.0f);

    g.setColour(juce::Colour::fromRGB(10, 13, 17));
    g.fillRoundedRectangle(rail, 6.0f);

    const std::array<float, 10> gridFreq {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    for (float f : gridFreq)
    {
        const float x = frequencyToX(f);
        g.setColour(lineColour().withAlpha(0.58f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());

        g.setColour(textMuted());
        const juce::String label = f >= 1000.0f
            ? juce::String(f / 1000.0f, f >= 10000.0f ? 0 : 1) + "k"
            : juce::String((int)f);

        g.drawText(label,
                   juce::roundToInt(x - 24.0f),
                   juce::roundToInt(b.getBottom() - 15.0f),
                   48, 13, juce::Justification::centred);
    }

    for (int i = 0; i <= 6; ++i)
    {
        const float db = floor + (-floor * ((float)i / 6.0f));
        const float y = levelToY(db);

        g.setColour(lineColour().withAlpha(i == 6 ? 0.80f : 0.34f));
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());

        g.setColour(textMuted());
        g.drawText(juce::String(db, 0),
                   juce::roundToInt(b.getX() + 4.0f),
                   juce::roundToInt(y - 7.0f),
                   38, 13, juce::Justification::centredLeft);
    }

    // Reference ticks are intentionally small. The energy field carries the colour.
    if (parameter("solfeggio_grid") > 0.5f)
    {
        for (int i = 0; i < (int)kSolfeggio.size(); ++i)
        {
            const float x = frequencyToX(kSolfeggio[(size_t)i]);
            g.setColour(kSolfeggioColours[(size_t)i].withAlpha(0.95f));
            g.fillRect(juce::Rectangle<float>(x - 1.5f, b.getY() + 3.0f, 3.0f, 10.0f));
        }
    }

    // Fixed-frequency aura cells. X never encodes time.
    for (int i = 0; i < auraColumns; ++i)
    {
        const float energy = auraEnergy[(size_t)i];
        if (energy < 0.006f)
            continue;

        const float norm = (float)i / (float)(auraColumns - 1);
        const float freq = 20.0f * std::pow(1000.0f, norm);
        const float db = juce::jlimit(floor, 0.0f, displayedSpectrumDbAt(freq));
        const float x = frequencyToX(freq);
        const float y = levelToY(db);

        float affinity = 0.0f;
        float reference = 0.0f;

        auto colour = parameter("solfeggio_grid") > 0.5f
            ? auraColourFor(freq, affinity, reference)
            : juce::Colour::fromRGB(73, 169, 203);

        const float alpha = juce::jlimit(
            0.0f, 0.88f,
            0.07f + energy * (0.46f + 0.34f * affinity));

        const float width = 4.0f + energy * 13.0f;
        const float height = 10.0f + energy * 68.0f;

        g.setColour(colour.withAlpha(alpha * 0.18f));
        g.fillEllipse(x - width * 2.2f, y - height * 1.2f,
                      width * 4.4f, height * 2.4f);

        g.setColour(colour.withAlpha(alpha * 0.46f));
        g.fillEllipse(x - width, y - height * 0.74f,
                      width * 2.0f, height * 1.48f);

        g.setColour(colour.withAlpha(alpha));
        g.fillEllipse(x - width * 0.30f, y - height * 0.22f,
                      width * 0.60f, height * 0.44f);
    }

    // Neutral measurement trace remains separate from aura colour.
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

    g.setColour(juce::Colour::fromRGB(228, 233, 238).withAlpha(0.76f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.25f));

    // Six isolated identity-stable node handles. No inferred territories.
    for (int node = 0; node < PRISMVSTAudioProcessor::numEqBands; ++node)
    {
        const auto p = nodePosition(node);
        const bool selected = node == selectedBand;
        const bool enabled = parameter("band" + juce::String(node + 1) + "_enabled") > 0.5f;
        const float freq = parameter("band" + juce::String(node + 1) + "_freq");

        float affinity = 0.0f;
        float ref = 0.0f;
        auto nodeColour = referenceColourForFrequency(
            freq, parameter("color_affinity"), affinity, ref);

        if (!enabled)
            nodeColour = juce::Colour::fromRGB(83, 89, 97);

        const float radius = selected ? 9.0f : 7.0f;

        if (selected)
        {
            g.setColour(nodeColour.withAlpha(0.20f));
            g.fillEllipse(p.x - 15.0f, p.y - 15.0f, 30.0f, 30.0f);
        }

        g.setColour(nodeColour);
        g.fillEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colour::fromRGB(6, 8, 11));
        g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
        g.drawText(juce::String(node + 1),
                   juce::roundToInt(p.x - 8.0f),
                   juce::roundToInt(p.y - 7.0f),
                   16, 14, juce::Justification::centred);
    }

    if (hasHover && b.contains(hoverPoint))
    {
        const float f = xToFrequency(hoverPoint.x);
        const float rawDb = spectrumDbAt(f);
        const float wavelength = 343.0f / juce::jmax(1.0f, f);

        float affinity = 0.0f;
        float ref = 0.0f;
        const auto aura = auraColourFor(f, affinity, ref);

        const juce::String line1 =
            juce::String(f, f < 1000.0f ? 1 : 0) + " Hz   "
            + juce::String(rawDb, 1) + " dBFS";

        const juce::String line2 =
            "lambda " + juce::String(wavelength, wavelength < 1.0f ? 2 : 1) + " m   "
            + formatFrequency(ref) + " aura   "
            + juce::String(affinity * 100.0f, 0) + "%";

        constexpr float boxW = 220.0f;
        constexpr float boxH = 42.0f;

        const float bx = juce::jlimit(
            b.getX(), b.getRight() - boxW, hoverPoint.x + 12.0f);
        const float by = juce::jlimit(
            b.getY(), b.getBottom() - boxH, hoverPoint.y - 50.0f);

        g.setColour(juce::Colour::fromRGB(5, 7, 10).withAlpha(0.96f));
        g.fillRoundedRectangle(bx, by, boxW, boxH, 5.0f);

        g.setColour(aura.withAlpha(0.90f));
        g.drawRoundedRectangle(bx, by, boxW, boxH, 5.0f, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(0.95f));
        g.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        g.drawText(line1, juce::roundToInt(bx + 8.0f), juce::roundToInt(by + 5.0f),
                   (int)boxW - 16, 14, juce::Justification::centredLeft);

        g.setColour(textMuted());
        g.setFont(juce::Font(juce::FontOptions(9.5f)));
        g.drawText(line2, juce::roundToInt(bx + 8.0f), juce::roundToInt(by + 22.0f),
                   (int)boxW - 16, 14, juce::Justification::centredLeft);
    }

    g.setColour(juce::Colour::fromRGB(79, 89, 101));
    g.drawRoundedRectangle(b, 8.0f, 1.0f);
    g.drawRoundedRectangle(rail, 6.0f, 1.0f);
}

void SpectrumAuraDisplay::mouseDown(const juce::MouseEvent& e)
{
    draggingNode = findNodeAt(e.position);

    // Background clicks never change node identity.
    if (draggingNode < 0)
        return;

    selectedBand = draggingNode;
    dragStartFrequency = parameter("band" + juce::String(draggingNode + 1) + "_freq");
    dragStartX = e.position.x;

    if (auto* p = processor.apvts.getParameter(
            "band" + juce::String(draggingNode + 1) + "_freq"))
        p->beginChangeGesture();

    if (onBandSelected)
        onBandSelected(draggingNode);

    repaint();
}

void SpectrumAuraDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingNode < 0)
        return;

    int mode = juce::roundToInt(parameter("precision_mode"));

    if (e.mods.isShiftDown())
        mode = juce::jmax(mode, 1);

    if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        mode = 2;

    const float pixelsForFullRange = mode == 0 ? 3000.0f
                                               : (mode == 1 ? 9000.0f : 24000.0f);

    const float startNorm =
        std::log10(juce::jlimit(20.0f, 20000.0f, dragStartFrequency) / 20.0f)
        / std::log10(1000.0f);

    const float norm = juce::jlimit(
        0.0f, 1.0f,
        startNorm + (e.position.x - dragStartX) / pixelsForFullRange);

    const float proposed = 20.0f * std::pow(1000.0f, norm);
    const float frequency = clampedNodeFrequency(draggingNode, proposed);

    setParameter("band" + juce::String(draggingNode + 1) + "_freq", frequency);
    repaint();
}

void SpectrumAuraDisplay::mouseUp(const juce::MouseEvent&)
{
    if (draggingNode >= 0)
    {
        if (auto* p = processor.apvts.getParameter(
                "band" + juce::String(draggingNode + 1) + "_freq"))
            p->endChangeGesture();
    }

    draggingNode = -1;
}

void SpectrumAuraDisplay::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int node = findNodeAt(e.position);
    if (node < 0)
        return;

    const auto enabledId = "band" + juce::String(node + 1) + "_enabled";
    setParameter(enabledId, parameter(enabledId) > 0.5f ? 0.0f : 1.0f);

    if (onBandSelected)
        onBandSelected(node);

    repaint();
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
// Maximus-derived input -> output transfer map for the selected node.
DynamicsTransferDisplay::DynamicsTransferDisplay(PRISMVSTAudioProcessor& p)
    : processor(p)
{
    setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> DynamicsTransferDisplay::graphBounds() const
{
    return getLocalBounds().toFloat().reduced(18.0f, 15.0f);
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

float DynamicsTransferDisplay::outputDbForInput(float inputDb) const
{
    const float threshold = parameter("threshold");

    if (inputDb <= threshold)
        return inputDb;

    const float ratio = juce::jlimit(1.0f, 20.0f, parameter("ratio"));
    const float depth = juce::jlimit(0.0f, 24.0f, parameter("dyn_range"));
    const float curve = juce::jlimit(-1.0f, 1.0f, parameter("curve"));

    const float over = inputDb - threshold;
    const float exponent = juce::jmap(curve, -1.0f, 1.0f, 0.65f, 1.75f);
    const float shaped = 24.0f * std::pow(over / 24.0f, exponent);
    const float reduction = juce::jmin(
        depth, shaped * (1.0f - 1.0f / ratio));

    return inputDb - reduction;
}

juce::Point<float> DynamicsTransferDisplay::thresholdPoint() const
{
    const float threshold = parameter("threshold");
    return { dbToX(threshold), dbToY(threshold) };
}

juce::Point<float> DynamicsTransferDisplay::ratioPoint() const
{
    const float threshold = parameter("threshold");
    const float input = juce::jmin(0.0f, threshold + 18.0f);
    return { dbToX(input), dbToY(outputDbForInput(input)) };
}

float DynamicsTransferDisplay::dragScale(const juce::ModifierKeys& mods) const
{
    int mode = juce::roundToInt(readParameter(processor.apvts, "precision_mode"));

    if (mods.isShiftDown())
        mode = juce::jmax(mode, 1);

    if (mods.isCtrlDown() || mods.isCommandDown())
        mode = 2;

    return mode == 0 ? 3000.0f : (mode == 1 ? 9000.0f : 24000.0f);
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

        g.setColour(lineColour().withAlpha(0.34f));
        g.drawVerticalLine(juce::roundToInt(x), b.getY(), b.getBottom());
        g.drawHorizontalLine(juce::roundToInt(y), b.getX(), b.getRight());
    }

    g.setColour(juce::Colour::fromRGB(100, 111, 123).withAlpha(0.55f));
    g.drawLine(b.getX(), b.getBottom(), b.getRight(), b.getY(), 1.0f);

    juce::Path curvePath;
    bool started = false;

    for (int px = 0; px <= juce::roundToInt(b.getWidth()); px += 2)
    {
        const float input = xToDb(b.getX() + (float)px);
        const float output = outputDbForInput(input);
        const float x = dbToX(input);
        const float y = dbToY(output);

        if (!started)
        {
            curvePath.startNewSubPath(x, y);
            started = true;
        }
        else
        {
            curvePath.lineTo(x, y);
        }
    }

    const float centreFreq = readParameter(
        processor.apvts,
        "band" + juce::String(selectedBand + 1) + "_freq");

    float affinity = 0.0f;
    float reference = 0.0f;
    const auto nodeColour = referenceColourForFrequency(
        centreFreq,
        readParameter(processor.apvts, "color_affinity"),
        affinity,
        reference);

    g.setColour(nodeColour.withAlpha(0.92f));
    g.strokePath(curvePath, juce::PathStrokeType(2.2f));

    const auto thresholdHandle = thresholdPoint();
    const auto slopeHandle = ratioPoint();

    g.setColour(nodeColour);
    g.fillEllipse(thresholdHandle.x - 6.5f, thresholdHandle.y - 6.5f, 13.0f, 13.0f);

    g.setColour(juce::Colour::fromRGB(228, 233, 238));
    g.fillEllipse(slopeHandle.x - 5.5f, slopeHandle.y - 5.5f, 11.0f, 11.0f);

    g.setColour(textMuted());
    g.setFont(juce::Font(juce::FontOptions(9.3f)));

    const juce::String info =
        "NODE " + juce::String(selectedBand + 1)
        + "   THR " + juce::String(parameter("threshold"), 1) + " dB"
        + "   SLOPE " + juce::String(parameter("ratio"), 2) + ":1"
        + "   CURVE " + juce::String(parameter("curve"), 2)
        + "   DEPTH " + juce::String(parameter("dyn_range"), 1) + " dB";

    g.drawFittedText(info,
                     juce::roundToInt(b.getX() + 8.0f),
                     juce::roundToInt(b.getY() + 4.0f),
                     juce::roundToInt(b.getWidth() - 16.0f),
                     14, juce::Justification::centred, 1);

    g.setColour(juce::Colour::fromRGB(82, 92, 104));
    g.drawRoundedRectangle(b, 7.0f, 1.0f);
}

void DynamicsTransferDisplay::mouseDown(const juce::MouseEvent& e)
{
    const float thresholdDistance = e.position.getDistanceFrom(thresholdPoint());
    const float ratioDistance = e.position.getDistanceFrom(ratioPoint());

    // No implicit "nearest handle" behavior: you must actually grab a handle.
    if (thresholdDistance <= 16.0f)
    {
        dragTarget = DragTarget::threshold;
        dragStartValue = parameter("threshold");
        dragStartPixel = e.position.x;

        if (auto* p = rangedParameter("threshold"))
            p->beginChangeGesture();
    }
    else if (ratioDistance <= 16.0f)
    {
        dragTarget = DragTarget::ratio;
        dragStartValue = parameter("ratio");
        dragStartPixel = e.position.y;

        if (auto* p = rangedParameter("ratio"))
            p->beginChangeGesture();
    }
    else
    {
        dragTarget = DragTarget::none;
    }
}

void DynamicsTransferDisplay::mouseDrag(const juce::MouseEvent& e)
{
    const float pixels = dragScale(e.mods);

    if (dragTarget == DragTarget::threshold)
    {
        const float delta = (e.position.x - dragStartPixel) * 60.0f / pixels;
        setParameter("threshold",
                     juce::jlimit(-60.0f, 0.0f, dragStartValue + delta));
    }
    else if (dragTarget == DragTarget::ratio)
    {
        const float delta = (dragStartPixel - e.position.y) * 19.0f / pixels;
        setParameter("ratio",
                     juce::jlimit(1.0f, 20.0f, dragStartValue + delta));
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
    if (e.position.getDistanceFrom(thresholdPoint()) <= 16.0f)
        setParameter("threshold", -18.0f);
    else if (e.position.getDistanceFrom(ratioPoint()) <= 16.0f)
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
    setSize(1360, 900);
    setResizable(true, true);
    setResizeLimits(1180, 820, 1800, 1180);

    setLookAndFeel(&lookAndFeel);

    lookAndFeel.setColour(juce::Slider::textBoxTextColourId,
                          juce::Colour::fromRGB(214, 221, 228));
    lookAndFeel.setColour(juce::Slider::textBoxBackgroundColourId,
                          juce::Colour::fromRGB(8, 11, 14));
    lookAndFeel.setColour(juce::Slider::textBoxOutlineColourId,
                          juce::Colour::fromRGB(42, 49, 58));
    lookAndFeel.setColour(juce::ComboBox::textColourId,
                          juce::Colour::fromRGB(211, 218, 225));
    lookAndFeel.setColour(juce::ComboBox::backgroundColourId,
                          juce::Colour::fromRGB(14, 18, 23));
    lookAndFeel.setColour(juce::ComboBox::outlineColourId,
                          juce::Colours::transparentBlack);

    addAndMakeVisible(spectrumDisplay);
    addAndMakeVisible(transferDisplay);

    spectrumDisplay.onBandSelected = [this](int band)
    {
        bindSelectedBand(band);
    };

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        auto& button = nodeButtons[(size_t)i];
        button.setClickingTogglesState(true);
        button.setRadioGroupId(101);
        button.onClick = [this, i] { bindSelectedBand(i); };
        addAndMakeVisible(button);
    }

    selectedBandLabel.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    selectedBandLabel.setColour(juce::Label::textColourId,
                                juce::Colour::fromRGB(164, 175, 187));
    selectedBandLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(selectedBandLabel);

    const std::array<PrecisionSlider*, nodeControlCount> nodeSliders {
        &frequency, &gain, &q, &dynamicRange, &slope, &attack,
        &curve, &releaseA, &releaseB, &releaseBlend, &sustain, &detectorMix
    };

    for (size_t i = 0; i < nodeSliders.size(); ++i)
    {
        configureRotary(*nodeSliders[i]);
        addAndMakeVisible(*nodeSliders[i]);

        controlLabels[i].setText(kControlNames[i], juce::dontSendNotification);
        controlLabels[i].setFont(juce::Font(juce::FontOptions(8.8f, juce::Font::bold)));
        controlLabels[i].setColour(juce::Label::textColourId, textMuted());
        controlLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(controlLabels[i]);
    }

    frequency.setTextValueSuffix(" Hz");
    gain.setTextValueSuffix(" dB");
    dynamicRange.setTextValueSuffix(" dB");
    slope.setTextValueSuffix(":1");
    attack.setTextValueSuffix(" ms");
    releaseA.setTextValueSuffix(" ms");
    releaseB.setTextValueSuffix(" ms");
    sustain.setTextValueSuffix(" ms");

    curve.textFromValueFunction = [] (double value)
    {
        if (std::abs(value) < 0.005)
            return juce::String("LINEAR");
        return juce::String(value, 2);
    };

    releaseBlend.textFromValueFunction = [] (double value)
    {
        return juce::String(value * 100.0, 0) + "% B";
    };

    detectorMix.textFromValueFunction = [] (double value)
    {
        if (value <= 0.01)
            return juce::String("PEAK");
        if (value >= 0.99)
            return juce::String("RMS");
        return juce::String(value * 100.0, 0) + "% RMS";
    };

    for (auto* slider : { &onyx, &onyxDrive, &masterTrim, &ceiling,
                          &analyzerSlope, &auraMemory, &colorAffinity })
    {
        configureRotary(*slider);
        addAndMakeVisible(*slider);
    }

    onyxDrive.setTextValueSuffix(" dB");
    masterTrim.setTextValueSuffix(" dB");
    ceiling.setTextValueSuffix(" dBFS");
    analyzerSlope.setTextValueSuffix(" dB/oct");
    auraMemory.setTextValueSuffix(" s");

    colorAffinity.textFromValueFunction = [] (double value)
    {
        return juce::String(value * 100.0, 0) + " %";
    };

    onyxLabel.setText("ONYX", juce::dontSendNotification);
    driveLabel.setText("DRIVE", juce::dontSendNotification);
    trimLabel.setText("OUTPUT", juce::dontSendNotification);
    ceilingLabel.setText("CEILING", juce::dontSendNotification);
    analyzerSlopeLabel.setText("DISPLAY SLOPE", juce::dontSendNotification);
    auraMemoryLabel.setText("AURA MEMORY", juce::dontSendNotification);
    affinityLabel.setText("COLOR AFFINITY", juce::dontSendNotification);
    depthLabel.setText("ANALYZER DEPTH", juce::dontSendNotification);
    precisionLabel.setText("CONTROL PRECISION", juce::dontSendNotification);

    for (auto* label : { &onyxLabel, &driveLabel, &trimLabel, &ceilingLabel,
                         &analyzerSlopeLabel, &auraMemoryLabel, &affinityLabel,
                         &depthLabel, &precisionLabel })
    {
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(juce::FontOptions(8.7f, juce::Font::bold)));
        label->setColour(juce::Label::textColourId, textMuted());
        addAndMakeVisible(*label);
    }

    analyzerDepth.addItem("MIX  0 -> -36 dB", 1);
    analyzerDepth.addItem("DEEP  0 -> -72 dB", 2);
    analyzerDepth.addItem("FORENSIC  0 -> -120 dB", 3);
    addAndMakeVisible(analyzerDepth);

    precisionMode.addItem("NORMAL", 1);
    precisionMode.addItem("FINE", 2);
    precisionMode.addItem("MICRO", 3);
    addAndMakeVisible(precisionMode);

    addAndMakeVisible(nodeEnabled);
    addAndMakeVisible(solfeggio);
    addAndMakeVisible(masterBypass);

    for (auto* label : { &inputPeakLabel, &grLabel, &peakLabel,
                         &lufsShortLabel, &lufsIntLabel })
    {
        label->setJustificationType(juce::Justification::centredLeft);
        label->setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        label->setColour(juce::Label::textColourId,
                         juce::Colour::fromRGB(173, 183, 194));
        addAndMakeVisible(*label);
    }

    onyxA = std::make_unique<SliderAttachment>(processor.apvts, "onyx", onyx);
    onyxDriveA = std::make_unique<SliderAttachment>(processor.apvts, "onyx_drive", onyxDrive);
    masterTrimA = std::make_unique<SliderAttachment>(processor.apvts, "master_trim", masterTrim);
    ceilingA = std::make_unique<SliderAttachment>(processor.apvts, "ceiling", ceiling);

    analyzerSlopeA = std::make_unique<SliderAttachment>(
        processor.apvts, "analyzer_slope", analyzerSlope);
    auraMemoryA = std::make_unique<SliderAttachment>(
        processor.apvts, "aura_memory", auraMemory);
    colorAffinityA = std::make_unique<SliderAttachment>(
        processor.apvts, "color_affinity", colorAffinity);

    analyzerDepthA = std::make_unique<ComboBoxAttachment>(
        processor.apvts, "analyzer_depth", analyzerDepth);
    precisionModeA = std::make_unique<ComboBoxAttachment>(
        processor.apvts, "precision_mode", precisionMode);

    solfeggioA = std::make_unique<ButtonAttachment>(
        processor.apvts, "solfeggio_grid", solfeggio);
    masterBypassA = std::make_unique<ButtonAttachment>(
        processor.apvts, "master_bypass", masterBypass);

    enableDefaultReset(onyx, "onyx");
    enableDefaultReset(onyxDrive, "onyx_drive");
    enableDefaultReset(masterTrim, "master_trim");
    enableDefaultReset(ceiling, "ceiling");
    enableDefaultReset(analyzerSlope, "analyzer_slope");
    enableDefaultReset(auraMemory, "aura_memory");
    enableDefaultReset(colorAffinity, "color_affinity");

    precisionMode.onChange = [this] { applyPrecisionMode(); };
    frequency.onValueChange = [this] { clampSelectedFrequency(); };

    bindSelectedBand(0);
    updateNodeButtonText();
    applyPrecisionMode();

    startTimerHz(60);
}

PRISMVSTAudioProcessorEditor::~PRISMVSTAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void PRISMVSTAudioProcessorEditor::configureRotary(juce::Slider& slider,
                                                    const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.20f,
                               juce::MathConstants<float>::pi * 2.80f,
                               true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 18);
    slider.setTextValueSuffix(suffix);
    slider.setVelocityBasedMode(false);
    slider.setScrollWheelEnabled(true);
}

void PRISMVSTAudioProcessorEditor::enableDefaultReset(
    juce::Slider& slider,
    const juce::String& parameterId)
{
    if (auto* p = processor.apvts.getParameter(parameterId))
        slider.setDoubleClickReturnValue(
            true, p->convertFrom0to1(p->getDefaultValue()));
}

void PRISMVSTAudioProcessorEditor::applyPrecisionMode()
{
    const int mode = juce::jlimit(0, 2, precisionMode.getSelectedId() - 1);

    for (auto* slider : {
             &frequency, &gain, &q, &dynamicRange, &slope, &attack,
             &curve, &releaseA, &releaseB, &releaseBlend, &sustain, &detectorMix,
             &onyx, &onyxDrive, &masterTrim, &ceiling,
             &analyzerSlope, &auraMemory, &colorAffinity })
        slider->setPrecisionMode(mode);
}

void PRISMVSTAudioProcessorEditor::clampSelectedFrequency()
{
    if (clampingFrequency)
        return;

    constexpr float minimumRatio = 1.059463094f;

    float low = 20.0f;
    float high = 20000.0f;

    if (selectedBand > 0)
    {
        const auto prev = "band" + juce::String(selectedBand) + "_freq";
        low = juce::jmax(
            low, readParameter(processor.apvts, prev) * minimumRatio);
    }

    if (selectedBand < PRISMVSTAudioProcessor::numEqBands - 1)
    {
        const auto next = "band" + juce::String(selectedBand + 2) + "_freq";
        high = juce::jmin(
            high, readParameter(processor.apvts, next) / minimumRatio);
    }

    if (low > high)
        return;

    const double clamped = juce::jlimit(
        (double)low, (double)high, frequency.getValue());

    if (std::abs(clamped - frequency.getValue()) > 1.0e-6)
    {
        clampingFrequency = true;
        frequency.setValue(clamped, juce::sendNotificationSync);
        clampingFrequency = false;
    }
}

void PRISMVSTAudioProcessorEditor::updateNodeButtonText()
{
    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
    {
        const float freq = readParameter(
            processor.apvts,
            "band" + juce::String(i + 1) + "_freq");

        nodeButtons[(size_t)i].setButtonText(
            juce::String(i + 1) + "   " + formatFrequency(freq) + " Hz");

        const bool enabled = readParameter(
            processor.apvts,
            "band" + juce::String(i + 1) + "_enabled") > 0.5f;
        nodeButtons[(size_t)i].setAlpha(enabled ? 1.0f : 0.52f);
    }
}

void PRISMVSTAudioProcessorEditor::bindSelectedBand(int band)
{
    selectedBand = juce::jlimit(
        0, PRISMVSTAudioProcessor::numEqBands - 1, band);

    spectrumDisplay.setSelectedBand(selectedBand);
    transferDisplay.setSelectedBand(selectedBand);

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
        nodeButtons[(size_t)i].setToggleState(
            i == selectedBand, juce::dontSendNotification);

    const auto prefix = "band" + juce::String(selectedBand + 1) + "_";

    frequencyA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "freq", frequency);
    gainA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "gain", gain);
    qA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "q", q);
    dynamicRangeA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "dyn_range", dynamicRange);
    slopeA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "ratio", slope);
    attackA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "attack", attack);

    curveA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "curve", curve);
    releaseAA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "release", releaseA);
    releaseBA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "release2", releaseB);
    releaseBlendA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "release_blend", releaseBlend);
    sustainA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "sustain", sustain);
    detectorMixA = std::make_unique<SliderAttachment>(
        processor.apvts, prefix + "detector_mix", detectorMix);

    nodeEnabledA = std::make_unique<ButtonAttachment>(
        processor.apvts, prefix + "enabled", nodeEnabled);

    for (const auto& pair : std::array<std::pair<juce::Slider*, juce::String>, nodeControlCount> {{
        { &frequency, prefix + "freq" },
        { &gain, prefix + "gain" },
        { &q, prefix + "q" },
        { &dynamicRange, prefix + "dyn_range" },
        { &slope, prefix + "ratio" },
        { &attack, prefix + "attack" },
        { &curve, prefix + "curve" },
        { &releaseA, prefix + "release" },
        { &releaseB, prefix + "release2" },
        { &releaseBlend, prefix + "release_blend" },
        { &sustain, prefix + "sustain" },
        { &detectorMix, prefix + "detector_mix" }
    }})
        enableDefaultReset(*pair.first, pair.second);

    clampSelectedFrequency();

    selectedBandLabel.setText(
        "NODE " + juce::String(selectedBand + 1)
        + "  |  isolated identity  |  transfer slope + curve are per-node"
        + "  |  SHIFT = fine  CTRL/CMD = micro",
        juce::dontSendNotification);

    updateNodeButtonText();
}

void PRISMVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(6, 8, 11));

    // Header.
    g.setColour(juce::Colour::fromRGB(242, 245, 248));
    g.setFont(juce::Font(juce::FontOptions(25.0f, juce::Font::bold)));
    g.drawText("PRISM", 20, 10, 120, 32, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(105, 188, 217));
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::bold)));
    g.drawText("ALPHA 001", 124, 17, 78, 18, juce::Justification::centredLeft);

    g.setColour(textMuted());
    g.setFont(juce::Font(juce::FontOptions(9.5f)));
    g.drawText("ETHERTECH  /  SIX-NODE SPECTRAL DYNAMICS  /  STATIONARY HEAT AURA",
               205, 17, 520, 18, juce::Justification::centredLeft);

    g.setColour(lineColour());
    g.drawLine(20.0f, 50.0f, (float)getWidth() - 20.0f, 50.0f, 1.0f);

    const int railW = 252;
    const float railX = (float)getWidth() - railW - 14.0f;

    g.setColour(juce::Colour::fromRGB(10, 13, 17));
    g.fillRoundedRectangle(railX, 60.0f,
                           (float)railW, (float)getHeight() - 76.0f, 9.0f);

    g.setColour(juce::Colour::fromRGB(37, 44, 52));
    g.drawRoundedRectangle(railX, 60.0f,
                           (float)railW, (float)getHeight() - 76.0f, 9.0f, 1.0f);

    g.setColour(textMuted());
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText("MASTER / METERING / ANALYZER",
               (int)railX + 10, 66, railW - 20, 16,
               juce::Justification::centred);

    // Meter bars.
    const float meterX = railX + 122.0f;
    const float meterW = railW - 140.0f;
    const std::array<float, 3> meterY { 317.0f, 345.0f, 373.0f };

    const float inputNorm = meterNormalised(processor.getInputPeakDb());
    const float grNorm = juce::jlimit(0.0f, 1.0f, processor.getGainReductionDb() / 24.0f);
    const float outputNorm = meterNormalised(processor.getPeakDb());

    for (float y : meterY)
    {
        g.setColour(juce::Colour::fromRGB(23, 29, 35));
        g.fillRoundedRectangle(meterX, y, meterW, 5.0f, 2.5f);
    }

    g.setColour(juce::Colour::fromRGB(180, 190, 200));
    g.fillRoundedRectangle(meterX, meterY[0], meterW * inputNorm, 5.0f, 2.5f);

    g.setColour(juce::Colour::fromRGB(99, 186, 215));
    g.fillRoundedRectangle(meterX, meterY[1], meterW * grNorm, 5.0f, 2.5f);

    g.setColour(juce::Colour::fromRGB(218, 224, 230));
    g.fillRoundedRectangle(meterX, meterY[2], meterW * outputNorm, 5.0f, 2.5f);

    // Selected-node engine panel.
    const int left = 18;
    const int graphRight = getWidth() - railW - 30;
    const int graphW = graphRight - left;
    const int engineH = 214;
    const int engineY = getHeight() - engineH - 14;

    g.setColour(panelRaised());
    g.fillRoundedRectangle((float)left, (float)engineY,
                           (float)graphW, (float)engineH, 8.0f);

    g.setColour(juce::Colour::fromRGB(42, 49, 58));
    g.drawRoundedRectangle((float)left, (float)engineY,
                           (float)graphW, (float)engineH, 8.0f, 1.0f);

    g.setColour(textMuted());
    g.setFont(juce::Font(juce::FontOptions(8.8f, juce::Font::bold)));
    g.drawText("SELECTED NODE ENGINE",
               left + 10, engineY + 6, graphW - 20, 14,
               juce::Justification::centredLeft);
}

void PRISMVSTAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    const int railW = 252;
    const int left = 18;
    const int graphRight = w - railW - 30;
    const int graphW = graphRight - left;

    // Node selector strip.
    const int tabsY = 58;
    const int tabGap = 5;
    const int tabW = (graphW - tabGap * (PRISMVSTAudioProcessor::numEqBands - 1))
                     / PRISMVSTAudioProcessor::numEqBands;

    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
        nodeButtons[(size_t)i].setBounds(
            left + i * (tabW + tabGap), tabsY, tabW, 32);

    // Main analysis + transfer maps.
    const int engineH = 214;
    const int engineY = h - engineH - 14;
    const int labelH = 22;
    const int transferH = 160;

    const int analyzerY = 98;
    const int analyzerH = juce::jmax(
        265, engineY - analyzerY - transferH - labelH - 18);

    spectrumDisplay.setBounds(left, analyzerY, graphW, analyzerH);

    const int transferY = analyzerY + analyzerH + 8;
    transferDisplay.setBounds(left, transferY, graphW, transferH);

    selectedBandLabel.setBounds(
        left + 8, transferY + transferH + 1, graphW - 142, labelH);
    nodeEnabled.setBounds(
        left + graphW - 126, transferY + transferH + 1, 118, labelH);

    // Twelve selected-node controls, two rows of six.
    const int controlsTop = engineY + 22;
    const int rowGap = 2;
    const int rowH = (engineH - 27 - rowGap) / 2;
    const int cellW = graphW / 6;

    const std::array<PrecisionSlider*, nodeControlCount> sliders {
        &frequency, &gain, &q, &dynamicRange, &slope, &attack,
        &curve, &releaseA, &releaseB, &releaseBlend, &sustain, &detectorMix
    };

    for (int i = 0; i < nodeControlCount; ++i)
    {
        const int row = i / 6;
        const int col = i % 6;
        const int x = left + col * cellW;
        const int y = controlsTop + row * (rowH + rowGap);

        controlLabels[(size_t)i].setBounds(x, y, cellW, 14);
        sliders[(size_t)i]->setBounds(
            x + 5, y + 13, cellW - 10, rowH - 13);
    }

    // Right rail.
    const int rx = w - railW + 1;
    const int knobW = 100;
    const int rowGapKnob = 108;

    onyxLabel.setBounds(rx + 8, 88, knobW, 15);
    onyx.setBounds(rx + 8, 102, knobW, 90);

    driveLabel.setBounds(rx + 114, 88, knobW, 15);
    onyxDrive.setBounds(rx + 114, 102, knobW, 90);

    trimLabel.setBounds(rx + 8, 88 + rowGapKnob, knobW, 15);
    masterTrim.setBounds(rx + 8, 102 + rowGapKnob, knobW, 90);

    ceilingLabel.setBounds(rx + 114, 88 + rowGapKnob, knobW, 15);
    ceiling.setBounds(rx + 114, 102 + rowGapKnob, knobW, 90);

    inputPeakLabel.setBounds(rx + 8, 305, 108, 22);
    grLabel.setBounds(rx + 8, 333, 108, 22);
    peakLabel.setBounds(rx + 8, 361, 108, 22);
    lufsShortLabel.setBounds(rx + 8, 389, railW - 30, 20);
    lufsIntLabel.setBounds(rx + 8, 412, railW - 30, 20);

    int y = 448;

    depthLabel.setBounds(rx + 8, y, railW - 28, 14);
    y += 15;
    analyzerDepth.setBounds(rx + 16, y, railW - 44, 25);
    y += 34;

    precisionLabel.setBounds(rx + 8, y, railW - 28, 14);
    y += 15;
    precisionMode.setBounds(rx + 16, y, railW - 44, 25);
    y += 36;

    const int smallW = 72;

    analyzerSlopeLabel.setBounds(rx + 2, y, smallW + 4, 14);
    auraMemoryLabel.setBounds(rx + 78, y, smallW + 6, 14);
    affinityLabel.setBounds(rx + 158, y, smallW + 8, 14);

    analyzerSlope.setBounds(rx + 2, y + 13, smallW + 4, 82);
    auraMemory.setBounds(rx + 79, y + 13, smallW + 4, 82);
    colorAffinity.setBounds(rx + 158, y + 13, smallW + 4, 82);

    y += 100;

    solfeggio.setBounds(rx + 16, y, railW - 44, 28);
    masterBypass.setBounds(rx + 16, y + 34, railW - 44, 28);
}

void PRISMVSTAudioProcessorEditor::timerCallback()
{
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> spectrum {};
    processor.copySpectrum(spectrum);

    spectrumDisplay.pushSpectrum(spectrum);
    transferDisplay.repaint();
    updateNodeButtonText();

    inputPeakLabel.setText(
        "IN   " + juce::String(processor.getInputPeakDb(), 1) + " dBFS",
        juce::dontSendNotification);

    grLabel.setText(
        "GR   " + juce::String(processor.getGainReductionDb(), 1) + " dB",
        juce::dontSendNotification);

    peakLabel.setText(
        "OUT  " + juce::String(processor.getPeakDb(), 1) + " dBFS",
        juce::dontSendNotification);

    lufsShortLabel.setText(
        "LUFS-S   " + juce::String(processor.getLufsShort(), 1),
        juce::dontSendNotification);

    lufsIntLabel.setText(
        "LUFS-I   " + juce::String(processor.getLufsIntegrated(), 1),
        juce::dontSendNotification);

    repaint();
}
