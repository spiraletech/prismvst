#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float kFloorDb = -100.0f;
constexpr std::array<float, PRISMVSTAudioProcessor::numEqBands> kDefaultFreq {
    60.0f, 120.0f, 400.0f, 1200.0f, 4000.0f, 10000.0f
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
    const char* suffixes[] = { "enabled", "freq", "gain", "q", "dyn_range",
                              "threshold", "ratio", "attack", "release" };
    for (int b = 0; b < numEqBands; ++b)
        for (int j = 0; j < 9; ++j)
            bands[(size_t)b].parameters[(size_t)j] = apvts.getRawParameterValue(bandId(b, suffixes[j]));
    const char* globalIds[] = { "onyx", "onyx_drive", "onyx_bias", "onyx_density", "master_trim", "ceiling", "master_bypass" };
    for (size_t i = 0; i < globals.size(); ++i)
        globals[i] = apvts.getRawParameterValue(globalIds[i]);
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
        juce::ParameterID { "solfeggio_grid", 2 }, "Solfeggio Grid", false));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "master_trim", 1 }, "Master Trim",
        juce::NormalisableRange<float>(-18.0f, 12.0f, 0.01f), 0.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ceiling", 1 }, "Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.01f), -0.3f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "master_bypass", 1 }, "Master Bypass", false));

    return { p.begin(), p.end() };
}

void PRISMVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    trimGain.reset(sampleRate, 0.02);
    trimGain.setCurrentAndTargetValue(toGain(read(apvts, "master_trim")));
    const char* onyxIds[] = { "onyx", "onyx_drive", "onyx_bias", "onyx_density" };
    for (size_t i = 0; i < onyxControls.size(); ++i)
    {
        onyxControls[i].reset(sampleRate, 0.02);
        onyxControls[i].setCurrentAndTargetValue(read(apvts, onyxIds[i]));
    }

    juce::dsp::ProcessSpec monoSpec {
        sampleRate,
        static_cast<juce::uint32>(samplesPerBlock),
        1
    };

    for (auto& band : bands)
    {
        band.envelope = 0.0f;
        band.smoothedGainDb = 0.0f;
        band.controlCountdown = 0;
        band.frequency = band.parameters[1]->load();
        band.q = band.parameters[3]->load();
        band.gain = band.parameters[2]->load();
        band.enabledMix.reset(sampleRate, 0.005);
        band.enabledMix.setCurrentAndTargetValue(band.parameters[0]->load() >= 0.5f ? 1.0f : 0.0f);
        updateBandCoefficients((int) (&band - bands.data()));

        for (int c = 0; c < 2; ++c)
        {
            band.eq[(size_t)c].reset();
            band.eq[(size_t)c].prepare(monoSpec);
            band.detector[(size_t)c].reset();
            band.detector[(size_t)c].prepare(monoSpec);
        }
    }

    auto hpC = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.0);
    auto shC = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1681.0, 0.7071f, toGain(4.0f));

    for (int c = 0; c < 2; ++c)
    {
        loudHp[(size_t)c].reset();
        loudShelf[(size_t)c].reset();
        *loudHp[(size_t)c].coefficients = *hpC;
        *loudShelf[(size_t)c].coefficients = *shC;
        loudHp[(size_t)c].prepare(monoSpec);
        loudShelf[(size_t)c].prepare(monoSpec);
    }

    peakDb.store(kFloorDb);
    lufsShort.store(kFloorDb);
    lufsIntegrated.store(kFloorDb);
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

    const float smoothing = 1.0f - std::exp(-32.0f / (0.010f * (float)currentSampleRate));
    band.frequency += smoothing * (band.parameters[1]->load() - band.frequency);
    band.q += smoothing * (band.parameters[3]->load() - band.q);
    band.gain += smoothing * (band.parameters[2]->load() - band.gain);
    const float freq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), band.frequency);
    const float q = juce::jlimit(0.10f, 20.0f, band.q);
    const float staticGain = band.gain;

    const float effectiveGain = staticGain + band.smoothedGainDb;

    auto eqC = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(
        currentSampleRate, freq, q, toGain(effectiveGain));

    auto detectorC = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(
        currentSampleRate, freq, q);

    for (int c = 0; c < 2; ++c)
    {
        *band.eq[(size_t)c].coefficients = eqC;
        *band.detector[(size_t)c].coefficients = detectorC;
    }
}

void PRISMVSTAudioProcessor::processDynamicEq(juce::AudioBuffer<float>& buffer)
{
    // Fixed sample cadence keeps dynamics independent of host buffer boundaries.
    for (auto& band : bands)
    {
        const bool enabled = band.parameters[0]->load() >= 0.5f;
        band.enabledMix.setTargetValue(enabled ? 1.0f : 0.0f);
        const float threshold = band.parameters[5]->load();
        const float range = band.parameters[4]->load();
        const float ratio = juce::jmax(1.0f, band.parameters[6]->load());
        const float attack = std::exp(-1.0f / (0.001f * band.parameters[7]->load() * (float)currentSampleRate));
        const float release = std::exp(-1.0f / (0.001f * band.parameters[8]->load() * (float)currentSampleRate));
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (band.controlCountdown-- <= 0)
            {
                updateBandCoefficients((int) (&band - bands.data()));
                band.controlCountdown = 31;
            }
            const float detected = juce::jmax(std::abs(band.detector[0].processSample(left[i])),
                                             std::abs(band.detector[1].processSample(right[i])));
            const float coeff = detected > band.envelope ? attack : release;
            band.envelope = coeff * band.envelope + (1.0f - coeff) * detected;
            const float target = -juce::jmin(range, juce::jmax(0.0f, toDb(band.envelope + 1.0e-9f) - threshold)
                                                       * (1.0f - 1.0f / ratio));
            const float gainCoeff = target < band.smoothedGainDb ? attack : release;
            band.smoothedGainDb = gainCoeff * band.smoothedGainDb + (1.0f - gainCoeff) * target;
            // Keep disabled filters warm so re-enabling does not replay stale state.
            const float filteredLeft = band.eq[0].processSample(left[i]);
            const float filteredRight = band.eq[1].processSample(right[i]);
            const float mix = band.enabledMix.getNextValue();
            left[i] += mix * (filteredLeft - left[i]);
            right[i] += mix * (filteredRight - right[i]);
        }
    }
}

void PRISMVSTAudioProcessor::processOnyx(juce::AudioBuffer<float>& buffer)
{
    for (size_t i = 0; i < onyxControls.size(); ++i)
        onyxControls[i].setTargetValue(globals[i]->load());
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float amount = onyxControls[0].getNextValue();
        const float drive = toGain(onyxControls[1].getNextValue()) * (1.0f + amount * 1.75f);
        const float bias = onyxControls[2].getNextValue() * 0.18f * amount;
        const float density = onyxControls[3].getNextValue();
        if (amount <= 0.0001f) continue;
        const float wet = amount * (0.55f + 0.45f * density);
        const float norm = juce::jmax(0.001f, std::tanh(drive + std::abs(bias)));
        const float offset = std::tanh(bias) / norm;
        for (auto* channel : { left, right })
        {
            const float dry = channel[i];
            const float shaped = std::tanh(dry * drive + bias) / norm - offset;
            const float dense = 0.5f * (shaped + std::tanh(shaped * (1.0f + density * 2.5f)));
            channel[i] = dry + wet * (dense - dry);
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
        const float magnitude = fftData[(size_t)bin] * (2.0f / (float)fftSize);
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
    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() < 2)
        return;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());

    if (globals[6]->load() > 0.5f)
    {
        pushAnalyzerSamples(buffer);
        updateMeters(buffer);
        return;
    }

    processDynamicEq(buffer);
    processOnyx(buffer);

    trimGain.setTargetValue(toGain(globals[4]->load()));
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float gain = trimGain.getNextValue();
        for (int c = 0; c < 2; ++c)
            buffer.getWritePointer(c)[i] *= gain;
    }

    const float ceiling = toGain(globals[5]->load());

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
