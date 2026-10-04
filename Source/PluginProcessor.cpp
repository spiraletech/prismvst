#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace {
constexpr std::array<const char*, 5> kNames { "SUB", "KICK", "LOW", "MID", "HIGH" };
constexpr std::array<float, 4> kDefaultXover { 90.0f, 180.0f, 650.0f, 4500.0f };
constexpr float kFloorDb = -100.0f;

inline float db(float g) { return juce::Decibels::gainToDecibels(g, kFloorDb); }
inline float gain(float d) { return juce::Decibels::decibelsToGain(d); }
}

PRISMVSTAudioProcessor::PRISMVSTAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

bool PRISMVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

juce::String PRISMVSTAudioProcessor::bandPrefix(int i)
{
    return juce::String(kNames[(size_t) juce::jlimit(0, numBands - 1, i)]).toLowerCase();
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

    for (int i = 0; i < numBands; ++i) {
        const auto pre = bandPrefix(i);
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pre + "_scrape", 1 }, juce::String(kNames[(size_t)i]) + " Scrape",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pre + "_depth", 1 }, juce::String(kNames[(size_t)i]) + " Depth",
            juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pre + "_tone", 1 }, juce::String(kNames[(size_t)i]) + " Tone",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pre + "_density", 1 }, juce::String(kNames[(size_t)i]) + " Density",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { pre + "_trim", 1 }, juce::String(kNames[(size_t)i]) + " Trim",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
        p.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { pre + "_solo", 1 }, juce::String(kNames[(size_t)i]) + " Solo", false));
        p.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { pre + "_bypass", 1 }, juce::String(kNames[(size_t)i]) + " Bypass", false));
    }

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "xover_1", 1 }, "SUB/KICK", juce::NormalisableRange<float>(45.0f, 140.0f, 1.0f), kDefaultXover[0]));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "xover_2", 1 }, "KICK/LOW", juce::NormalisableRange<float>(120.0f, 350.0f, 1.0f), kDefaultXover[1]));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "xover_3", 1 }, "LOW/MID", juce::NormalisableRange<float>(300.0f, 1800.0f, 1.0f), kDefaultXover[2]));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "xover_4", 1 }, "MID/HIGH", juce::NormalisableRange<float>(1800.0f, 9000.0f, 1.0f), kDefaultXover[3]));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "master_trim", 1 }, "Master Trim", juce::NormalisableRange<float>(-18.0f, 12.0f, 0.1f), 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "ceiling", 1 }, "Ceiling", juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -0.3f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "master_bypass", 1 }, "Master Bypass", false));

    return { p.begin(), p.end() };
}

void PRISMVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 1 };

    for (auto& stage : lp)
        for (auto& f : stage) { f.reset(); f.prepare(spec); f.setType(juce::dsp::LinkwitzRileyFilterType::lowpass); }
    for (auto& stage : hp)
        for (auto& f : stage) { f.reset(); f.prepare(spec); f.setType(juce::dsp::LinkwitzRileyFilterType::highpass); }

    for (auto& b : bands) {
        b.buffer.setSize(2, samplesPerBlock, false, false, true);
        b.envelope = 0.0f;
        b.gainSmooth = 1.0f;
    }

    auto hpC = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.0);
    auto shC = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1681.0, 0.7071f, gain(4.0f));
    for (int c = 0; c < 2; ++c) {
        loudHp[(size_t)c].reset();
        loudShelf[(size_t)c].reset();
        *loudHp[(size_t)c].coefficients = *hpC;
        *loudShelf[(size_t)c].coefficients = *shC;
    }

    integratedEnergy = 0.0;
    integratedSamples = 0;
}

void PRISMVSTAudioProcessor::splitBands(const juce::AudioBuffer<float>& source)
{
    const int n = source.getNumSamples();
    for (auto& b : bands) {
        b.buffer.setSize(2, n, false, false, true);
        b.buffer.clear();
    }

    std::array<float, 4> xo {
        read(apvts, "xover_1"), read(apvts, "xover_2"),
        read(apvts, "xover_3"), read(apvts, "xover_4")
    };

    xo[1] = juce::jmax(xo[1], xo[0] + 20.0f);
    xo[2] = juce::jmax(xo[2], xo[1] + 80.0f);
    xo[3] = juce::jmax(xo[3], xo[2] + 300.0f);

    for (int s = 0; s < 4; ++s)
        for (int c = 0; c < 2; ++c) {
            lp[(size_t)s][(size_t)c].setCutoffFrequency(xo[(size_t)s]);
            hp[(size_t)s][(size_t)c].setCutoffFrequency(xo[(size_t)s]);
        }

    for (int c = 0; c < 2; ++c) {
        const float* in = source.getReadPointer(c);
        float* b0 = bands[0].buffer.getWritePointer(c);
        float* b1 = bands[1].buffer.getWritePointer(c);
        float* b2 = bands[2].buffer.getWritePointer(c);
        float* b3 = bands[3].buffer.getWritePointer(c);
        float* b4 = bands[4].buffer.getWritePointer(c);

        for (int i = 0; i < n; ++i) {
            const float x = in[i];
            const float lo0 = lp[0][(size_t)c].processSample(0, x);
            float rem = hp[0][(size_t)c].processSample(0, x);
            const float lo1 = lp[1][(size_t)c].processSample(0, rem);
            rem = hp[1][(size_t)c].processSample(0, rem);
            const float lo2 = lp[2][(size_t)c].processSample(0, rem);
            rem = hp[2][(size_t)c].processSample(0, rem);
            const float lo3 = lp[3][(size_t)c].processSample(0, rem);
            const float hi3 = hp[3][(size_t)c].processSample(0, rem);

            b0[i] = lo0;
            b1[i] = lo1;
            b2[i] = lo2;
            b3[i] = lo3;
            b4[i] = hi3;
        }
    }
}

void PRISMVSTAudioProcessor::processBand(int bandIndex, int numSamples)
{
    auto& st = bands[(size_t)bandIndex];
    const auto pre = bandPrefix(bandIndex);

    if (read(apvts, pre + "_bypass") > 0.5f)
        return;

    const float threshold = read(apvts, pre + "_scrape");
    const float maxDepth = read(apvts, pre + "_depth");
    const float tone = read(apvts, pre + "_tone");
    const float density = read(apvts, pre + "_density");
    const float trim = gain(read(apvts, pre + "_trim"));

    const float attackMs = 2.0f;
    const float releaseMs = juce::jmap(density, 160.0f, 35.0f);
    const float a = std::exp(-1.0f / (0.001f * attackMs * (float)currentSampleRate));
    const float r = std::exp(-1.0f / (0.001f * releaseMs * (float)currentSampleRate));
    const float saturation = juce::jlimit(0.0f, 0.45f, std::abs(tone) * (0.08f + density * 0.30f));

    auto* l = st.buffer.getWritePointer(0);
    auto* rr = st.buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i) {
        const float detector = juce::jmax(std::abs(l[i]), std::abs(rr[i]));
        const float coeff = detector > st.envelope ? a : r;
        st.envelope = coeff * st.envelope + (1.0f - coeff) * detector;

        const float level = db(st.envelope + 1.0e-9f);
        const float over = juce::jmax(0.0f, level - threshold);
        const float densityShape = 0.45f + density * 1.55f;
        const float reductionDb = juce::jmin(maxDepth, over * densityShape);
        const float targetGain = gain(-reductionDb);
        st.gainSmooth += 0.04f * (targetGain - st.gainSmooth);

        float sl = l[i] * st.gainSmooth * trim;
        float sr = rr[i] * st.gainSmooth * trim;

        if (saturation > 0.0f) {
            const float drive = 1.0f + saturation * 5.0f;
            const float norm = std::tanh(drive);
            const float shapedL = std::tanh(sl * drive) / norm;
            const float shapedR = std::tanh(sr * drive) / norm;
            const float mix = saturation;
            sl = juce::jmap(mix, sl, shapedL);
            sr = juce::jmap(mix, sr, shapedR);
        }

        // Tone is deliberately not an EQ shelf/curve: it biases the nonlinear blend polarity.
        const float toneGain = gain(tone * 1.5f);
        l[i] = sl * toneGain;
        rr[i] = sr * toneGain;
    }
}

void PRISMVSTAudioProcessor::updateMeters(const juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    float peak = 0.0f;
    double weightedEnergy = 0.0;

    for (int c = 0; c < juce::jmin(2, buffer.getNumChannels()); ++c) {
        const float* x = buffer.getReadPointer(c);
        for (int i = 0; i < n; ++i) {
            peak = juce::jmax(peak, std::abs(x[i]));
            float y = loudHp[(size_t)c].processSample(x[i]);
            y = loudShelf[(size_t)c].processSample(y);
            weightedEnergy += (double)y * (double)y;
        }
    }

    peakDb.store(db(peak + 1.0e-9f));

    const auto count = (uint64_t)n * 2u;
    integratedEnergy += weightedEnergy;
    integratedSamples += count;

    const double blockMean = count > 0 ? weightedEnergy / (double)count : 0.0;
    const double intMean = integratedSamples > 0 ? integratedEnergy / (double)integratedSamples : 0.0;

    const float shortL = blockMean > 1.0e-12 ? (float)(-0.691 + 10.0 * std::log10(blockMean)) : kFloorDb;
    const float intL = intMean > 1.0e-12 ? (float)(-0.691 + 10.0 * std::log10(intMean)) : kFloorDb;
    lufsShort.store(shortL);
    lufsIntegrated.store(intL);
}

void PRISMVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());

    if (read(apvts, "master_bypass") > 0.5f) {
        updateMeters(buffer);
        return;
    }

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf(buffer, true);

    splitBands(dry);
    for (int b = 0; b < numBands; ++b)
        processBand(b, buffer.getNumSamples());

    bool anySolo = false;
    for (int b = 0; b < numBands; ++b)
        anySolo = anySolo || read(apvts, bandPrefix(b) + "_solo") > 0.5f;

    buffer.clear();
    for (int b = 0; b < numBands; ++b) {
        if (anySolo && read(apvts, bandPrefix(b) + "_solo") <= 0.5f)
            continue;
        for (int c = 0; c < 2; ++c)
            buffer.addFrom(c, 0, bands[(size_t)b].buffer, c, 0, buffer.getNumSamples());
    }

    buffer.applyGain(gain(read(apvts, "master_trim")));

    const float ceiling = gain(read(apvts, "ceiling"));
    for (int c = 0; c < 2; ++c) {
        float* x = buffer.getWritePointer(c);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            x[i] = juce::jlimit(-ceiling, ceiling, x[i]);
    }

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
