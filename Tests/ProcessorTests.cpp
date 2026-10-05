#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
bool setActual(PRISMVSTAudioProcessor& p, const juce::String& id, float value)
{
    if (auto* parameter = p->apvts.getParameter(id))
    {
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        return true;
    }
    return false;
}

void fillTone(juce::AudioBuffer<float>& buffer,
              float frequency,
              float amplitude,
              double sampleRate,
              int64_t sampleOffset,
              bool antiPhase = false)
{
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float x = amplitude * std::sin(
            2.0f * juce::MathConstants<float>::pi * frequency
            * (float)(sampleOffset + i) / (float)sampleRate);

        buffer.setSample(0, i, x);
        buffer.setSample(1, i, antiPhase ? -x : x);
    }
}

double rms(const juce::AudioBuffer<float>& buffer)
{
    double energy = 0.0;
    const int count = buffer.getNumChannels() * buffer.getNumSamples();

    for (int c = 0; c < buffer.getNumChannels(); ++c)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const double x = buffer.getSample(c, i);
            energy += x * x;
        }

    return count > 0 ? std::sqrt(energy / (double)count) : 0.0;
}

bool finiteBuffer(const juce::AudioBuffer<float>& buffer)
{
    for (int c = 0; c < buffer.getNumChannels(); ++c)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!std::isfinite(buffer.getSample(c, i)))
                return false;
    return true;
}
}

int main()
{
    std::cerr << "TEST 1 schema" << std::endl;
    // -------------------------------------------------------------------------
    // Contract: seven named sections, six crossovers, no legacy master bypass.
    {
        auto p = std::make_unique<PRISMVSTAudioProcessor>();

        if (PRISMVSTAudioProcessor::numSections != 7
            || PRISMVSTAudioProcessor::numCrossovers != 6)
        {
            std::cerr << "PRISM must expose seven sections and six crossovers\n";
            return 1;
        }

        for (int s = 0; s < PRISMVSTAudioProcessor::numSections; ++s)
        {
            for (const auto& suffix : {
                     "on", "solo", "input_db", "output_db",
                     "attack_ms", "release_ms", "width_pct", "onyx_pct",
                     "threshold_db", "ratio", "curve" })
            {
                if (p->apvts.getParameter(PRISMVSTAudioProcessor::sectionId(s, suffix)) == nullptr)
                {
                    std::cerr << "Missing section parameter: "
                              << PRISMVSTAudioProcessor::sectionId(s, suffix) << "\n";
                    return 2;
                }
            }
        }

        for (int x = 0; x < PRISMVSTAudioProcessor::numCrossovers; ++x)
            if (p->apvts.getParameter(PRISMVSTAudioProcessor::crossoverId(x)) == nullptr)
            {
                std::cerr << "Missing crossover parameter\n";
                return 3;
            }

        if (p->apvts.getParameter("master_bypass") != nullptr
            || p->apvts.getParameter("master_trim") != nullptr
            || p->apvts.getParameter("ceiling") != nullptr
            || p->apvts.getParameter("band1_freq") != nullptr)
        {
            std::cerr << "Legacy master/node parameters survived migration\n";
            return 4;
        }
    }

    std::cerr << "TEST 2 off mute" << std::endl;
    // -------------------------------------------------------------------------
    // OFF means muted. A soloed-but-OFF section must produce silence.
    {
        auto p = std::make_unique<PRISMVSTAudioProcessor>();
        p->prepareToPlay(48000.0, 512);

        setActual(*p, PRISMVSTAudioProcessor::sectionId(1, "solo"), 1.0f);
        setActual(*p, PRISMVSTAudioProcessor::sectionId(1, "on"), 0.0f);

        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer midi;
        fillTone(b, 90.0f, 0.6f, 48000.0, 0);
        p->processBlock(b, midi);

        if (b.getMagnitude(0, 0, b.getNumSamples()) > 1.0e-7f
            || b.getMagnitude(1, 0, b.getNumSamples()) > 1.0e-7f)
        {
            std::cerr << "OFF section is not muted\n";
            return 5;
        }
    }

    std::cerr << "TEST 3 solo" << std::endl;
    // -------------------------------------------------------------------------
    // SOLO means audible isolation, and the section path must remain finite.
    {
        auto p = std::make_unique<PRISMVSTAudioProcessor>();
        p->prepareToPlay(48000.0, 512);
        setActual(*p, PRISMVSTAudioProcessor::sectionId(1, "solo"), 1.0f);

        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer midi;

        for (int block = 0; block < 12; ++block)
        {
            fillTone(b, 90.0f, 0.5f, 48000.0, (int64_t)block * 512);
            p->processBlock(b, midi);
        }

        if (!finiteBuffer(b) || rms(b) < 0.005)
        {
            std::cerr << "SOLO section is silent or unstable\n";
            return 6;
        }
    }

    std::cerr << "TEST 4 neutral" << std::endl;
    // -------------------------------------------------------------------------
    // Neutral seven-way reconstruction must not grossly change amplitude.
    {
        auto p = std::make_unique<PRISMVSTAudioProcessor>();
        p->prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> b(2, 512);
        juce::AudioBuffer<float> dry(2, 512);
        juce::MidiBuffer midi;

        double dryRms = 0.0;
        double wetRms = 0.0;

        for (int block = 0; block < 48; ++block)
        {
            fillTone(b, 1000.0f, 0.35f, 48000.0, (int64_t)block * 512);
            dry.makeCopyOf(b);
            p->processBlock(b, midi);

            if (block == 47)
            {
                dryRms = rms(dry);
                wetRms = rms(b);
            }
        }

        if (!finiteBuffer(b) || dryRms <= 1.0e-9)
        {
            std::cerr << "Neutral reconstruction test invalid\n";
            return 7;
        }

        const double ratio = wetRms / dryRms;
        if (ratio < 0.80 || ratio > 1.20)
        {
            std::cerr << "Neutral crossover reconstruction amplitude drift: " << ratio << "\n";
            return 8;
        }
    }

    std::cerr << "TEST 5 width" << std::endl;
    // -------------------------------------------------------------------------
    // WIDTH 0% must collapse side information for the selected section.
    {
        auto p = std::make_unique<PRISMVSTAudioProcessor>();
        p->prepareToPlay(48000.0, 512);

        const int mid = 4; // MID: nominally 500 Hz -> 2 kHz.
        setActual(*p, PRISMVSTAudioProcessor::sectionId(mid, "solo"), 1.0f);
        setActual(*p, PRISMVSTAudioProcessor::sectionId(mid, "width_pct"), 0.0f);

        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer midi;

        for (int block = 0; block < 16; ++block)
        {
            fillTone(b, 1000.0f, 0.4f, 48000.0, (int64_t)block * 512, true);
            p->processBlock(b, midi);
        }

        if (rms(b) > 0.003)
        {
            std::cerr << "WIDTH 0% did not collapse side-only material\n";
            return 9;
        }
    }

    std::cerr << "TEST 6 sanitize" << std::endl;
    // -------------------------------------------------------------------------
    // Invalid crossover ordering must repair without changing the 7-section law.
    {
        auto p = std::make_unique<PRISMVSTAudioProcessor>();
        setActual(*p, PRISMVSTAudioProcessor::crossoverId(4), 11000.0f);
        setActual(*p, PRISMVSTAudioProcessor::crossoverId(5), 900.0f);
        p->prepareToPlay(48000.0, 512);

        float previous = 20.0f;
        for (int x = 0; x < PRISMVSTAudioProcessor::numCrossovers; ++x)
        {
            const auto* value = p->apvts.getRawParameterValue(PRISMVSTAudioProcessor::crossoverId(x));
            if (value == nullptr || value->load() <= previous)
            {
                std::cerr << "Crossover sanitizer failed\n";
                return 10;
            }
            previous = value->load();
        }
    }

    std::cout << "PRISM Alpha 001 v0.3.0 seven-section DSP tests passed\n";
    return 0;
}
