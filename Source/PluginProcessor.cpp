#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float kAnalyzerFloorDb = -144.0f;
constexpr float kOnyxThresholdDb = -18.0f;
constexpr float kMaxOnyxReductionDb = 12.0f;
constexpr std::array<float, 3> kButterworth6Q {
    0.51763809f, 0.70710678f, 1.93185165f
};

inline float dbToGain(float db)
{
    return juce::Decibels::decibelsToGain(db);
}

inline float gainToDb(float gain)
{
    return juce::Decibels::gainToDecibels(gain, kAnalyzerFloorDb);
}
}

PRISMVSTAudioProcessor::PRISMVSTAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& value : spectrum)
        value.store(kAnalyzerFloorDb);
}

bool PRISMVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

juce::String PRISMVSTAudioProcessor::sectionId(int section, const juce::String& suffix)
{
    return "section" + juce::String(section + 1) + "_" + suffix;
}

float PRISMVSTAudioProcessor::read(const juce::AudioProcessorValueTreeState& state,
                                   const juce::String& id)
{
    if (auto* value = state.getRawParameterValue(id))
        return value->load();
    return 0.0f;
}

juce::AudioProcessorValueTreeState::ParameterLayout PRISMVSTAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::NormalisableRange<float> inputRange(-18.0f, 18.0f, 0.01f);
    juce::NormalisableRange<float> outputRange(-18.0f, 18.0f, 0.01f);
    juce::NormalisableRange<float> attackRange(0.10f, 250.0f, 0.01f);
    attackRange.setSkewForCentre(10.0f);
    juce::NormalisableRange<float> releaseRange(5.0f, 2000.0f, 0.1f);
    releaseRange.setSkewForCentre(180.0f);

    for (int i = 0; i < numSections; ++i)
    {
        const auto prefix = "Section " + juce::String(i + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { sectionId(i, "enabled"), 3 }, prefix + "Enabled", true));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { sectionId(i, "solo"), 3 }, prefix + "Solo", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "input"), 3 }, prefix + "Input",
            inputRange, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "output"), 3 }, prefix + "Output",
            outputRange, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "attack"), 3 }, prefix + "Attack",
            attackRange, 10.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "release"), 3 }, prefix + "Release",
            releaseRange, 180.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "width"), 3 }, prefix + "Width",
            juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { sectionId(i, "onyx"), 3 }, prefix + "Onyx",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    }

    return { params.begin(), params.end() };
}

void PRISMVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentMaxBlockSize = juce::jmax(1, samplesPerBlock);

    dryScratch.setSize(2, currentMaxBlockSize, false, false, true);
    bandScratch.setSize(2, currentMaxBlockSize, false, false, true);

    juce::dsp::ProcessSpec monoSpec {
        sampleRate,
        static_cast<juce::uint32>(currentMaxBlockSize),
        1
    };

    for (auto& section : sections)
    {
        section.envelope = 0.0f;
        section.dynamicGainDb = 0.0f;

        for (int channel = 0; channel < 2; ++channel)
        {
            for (int stage = 0; stage < filterStages; ++stage)
            {
                section.highPass[(size_t)channel][(size_t)stage].prepare(monoSpec);
                section.lowPass[(size_t)channel][(size_t)stage].prepare(monoSpec);
            }
        }
    }

    configureSectionFilters();
    resetSectionFilters();

    fftWritePos = 0;
    fftData.fill(0.0f);
    for (auto& value : spectrum)
        value.store(kAnalyzerFloorDb);

    inputPeakDb.store(kAnalyzerFloorDb);
    outputPeakDb.store(kAnalyzerFloorDb);
    gainReductionDb.store(0.0f);
}

void PRISMVSTAudioProcessor::configureSectionFilters()
{
    const auto nyquistSafe = static_cast<float>(currentSampleRate * 0.45);

    for (int sectionIndex = 0; sectionIndex < numSections; ++sectionIndex)
    {
        const float lowEdge = sectionIndex == 0
            ? 20.0f
            : splitFrequencies[(size_t)(sectionIndex - 1)];
        const float highEdge = sectionIndex == numSections - 1
            ? juce::jmin(20000.0f, nyquistSafe)
            : splitFrequencies[(size_t)sectionIndex];

        auto& section = sections[(size_t)sectionIndex];

        for (int stage = 0; stage < filterStages; ++stage)
        {
            const auto q = kButterworth6Q[(size_t)stage];

            if (sectionIndex > 0)
            {
                auto coeff = juce::dsp::IIR::Coefficients<float>::makeHighPass(
                    currentSampleRate, juce::jmin(lowEdge, nyquistSafe), q);
                for (int channel = 0; channel < 2; ++channel)
                    *section.highPass[(size_t)channel][(size_t)stage].coefficients = *coeff;
            }

            if (sectionIndex < numSections - 1)
            {
                auto coeff = juce::dsp::IIR::Coefficients<float>::makeLowPass(
                    currentSampleRate, juce::jmin(highEdge, nyquistSafe), q);
                for (int channel = 0; channel < 2; ++channel)
                    *section.lowPass[(size_t)channel][(size_t)stage].coefficients = *coeff;
            }
        }
    }
}

void PRISMVSTAudioProcessor::resetSectionFilters()
{
    for (auto& section : sections)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            for (int stage = 0; stage < filterStages; ++stage)
            {
                section.highPass[(size_t)channel][(size_t)stage].reset();
                section.lowPass[(size_t)channel][(size_t)stage].reset();
            }
        }
    }
}

float PRISMVSTAudioProcessor::peakDb(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    const int channels = juce::jmin(2, buffer.getNumChannels());

    for (int channel = 0; channel < channels; ++channel)
    {
        const auto* data = buffer.getReadPointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = juce::jmax(peak, std::abs(data[i]));
    }

    return gainToDb(peak + 1.0e-12f);
}

void PRISMVSTAudioProcessor::processTerritory(int sectionIndex,
                                              const juce::AudioBuffer<float>& dry,
                                              juce::AudioBuffer<float>& output,
                                              bool soloMode)
{
    const int samples = dry.getNumSamples();
    auto& section = sections[(size_t)sectionIndex];

    for (int channel = 0; channel < 2; ++channel)
        bandScratch.copyFrom(channel, 0, dry, channel, 0, samples);

    for (int channel = 0; channel < 2; ++channel)
    {
        auto* data = bandScratch.getWritePointer(channel);

        for (int i = 0; i < samples; ++i)
        {
            float value = data[i];

            if (sectionIndex > 0)
                for (int stage = 0; stage < filterStages; ++stage)
                    value = section.highPass[(size_t)channel][(size_t)stage].processSample(value);

            if (sectionIndex < numSections - 1)
                for (int stage = 0; stage < filterStages; ++stage)
                    value = section.lowPass[(size_t)channel][(size_t)stage].processSample(value);

            data[i] = value;
        }
    }

    const float detectorDrive = dbToGain(read(apvts, sectionId(sectionIndex, "input")));
    const float attackMs = juce::jmax(0.10f, read(apvts, sectionId(sectionIndex, "attack")));
    const float releaseMs = juce::jmax(5.0f, read(apvts, sectionId(sectionIndex, "release")));
    const float attackCoeff = std::exp(-1.0f / (0.001f * attackMs * static_cast<float>(currentSampleRate)));
    const float releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * static_cast<float>(currentSampleRate)));

    const float* bandL = bandScratch.getReadPointer(0);
    const float* bandR = bandScratch.getReadPointer(1);

    for (int i = 0; i < samples; ++i)
    {
        const float detected = juce::jmax(std::abs(bandL[i]), std::abs(bandR[i])) * detectorDrive;
        const float coeff = detected > section.envelope ? attackCoeff : releaseCoeff;
        section.envelope = coeff * section.envelope + (1.0f - coeff) * detected;
    }

    const float levelDb = gainToDb(section.envelope + 1.0e-12f);
    const float overDb = juce::jmax(0.0f, levelDb - kOnyxThresholdDb);
    const float onyxAmount = juce::jlimit(0.0f, 1.0f,
        read(apvts, sectionId(sectionIndex, "onyx")) / 100.0f);
    const float targetReduction = -juce::jmin(kMaxOnyxReductionDb, overDb * 0.75f) * onyxAmount;
    section.dynamicGainDb += 0.25f * (targetReduction - section.dynamicGainDb);

    const float postGainDb = read(apvts, sectionId(sectionIndex, "output"));
    const float sectionGain = dbToGain(postGainDb + section.dynamicGainDb);
    const float width = juce::jlimit(0.0f, 2.0f,
        read(apvts, sectionId(sectionIndex, "width")) / 100.0f);

    auto* outL = output.getWritePointer(0);
    auto* outR = output.getWritePointer(1);

    for (int i = 0; i < samples; ++i)
    {
        const float originalL = bandL[i];
        const float originalR = bandR[i];
        const float mid = 0.5f * (originalL + originalR);
        const float side = 0.5f * (originalL - originalR) * width;
        const float shapedL = (mid + side) * sectionGain;
        const float shapedR = (mid - side) * sectionGain;

        if (soloMode)
        {
            outL[i] += shapedL;
            outR[i] += shapedR;
        }
        else
        {
            outL[i] += shapedL - originalL;
            outR[i] += shapedR - originalR;
        }
    }
}

void PRISMVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int samples = buffer.getNumSamples();
    const int channels = juce::jmin(2, buffer.getNumChannels());
    if (channels < 2 || samples <= 0)
        return;

    if (samples > currentMaxBlockSize)
    {
        currentMaxBlockSize = samples;
        dryScratch.setSize(2, currentMaxBlockSize, false, false, true);
        bandScratch.setSize(2, currentMaxBlockSize, false, false, true);
    }

    inputPeakDb.store(peakDb(buffer));

    for (int channel = 0; channel < 2; ++channel)
        dryScratch.copyFrom(channel, 0, buffer, channel, 0, samples);

    bool anySolo = false;
    for (int sectionIndex = 0; sectionIndex < numSections; ++sectionIndex)
    {
        const bool enabled = read(apvts, sectionId(sectionIndex, "enabled")) > 0.5f;
        const bool solo = read(apvts, sectionId(sectionIndex, "solo")) > 0.5f;
        anySolo = anySolo || (enabled && solo);
    }

    if (anySolo)
        buffer.clear();
    else
        for (int channel = 0; channel < 2; ++channel)
            buffer.copyFrom(channel, 0, dryScratch, channel, 0, samples);

    float mostReduction = 0.0f;

    for (int sectionIndex = 0; sectionIndex < numSections; ++sectionIndex)
    {
        if (read(apvts, sectionId(sectionIndex, "enabled")) < 0.5f)
            continue;

        if (anySolo && read(apvts, sectionId(sectionIndex, "solo")) < 0.5f)
            continue;

        processTerritory(sectionIndex, dryScratch, buffer, anySolo);
        mostReduction = juce::jmin(mostReduction, sections[(size_t)sectionIndex].dynamicGainDb);
    }

    gainReductionDb.store(mostReduction);
    outputPeakDb.store(peakDb(buffer));
    pushAnalyzerSamples(buffer);
}

void PRISMVSTAudioProcessor::pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer)
{
    const auto* left = buffer.getReadPointer(0);
    const auto* right = buffer.getReadPointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
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
        const float norm = static_cast<float>(i) / static_cast<float>(spectrumBins - 1);
        const int bin = juce::jlimit(0, maxBin,
            juce::roundToInt(norm * static_cast<float>(maxBin)));
        const float magnitude = fftData[(size_t)bin] / static_cast<float>(fftSize);
        spectrum[(size_t)i].store(gainToDb(magnitude + 1.0e-12f));
    }
}

void PRISMVSTAudioProcessor::copySpectrum(std::array<float, spectrumBins>& destination) const noexcept
{
    for (int i = 0; i < spectrumBins; ++i)
        destination[(size_t)i] = spectrum[(size_t)i].load();
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
