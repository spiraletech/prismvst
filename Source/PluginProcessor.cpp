#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float kFloorDb = -144.0f;
constexpr std::array<float, PRISMVSTAudioProcessor::numEqBands> kDefaultFreq {
    45.0f, 100.0f, 280.0f, 1200.0f, 6500.0f
};

inline float toDb(float g) { return juce::Decibels::gainToDecibels(g, kFloorDb); }
inline float toGain(float d) { return juce::Decibels::decibelsToGain(d); }
}

PRISMVSTAudioProcessor::PRISMVSTAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& v : spectrum)
        v.store(kFloorDb);
}

bool PRISMVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

juce::String PRISMVSTAudioProcessor::bandId(int band, const juce::String& suffix)
{
    return "band" + juce::String(band + 1) + "_" + suffix;
}

float PRISMVSTAudioProcessor::read(const juce::AudioProcessorValueTreeState& state, const juce::String& id)
{
    if (auto* v = state.getRawParameterValue(id))
        return v->load();
    return 0.0f;
}

juce::AudioProcessorValueTreeState::ParameterLayout PRISMVSTAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 0.01f);
    freqRange.setSkewForCentre(1000.0f);

    juce::NormalisableRange<float> qRange(0.10f, 20.0f, 0.01f);
    qRange.setSkewForCentre(1.0f);

    juce::NormalisableRange<float> attackRange(0.10f, 200.0f, 0.01f);
    attackRange.setSkewForCentre(10.0f);

    juce::NormalisableRange<float> releaseRange(5.0f, 1000.0f, 0.1f);
    releaseRange.setSkewForCentre(120.0f);

    for (int i = 0; i < numEqBands; ++i)
    {
        const auto prefix = "Band " + juce::String(i + 1) + " ";

        p.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { bandId(i, "enabled"), 2 }, prefix + "Enabled", true));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "freq"), 2 }, prefix + "Frequency",
            freqRange, kDefaultFreq[(size_t)i]));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "gain"), 2 }, prefix + "Gain",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.01f), 0.0f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "q"), 2 }, prefix + "Q",
            qRange, 0.85f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "dyn_range"), 2 }, prefix + "Dynamic Range",
            juce::NormalisableRange<float>(0.0f, 18.0f, 0.01f), 0.0f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "threshold"), 2 }, prefix + "Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.01f), -18.0f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "ratio"), 2 }, prefix + "Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.01f), 2.0f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "attack"), 2 }, prefix + "Attack",
            attackRange, 10.0f));

        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { bandId(i, "release"), 2 }, prefix + "Release",
            releaseRange, 120.0f));
    }

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "onyx", 2 }, "ONYX",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "onyx_drive", 2 }, "ONYX Drive",
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.01f), 3.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "onyx_bias", 2 }, "ONYX Bias",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.05f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "onyx_density", 2 }, "ONYX Density",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "solfeggio_grid", 3 }, "Solfeggio Aura", true));

    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "analyzer_depth", 1 }, "Analyzer Depth",
        juce::StringArray { "MIX -36", "DEEP -72", "FORENSIC -120" }, 0));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "analyzer_slope", 1 }, "Analyzer Slope",
        juce::NormalisableRange<float>(0.0f, 6.0f, 0.1f), 4.5f));

    juce::NormalisableRange<float> auraMemoryRange(0.05f, 10.0f, 0.01f);
    auraMemoryRange.setSkewForCentre(1.5f);
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "aura_memory", 1 }, "Aura Memory",
        auraMemoryRange, 1.5f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "color_affinity", 1 }, "Color Affinity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.65f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "master_trim", 1 }, "Master Trim",
        juce::NormalisableRange<float>(-18.0f, 12.0f, 0.01f), 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ceiling", 1 }, "Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.01f), -1.0f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "master_bypass", 1 }, "Master Bypass", false));

    return { p.begin(), p.end() };
}

void PRISMVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec monoSpec {
        sampleRate,
        static_cast<juce::uint32>(samplesPerBlock),
        1
    };

    for (auto& band : bands)
    {
        band.envelope = 0.0f;
        band.smoothedGainDb = 0.0f;

        for (int c = 0; c < 2; ++c)
        {
            band.eq[(size_t)c].reset();
            band.eq[(size_t)c].prepare(monoSpec);
            band.detector[(size_t)c].reset();
            band.detector[(size_t)c].prepare(monoSpec);
        }
    }

    for (int i = 0; i < numEqBands; ++i)
        updateBandCoefficients(i);

    auto hpC = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.0);
    auto shC = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1681.0, 0.7071f, toGain(4.0f));

    for (int c = 0; c < 2; ++c)
    {
        loudHp[(size_t)c].reset();
        loudShelf[(size_t)c].reset();
        *loudHp[(size_t)c].coefficients = *hpC;
        *loudShelf[(size_t)c].coefficients = *shC;
    }

    fftWritePos = 0;
    fftData.fill(0.0f);
    for (auto& v : spectrum)
        v.store(kFloorDb);

    integratedEnergy = 0.0;
    integratedSamples = 0;
}

void PRISMVSTAudioProcessor::updateBandCoefficients(int bandIndex)
{
    auto& band = bands[(size_t)bandIndex];

    const float freq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45),
                                   read(apvts, bandId(bandIndex, "freq")));
    const float q = juce::jlimit(0.10f, 20.0f, read(apvts, bandId(bandIndex, "q")));
    const float staticGain = read(apvts, bandId(bandIndex, "gain"));

    const float effectiveGain = staticGain + band.smoothedGainDb;

    auto eqC = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        currentSampleRate, freq, q, toGain(effectiveGain));

    auto detectorC = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        currentSampleRate, freq, q);

    for (int c = 0; c < 2; ++c)
    {
        *band.eq[(size_t)c].coefficients = *eqC;
        *band.detector[(size_t)c].coefficients = *detectorC;
    }
}

void PRISMVSTAudioProcessor::processDynamicEq(juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();

    for (int b = 0; b < numEqBands; ++b)
    {
        if (read(apvts, bandId(b, "enabled")) < 0.5f)
            continue;

        auto& band = bands[(size_t)b];

        const float threshold = read(apvts, bandId(b, "threshold"));
        const float dynamicRange = read(apvts, bandId(b, "dyn_range"));
        const float ratio = juce::jmax(1.0f, read(apvts, bandId(b, "ratio")));
        const float attackMs = juce::jmax(0.10f, read(apvts, bandId(b, "attack")));
        const float releaseMs = juce::jmax(5.0f, read(apvts, bandId(b, "release")));

        const float attackCoeff = std::exp(-1.0f / (0.001f * attackMs * (float)currentSampleRate));
        const float releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * (float)currentSampleRate));

        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);

        for (int i = 0; i < n; ++i)
        {
            const float dl = band.detector[0].processSample(left[i]);
            const float dr = band.detector[1].processSample(right[i]);
            const float detected = juce::jmax(std::abs(dl), std::abs(dr));

            const float coeff = detected > band.envelope ? attackCoeff : releaseCoeff;
            band.envelope = coeff * band.envelope + (1.0f - coeff) * detected;
        }

        const float levelDb = toDb(band.envelope + 1.0e-9f);
        const float overDb = juce::jmax(0.0f, levelDb - threshold);
        const float compressedDb = overDb * (1.0f - 1.0f / ratio);
        const float targetDynamicDb = -juce::jmin(dynamicRange, compressedDb);

        band.smoothedGainDb += 0.35f * (targetDynamicDb - band.smoothedGainDb);
        updateBandCoefficients(b);

        for (int c = 0; c < 2; ++c)
        {
            float* x = buffer.getWritePointer(c);
            auto& filter = band.eq[(size_t)c];

            for (int i = 0; i < n; ++i)
                x[i] = filter.processSample(x[i]);
        }
    }
}

void PRISMVSTAudioProcessor::processOnyx(juce::AudioBuffer<float>& buffer)
{
    const float amount = read(apvts, "onyx");
    if (amount <= 0.0001f)
        return;

    const float driveDb = read(apvts, "onyx_drive");
    const float drive = toGain(driveDb) * (1.0f + amount * 1.75f);
    const float bias = read(apvts, "onyx_bias") * 0.18f * amount;
    const float density = read(apvts, "onyx_density");
    const float wet = juce::jlimit(0.0f, 1.0f, amount * (0.55f + 0.45f * density));
    const float norm = std::tanh(drive + std::abs(bias));

    for (int c = 0; c < juce::jmin(2, buffer.getNumChannels()); ++c)
    {
        float* x = buffer.getWritePointer(c);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float dry = x[i];
            float shaped = std::tanh(dry * drive + bias) / juce::jmax(0.001f, norm);
            shaped -= std::tanh(bias) / juce::jmax(0.001f, norm);

            const float dense = 0.5f * (shaped + std::tanh(shaped * (1.0f + density * 2.5f)));
            x[i] = dry + wet * (dense - dry);
        }
    }
}

void PRISMVSTAudioProcessor::pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    for (int i = 0; i < n; ++i)
    {
        fftData[(size_t)fftWritePos++] = 0.5f * (left[i] + right[i]);

        if (fftWritePos >= fftSize)
        {
            renderSpectrumFrame();
            fftWritePos = 0;
        }
    }
}

void PRISMVSTAudioProcessor::renderSpectrumFrame()
{
    for (int i = fftSize; i < fftSize * 2; ++i)
        fftData[(size_t)i] = 0.0f;

    fftWindow.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const int maxBin = fftSize / 2;

    for (int i = 0; i < spectrumBins; ++i)
    {
        const float pos = (float)i / (float)(spectrumBins - 1);
        const int bin = juce::jlimit(0, maxBin, juce::roundToInt(pos * (float)maxBin));
        const float magnitude = fftData[(size_t)bin] / (float)fftSize;
        spectrum[(size_t)i].store(toDb(magnitude + 1.0e-9f));
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
        ? (float)(-0.691 + 10.0 * std::log10(blockMean))
        : kFloorDb);

    lufsIntegrated.store(intMean > 1.0e-12
        ? (float)(-0.691 + 10.0 * std::log10(intMean))
        : kFloorDb);
}

void PRISMVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());

    if (read(apvts, "master_bypass") > 0.5f)
    {
        pushAnalyzerSamples(buffer);
        updateMeters(buffer);
        return;
    }

    processDynamicEq(buffer);
    processOnyx(buffer);

    buffer.applyGain(toGain(read(apvts, "master_trim")));

    const float ceiling = toGain(read(apvts, "ceiling"));

    for (int c = 0; c < juce::jmin(2, buffer.getNumChannels()); ++c)
    {
        float* x = buffer.getWritePointer(c);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            x[i] = juce::jlimit(-ceiling, ceiling, x[i]);
    }

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
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* PRISMVSTAudioProcessor::createEditor()
{
    return new PRISMVSTAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PRISMVSTAudioProcessor();
}
