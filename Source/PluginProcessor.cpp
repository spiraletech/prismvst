#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float kFloorDb = -144.0f;
constexpr std::array<float, PRISMVSTAudioProcessor::numCrossovers> kDefaultCrossovers {
    60.0f, 120.0f, 250.0f, 500.0f, 2000.0f, 6000.0f
};

inline float toDb(float g) { return juce::Decibels::gainToDecibels(g, kFloorDb); }
inline float toGain(float d) { return juce::Decibels::decibelsToGain(d); }

inline float safeFrequency(double sampleRate, float frequency)
{
    return juce::jlimit(20.0f, (float)(sampleRate * 0.45), frequency);
}
}

const char* PRISMVSTAudioProcessor::sectionName(int section) noexcept
{
    static constexpr std::array<const char*, numSections> names {
        "SUB", "KICK", "LOW", "LOWER MID", "MID", "HIGH", "HIGHER"
    };
    return names[(size_t)juce::jlimit(0, numSections - 1, section)];
}

juce::String PRISMVSTAudioProcessor::sectionId(int section, const juce::String& suffix)
{
    return "section" + juce::String(section + 1) + "_" + suffix;
}

juce::String PRISMVSTAudioProcessor::crossoverId(int crossover)
{
    return "crossover" + juce::String(crossover + 1) + "_hz";
}

float PRISMVSTAudioProcessor::read(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    if (auto* v = state.getRawParameterValue(id))
        return v->load();
    return 0.0f;
}

//==============================================================================
// Allocation-free filter primitives.
float PRISMVSTAudioProcessor::Biquad::process(float x) noexcept
{
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

void PRISMVSTAudioProcessor::Biquad::setFirstOrderLowPass(double sr, float f) noexcept
{
    const float k = std::tan(juce::MathConstants<float>::pi * safeFrequency(sr, f) / (float)sr);
    const float n = 1.0f / (1.0f + k);
    b0 = k * n; b1 = b0; b2 = 0.0f;
    a1 = (k - 1.0f) * n; a2 = 0.0f;
}

void PRISMVSTAudioProcessor::Biquad::setFirstOrderHighPass(double sr, float f) noexcept
{
    const float k = std::tan(juce::MathConstants<float>::pi * safeFrequency(sr, f) / (float)sr);
    const float n = 1.0f / (1.0f + k);
    b0 = n; b1 = -n; b2 = 0.0f;
    a1 = (k - 1.0f) * n; a2 = 0.0f;
}

void PRISMVSTAudioProcessor::Biquad::setSecondOrderLowPass(double sr, float f, float q) noexcept
{
    const float k = std::tan(juce::MathConstants<float>::pi * safeFrequency(sr, f) / (float)sr);
    const float k2 = k * k;
    const float n = 1.0f / (1.0f + k / q + k2);
    b0 = k2 * n; b1 = 2.0f * b0; b2 = b0;
    a1 = 2.0f * (k2 - 1.0f) * n;
    a2 = (1.0f - k / q + k2) * n;
}

void PRISMVSTAudioProcessor::Biquad::setSecondOrderHighPass(double sr, float f, float q) noexcept
{
    const float k = std::tan(juce::MathConstants<float>::pi * safeFrequency(sr, f) / (float)sr);
    const float k2 = k * k;
    const float n = 1.0f / (1.0f + k / q + k2);
    b0 = n; b1 = -2.0f * n; b2 = n;
    a1 = 2.0f * (k2 - 1.0f) * n;
    a2 = (1.0f - k / q + k2) * n;
}

void PRISMVSTAudioProcessor::Biquad::setFirstOrderAllPass(double sr, float f) noexcept
{
    const float k = std::tan(juce::MathConstants<float>::pi * safeFrequency(sr, f) / (float)sr);
    const float a = (k - 1.0f) / (k + 1.0f);
    b0 = a; b1 = 1.0f; b2 = 0.0f;
    a1 = a; a2 = 0.0f;
}

void PRISMVSTAudioProcessor::Biquad::setSecondOrderAllPass(double sr, float f, float q) noexcept
{
    const float k = std::tan(juce::MathConstants<float>::pi * safeFrequency(sr, f) / (float)sr);
    const float k2 = k * k;
    const float n = 1.0f / (1.0f + k / q + k2);
    a1 = 2.0f * (k2 - 1.0f) * n;
    a2 = (1.0f - k / q + k2) * n;
    b0 = a2; b1 = a1; b2 = 1.0f;
}

void PRISMVSTAudioProcessor::LR6Splitter::reset() noexcept
{
    for (auto& f : low) f.reset();
    for (auto& f : high) f.reset();
}

void PRISMVSTAudioProcessor::LR6Splitter::setCutoff(double sr, float f) noexcept
{
    // One 3rd-order Butterworth = first-order + second-order Q=1.
    // Squaring it produces Linkwitz-Riley 6th order: 36 dB/oct and -6 dB at Fc.
    low[0].setFirstOrderLowPass(sr, f);
    low[1].setSecondOrderLowPass(sr, f, 1.0f);
    low[2].setFirstOrderLowPass(sr, f);
    low[3].setSecondOrderLowPass(sr, f, 1.0f);

    high[0].setFirstOrderHighPass(sr, f);
    high[1].setSecondOrderHighPass(sr, f, 1.0f);
    high[2].setFirstOrderHighPass(sr, f);
    high[3].setSecondOrderHighPass(sr, f, 1.0f);
}

void PRISMVSTAudioProcessor::LR6Splitter::process(float x, float& lowOut, float& highOut) noexcept
{
    lowOut = x;
    highOut = x;
    for (auto& f : low) lowOut = f.process(lowOut);
    for (auto& f : high) highOut = f.process(highOut);

    // LR6 branches are 180 degrees apart. Polarity-correct the high branch so
    // their electrical sum is the matching all-pass response.
    highOut = -highOut;
}

void PRISMVSTAudioProcessor::ThirdOrderAllPass::reset() noexcept
{
    first.reset();
    second.reset();
}

void PRISMVSTAudioProcessor::ThirdOrderAllPass::setCutoff(double sr, float f) noexcept
{
    first.setFirstOrderAllPass(sr, f);
    second.setSecondOrderAllPass(sr, f, 1.0f);
}

float PRISMVSTAudioProcessor::ThirdOrderAllPass::process(float x) noexcept
{
    return second.process(first.process(x));
}

//==============================================================================
PRISMVSTAudioProcessor::PRISMVSTAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& v : spectrum)
        v.store(kFloorDb);

    for (int s = 0; s < numSections; ++s)
    {
        stereoActivity[(size_t)s].store(0.0f);
        soloParameterIds[(size_t)s] = sectionId(s, "solo");
        apvts.addParameterListener(soloParameterIds[(size_t)s], this);
    }
}

PRISMVSTAudioProcessor::~PRISMVSTAudioProcessor()
{
    cancelPendingUpdate();
    for (const auto& id : soloParameterIds)
        apvts.removeParameterListener(id, this);

    analyzerWorker.signalThreadShouldExit();
    analyzerEvent.signal();
    analyzerWorker.stopThread(1000);
}

int PRISMVSTAudioProcessor::getActiveSoloSection() const noexcept
{
    const int recent = latestSoloSection.load();
    if (recent >= 0 && recent < numSections
        && read(apvts, soloParameterIds[(size_t)recent]) >= 0.5f)
        return recent;

    for (int s = 0; s < numSections; ++s)
        if (read(apvts, soloParameterIds[(size_t)s]) >= 0.5f)
            return s;

    return -1;
}

void PRISMVSTAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    for (int s = 0; s < numSections; ++s)
    {
        if (parameterID != soloParameterIds[(size_t)s])
            continue;

        if (newValue >= 0.5f)
            latestSoloSection.store(s);
        else if (latestSoloSection.load() == s)
            latestSoloSection.store(-1);

        // The parameter callback can arrive on the audio thread. Never
        // notify the host or mutate other parameters from here.
        triggerAsyncUpdate();
        return;
    }
}

void PRISMVSTAudioProcessor::handleAsyncUpdate()
{
    reconcileExclusiveSolo();
}

void PRISMVSTAudioProcessor::reconcileExclusiveSolo()
{
    const int active = getActiveSoloSection();
    if (active < 0)
        return;

    for (int s = 0; s < numSections; ++s)
        if (s != active && read(apvts, soloParameterIds[(size_t)s]) >= 0.5f)
            if (auto* p = apvts.getParameter(soloParameterIds[(size_t)s]))
                p->setValueNotifyingHost(p->convertTo0to1(0.0f));
}

void PRISMVSTAudioProcessor::releaseResources()
{
    analyzerWorker.signalThreadShouldExit();
    analyzerEvent.signal();
    analyzerWorker.stopThread(1000);
}

bool PRISMVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PRISMVSTAudioProcessor::sanitizeCrossovers()
{
    constexpr float minRatio = 1.08f;
    float previous = 20.0f;

    for (int i = 0; i < numCrossovers; ++i)
    {
        const float remaining = (float)(numCrossovers - 1 - i);
        const float maxForIndex = 20000.0f / std::pow(minRatio, remaining + 1.0f);
        const float low = previous * minRatio;
        const float current = read(apvts, crossoverId(i));
        const float corrected = juce::jlimit(low, maxForIndex, current);

        if (auto* p = apvts.getParameter(crossoverId(i)))
            if (std::abs(corrected - current) > 0.001f)
                p->setValueNotifyingHost(p->convertTo0to1(corrected));

        previous = corrected;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout PRISMVSTAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    juce::NormalisableRange<float> crossoverRange(20.0f, 20000.0f, 0.01f);
    crossoverRange.setSkewForCentre(1000.0f);

    for (int i = 0; i < numCrossovers; ++i)
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { crossoverId(i), 1 },
            "Crossover " + juce::String(i + 1),
            crossoverRange, kDefaultCrossovers[(size_t)i]));

    juce::NormalisableRange<float> attackRange(0.10f, 250.0f, 0.01f);
    attackRange.setSkewForCentre(10.0f);
    juce::NormalisableRange<float> releaseRange(5.0f, 2500.0f, 0.1f);
    releaseRange.setSkewForCentre(180.0f);

    for (int i = 0; i < numSections; ++i)
    {
        const auto prefix = juce::String(sectionName(i)) + " ";

        p.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { sectionId(i, "on"), 1 }, prefix + "On", true));
        p.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { sectionId(i, "solo"), 1 }, prefix + "Solo", false));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "input_db"), 1 }, prefix + "Input",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "output_db"), 1 }, prefix + "Output",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "attack_ms"), 1 }, prefix + "Attack",
            attackRange, 10.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "release_ms"), 1 }, prefix + "Release",
            releaseRange, 180.0f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "width_pct"), 1 }, prefix + "Stereo Width",
            juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "onyx_pct"), 1 }, prefix + "ONYX",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

        // The transfer map owns these. They are intentionally not duplicated as knobs.
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "threshold_db"), 1 }, prefix + "Curve Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.01f), -18.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "ratio"), 1 }, prefix + "Curve Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.01f), 1.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "curve"), 1 }, prefix + "Curve Shape",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    }

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "analyzer_depth", 2 }, "Analyzer Depth",
        juce::StringArray { "FULL -96", "DEEP -120", "FORENSIC -144" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "analyzer_slope", 2 }, "Analyzer Display Slope",
        juce::NormalisableRange<float>(0.0f, 6.0f, 0.1f), 0.0f));

    juce::NormalisableRange<float> auraMemoryRange(0.05f, 10.0f, 0.01f);
    auraMemoryRange.setSkewForCentre(1.5f);
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "aura_memory", 2 }, "Heat Memory",
        auraMemoryRange, 1.5f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "aura_color", 1 }, "Frequency Color Heat", true));

    return { p.begin(), p.end() };
}

void PRISMVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // No lookahead or intentional buffering latency in PRISM.
    setLatencySamples(0);

    analyzerWorker.signalThreadShouldExit();
    analyzerEvent.signal();
    analyzerWorker.stopThread(1000);

    currentSampleRate = sampleRate;
    sanitizeCrossovers();

    for (auto& section : sectionStates)
    {
        section.envelope = 0.0f;
        section.gainReductionDb = 0.0f;
        section.onyxLow.fill(0.0f);
    }
    for (auto& v : stereoActivity)
        v.store(0.0f);

    for (int i = 0; i < numCrossovers; ++i)
    {
        const float f = read(apvts, crossoverId(i));
        crossoverSmoothers[(size_t)i].reset(sampleRate, 0.05);
        crossoverSmoothers[(size_t)i].setCurrentAndTargetValue(f);

        for (int c = 0; c < 2; ++c)
        {
            splitters[(size_t)i][(size_t)c].reset();
            splitters[(size_t)i][(size_t)c].setCutoff(sampleRate, f);
        }

        for (int j = 0; j < numSections; ++j)
            for (int c = 0; c < 2; ++c)
            {
                compensators[(size_t)i][(size_t)j][(size_t)c].reset();
                compensators[(size_t)i][(size_t)j][(size_t)c].setCutoff(sampleRate, f);
            }
    }

    juce::dsp::ProcessSpec monoSpec {
        sampleRate,
        static_cast<juce::uint32>(samplesPerBlock),
        1
    };

    auto hpC = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.0);
    auto shC = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1681.0, 0.7071f, toGain(4.0f));

    for (int c = 0; c < 2; ++c)
    {
        loudHp[(size_t)c].reset();
        loudHp[(size_t)c].prepare(monoSpec);
        loudShelf[(size_t)c].reset();
        loudShelf[(size_t)c].prepare(monoSpec);
        *loudHp[(size_t)c].coefficients = *hpC;
        *loudShelf[(size_t)c].coefficients = *shC;
    }

    analyzerFifo.reset();
    analyzerQueue.fill(0.0f);
    fftWritePos = 0;
    fftHopCounter = 0;
    fftFifo.fill(0.0f);
    fftData.fill(0.0f);
    for (auto& v : spectrum)
        v.store(kFloorDb);

    integratedEnergy = 0.0;
    integratedSamples = 0;
    analyzerWorker.startThread();
}

void PRISMVSTAudioProcessor::updateCrossoverCoefficients(int blockSamples)
{
    for (int i = 0; i < numCrossovers; ++i)
    {
        auto& smoother = crossoverSmoothers[(size_t)i];
        smoother.setTargetValue(read(apvts, crossoverId(i)));
        const float f = smoother.skip(blockSamples);

        for (int c = 0; c < 2; ++c)
            splitters[(size_t)i][(size_t)c].setCutoff(currentSampleRate, f);

        for (int j = 0; j < i; ++j)
            for (int c = 0; c < 2; ++c)
                compensators[(size_t)i][(size_t)j][(size_t)c].setCutoff(currentSampleRate, f);
    }
}

void PRISMVSTAudioProcessor::processSevenSectionEngine(juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    updateCrossoverCoefficients(n);

    std::array<bool, numSections> on {};
    // SOLO is exclusive: the most recently activated section wins.
    // Any stale overlapping host automation is audibly resolved immediately,
    // then reconciled to one true SOLO parameter on the message thread.
    const int activeSolo = getActiveSoloSection();

    std::array<float, numSections> inputGain {}, outputGain {}, attackCoeff {}, releaseCoeff {};
    std::array<float, numSections> width {}, onyx {}, threshold {}, ratio {}, curve {};

    const float sr = (float)currentSampleRate;

    for (int s = 0; s < numSections; ++s)
    {
        on[(size_t)s] = read(apvts, sectionId(s, "on")) >= 0.5f;
        inputGain[(size_t)s] = toGain(read(apvts, sectionId(s, "input_db")));
        outputGain[(size_t)s] = toGain(read(apvts, sectionId(s, "output_db")));

        const float attackMs = juce::jmax(0.10f, read(apvts, sectionId(s, "attack_ms")));
        const float releaseMs = juce::jmax(5.0f, read(apvts, sectionId(s, "release_ms")));
        attackCoeff[(size_t)s] = std::exp(-1.0f / (0.001f * attackMs * sr));
        releaseCoeff[(size_t)s] = std::exp(-1.0f / (0.001f * releaseMs * sr));

        // Preserve the v1.1.1 parameter ID/range for project recall.
        // 100% stored = neutral; 150% stored = maximum +50% widening.
        // Old values outside that territory never narrow or over-expand.
        width[(size_t)s] = juce::jlimit(1.0f, 1.5f,
            read(apvts, sectionId(s, "width_pct")) * 0.01f);
        onyx[(size_t)s] = juce::jlimit(0.0f, 1.0f, read(apvts, sectionId(s, "onyx_pct")) * 0.01f);
        threshold[(size_t)s] = read(apvts, sectionId(s, "threshold_db"));
        ratio[(size_t)s] = juce::jlimit(1.0f, 20.0f, read(apvts, sectionId(s, "ratio")));
        curve[(size_t)s] = juce::jlimit(-1.0f, 1.0f, read(apvts, sectionId(s, "curve")));
    }

    float maxReduction = 0.0f;
    std::array<double, numSections> midEnergy {};
    std::array<double, numSections> sideEnergy {};
    // One-pole split for a very small input-derived high-frequency sheen.
    const float onyxToneCoeff = 1.0f - std::exp(
        -2.0f * juce::MathConstants<float>::pi * 1800.0f / sr);
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    for (int i = 0; i < n; ++i)
    {
        std::array<float, numSections> l {};
        std::array<float, numSections> r {};

        float remL = left[i];
        float remR = right[i];

        for (int x = 0; x < numCrossovers; ++x)
        {
            // Phase-compensate already extracted lower sections for this later split.
            for (int lower = 0; lower < x; ++lower)
            {
                l[(size_t)lower] = compensators[(size_t)x][(size_t)lower][0].process(l[(size_t)lower]);
                r[(size_t)lower] = compensators[(size_t)x][(size_t)lower][1].process(r[(size_t)lower]);
            }

            float lowL = 0.0f, highL = 0.0f;
            float lowR = 0.0f, highR = 0.0f;
            splitters[(size_t)x][0].process(remL, lowL, highL);
            splitters[(size_t)x][1].process(remR, lowR, highR);

            l[(size_t)x] = lowL;
            r[(size_t)x] = lowR;
            remL = highL;
            remR = highR;
        }

        l[(size_t)(numSections - 1)] = remL;
        r[(size_t)(numSections - 1)] = remR;

        float outL = 0.0f;
        float outR = 0.0f;

        for (int s = 0; s < numSections; ++s)
        {
            const bool audible = on[(size_t)s] && (activeSolo < 0 || s == activeSolo);
            if (!audible)
                continue;

            float xL = l[(size_t)s] * inputGain[(size_t)s];
            float xR = r[(size_t)s] * inputGain[(size_t)s];

            auto& state = sectionStates[(size_t)s];
            const float detected = juce::jmax(std::abs(xL), std::abs(xR));
            const float envCoeff = detected > state.envelope
                ? attackCoeff[(size_t)s] : releaseCoeff[(size_t)s];
            state.envelope = envCoeff * state.envelope + (1.0f - envCoeff) * detected;

            const float levelDb = toDb(state.envelope + 1.0e-9f);
            const float overDb = juce::jmax(0.0f, levelDb - threshold[(size_t)s]);
            const float exponent = juce::jmap(curve[(size_t)s], -1.0f, 1.0f, 0.65f, 1.75f);
            const float shapedOver = overDb > 0.0f
                ? 24.0f * std::pow(overDb / 24.0f, exponent)
                : 0.0f;
            state.gainReductionDb = shapedOver * (1.0f - 1.0f / ratio[(size_t)s]);
            maxReduction = juce::jmax(maxReduction, state.gainReductionDb);

            const float dynamicGain = toGain(-state.gainReductionDb);
            xL *= dynamicGain;
            xR *= dynamicGain;

            const float a = onyx[(size_t)s];
            // Resonant Wax: gentle differential soft clipping gives fine,
            // input-dependent harmonics without a loud saturation jump.
            // The high-passed component adds restrained musical sparkle
            // from signal content, never noise or an oscillator.
            const float drive = 1.0f + 0.9f * a;
            const float bias = 0.065f * a;
            const float zero = std::tanh(bias * drive);
            auto wax = [&](float sample, float& low)
            {
                low += onyxToneCoeff * (sample - low);
                if (a <= 0.0001f)
                    return sample; // bit-exact ONYX bypass

                const float coloured = (std::tanh((sample + bias) * drive) - zero) / drive;
                const float sheen = (sample - low) * (0.045f * a);
                const float mix = 0.40f * a;
                return (sample + mix * (coloured - sample) + sheen) * (1.0f - 0.015f * a);
            };
            xL = wax(xL, state.onyxLow[0]);
            xR = wax(xR, state.onyxLow[1]);

            const float mid = 0.5f * (xL + xR);
            const float side = 0.5f * (xL - xR) * width[(size_t)s];
            xL = (mid + side) * outputGain[(size_t)s];
            xR = (mid - side) * outputGain[(size_t)s];

            // Meter actual processed stereo activity, not knob position.
            // This feeds only the screen; never feeds any DSP decision.
            const double outMid = 0.5 * ((double)xL + (double)xR);
            const double outSide = 0.5 * ((double)xL - (double)xR);
            midEnergy[(size_t)s] += outMid * outMid;
            sideEnergy[(size_t)s] += outSide * outSide;

            outL += xL;
            outR += xR;
        }

        left[i] = outL;
        right[i] = outR;
    }

    gainReductionDb.store(maxReduction);
    for (int s = 0; s < numSections; ++s)
    {
        const double m = midEnergy[(size_t)s];
        const double side = sideEnergy[(size_t)s];
        const float measured = (float)std::sqrt(side / (side + m + 1.0e-15));
        const float previous = stereoActivity[(size_t)s].load();
        stereoActivity[(size_t)s].store(0.72f * previous + 0.28f * measured);
    }
}

void PRISMVSTAudioProcessor::pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer)
{
    const int requested = buffer.getNumSamples();
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    analyzerFifo.prepareToWrite(requested, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        analyzerQueue[(size_t)(start1 + i)] = 0.5f * (left[i] + right[i]);

    for (int i = 0; i < size2; ++i)
    {
        const int source = size1 + i;
        analyzerQueue[(size_t)(start2 + i)] = 0.5f * (left[source] + right[source]);
    }

    const int written = size1 + size2;
    analyzerFifo.finishedWrite(written);
    if (written > 0)
        analyzerEvent.signal();
}

void PRISMVSTAudioProcessor::runAnalyzerThread()
{
    while (!analyzerWorker.threadShouldExit())
    {
        analyzerEvent.wait(10);
        for (;;)
        {
            const int ready = analyzerFifo.getNumReady();
            if (ready <= 0)
                break;

            const int amount = juce::jmin(ready, 4096);
            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            analyzerFifo.prepareToRead(amount, start1, size1, start2, size2);

            for (int i = 0; i < size1; ++i)
                consumeAnalyzerSample(analyzerQueue[(size_t)(start1 + i)]);
            for (int i = 0; i < size2; ++i)
                consumeAnalyzerSample(analyzerQueue[(size_t)(start2 + i)]);

            analyzerFifo.finishedRead(size1 + size2);
            if (analyzerWorker.threadShouldExit())
                break;
        }
    }
}

void PRISMVSTAudioProcessor::consumeAnalyzerSample(float sample)
{
    fftFifo[(size_t)fftWritePos] = sample;
    fftWritePos = (fftWritePos + 1) % fftSize;

    if (++fftHopCounter >= fftHopSize)
    {
        renderSpectrumFrame();
        fftHopCounter = 0;
    }
}

void PRISMVSTAudioProcessor::renderSpectrumFrame()
{
    for (int i = 0; i < fftSize; ++i)
    {
        const int source = (fftWritePos + i) % fftSize;
        fftData[(size_t)i] = fftFifo[(size_t)source];
    }

    for (int i = fftSize; i < fftSize * 2; ++i)
        fftData[(size_t)i] = 0.0f;

    fftWindow.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const int maxBin = fftSize / 2;
    const double sr = juce::jmax(1.0, currentSampleRate);

    for (int i = 0; i < spectrumBins; ++i)
    {
        const float centrePos = (float)i / (float)(spectrumBins - 1);
        const float loPos = juce::jlimit(0.0f, 1.0f, ((float)i - 0.5f) / (float)(spectrumBins - 1));
        const float hiPos = juce::jlimit(0.0f, 1.0f, ((float)i + 0.5f) / (float)(spectrumBins - 1));

        const float centreHz = 20.0f * std::pow(1000.0f, centrePos);
        const float loHz = 20.0f * std::pow(1000.0f, loPos);
        const float hiHz = 20.0f * std::pow(1000.0f, hiPos);

        const int loBin = juce::jlimit(1, maxBin, (int)std::floor(loHz * (double)fftSize / sr));
        const int hiBin = juce::jlimit(loBin, maxBin, (int)std::ceil(hiHz * (double)fftSize / sr));

        float peakMagnitude = 0.0f;
        for (int bin = loBin; bin <= hiBin; ++bin)
            peakMagnitude = juce::jmax(peakMagnitude, (2.0f * fftData[(size_t)bin]) / (float)fftSize);

        if (peakMagnitude <= 0.0f)
        {
            const int nearest = juce::jlimit(1, maxBin,
                juce::roundToInt(centreHz * (float)fftSize / (float)sr));
            peakMagnitude = (2.0f * fftData[(size_t)nearest]) / (float)fftSize;
        }

        spectrum[(size_t)i].store(toDb(peakMagnitude + 1.0e-12f));
    }
}

void PRISMVSTAudioProcessor::copySpectrum(std::array<float, spectrumBins>& destination) const noexcept
{
    for (int i = 0; i < spectrumBins; ++i)
        destination[(size_t)i] = spectrum[(size_t)i].load();
}

void PRISMVSTAudioProcessor::updateMeters(const juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    float peak = 0.0f;
    double weightedEnergy = 0.0;

    for (int c = 0; c < juce::jmin(2, buffer.getNumChannels()); ++c)
    {
        const float* x = buffer.getReadPointer(c);
        for (int i = 0; i < n; ++i)
        {
            peak = juce::jmax(peak, std::abs(x[i]));
            float y = loudHp[(size_t)c].processSample(x[i]);
            y = loudShelf[(size_t)c].processSample(y);
            weightedEnergy += (double)y * (double)y;
        }
    }

    peakDb.store(toDb(peak + 1.0e-9f));

    const auto count = (uint64_t)n * 2u;
    integratedEnergy += weightedEnergy;
    integratedSamples += count;

    const double blockMean = count > 0 ? weightedEnergy / (double)count : 0.0;
    const double intMean = integratedSamples > 0 ? integratedEnergy / (double)integratedSamples : 0.0;

    lufsShort.store(blockMean > 1.0e-12
        ? (float)(-0.691 + 10.0 * std::log10(blockMean)) : kFloorDb);
    lufsIntegrated.store(intMean > 1.0e-12
        ? (float)(-0.691 + 10.0 * std::log10(intMean)) : kFloorDb);
}

void PRISMVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());

    float inputPeak = 0.0f;
    for (int c = 0; c < juce::jmin(2, buffer.getNumChannels()); ++c)
        inputPeak = juce::jmax(inputPeak, buffer.getMagnitude(c, 0, buffer.getNumSamples()));
    inputPeakDb.store(toDb(inputPeak + 1.0e-9f));

    processSevenSectionEngine(buffer);
    pushAnalyzerSamples(buffer);
    updateMeters(buffer);
}

void PRISMVSTAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void PRISMVSTAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
            sanitizeCrossovers();
            // Resolve old sessions that persisted more than one SOLO flag.
            triggerAsyncUpdate();
        }
}

juce::AudioProcessorEditor* PRISMVSTAudioProcessor::createEditor()
{
    return new PRISMVSTAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PRISMVSTAudioProcessor();
}
