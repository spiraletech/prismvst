#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <cmath>

static bool near(float a, float b, float tol) { return std::abs(a - b) <= tol; }

int main()
{
    PRISMVSTAudioProcessor p;
    p.prepareToPlay(48000.0, 512);

    juce::MidiBuffer midi;
    juce::AudioBuffer<float> b(2, 512);

    // Bypass must be bit-identical dry.
    if (auto* param = p.apvts.getParameter("master_bypass"))
        param->setValueNotifyingHost(1.0f);

    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            b.setSample(c, i, std::sin((float)i * 0.071f) * 0.7f);

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf(b);
    p.processBlock(b, midi);

    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            if (!near(b.getSample(c, i), dry.getSample(c, i), 1.0e-7f)) {
                std::cerr << "Bypass mismatch\n";
                return 1;
            }

    // Ceiling must never overshoot in processed mode.
    if (auto* param = p.apvts.getParameter("master_bypass"))
        param->setValueNotifyingHost(0.0f);
    if (auto* param = p.apvts.getParameter("ceiling"))
        param->setValueNotifyingHost(param->convertTo0to1(-6.0f));

    b.clear();
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            b.setSample(c, i, (i & 1) ? 1.5f : -1.5f);

    p.processBlock(b, midi);
    const float limit = juce::Decibels::decibelsToGain(-6.0f) + 1.0e-5f;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            if (std::abs(b.getSample(c, i)) > limit) {
                std::cerr << "Ceiling overshoot\n";
                return 2;
            }


    // Isolate ONYX from EQ dynamics so the test measures the named effect.
    for (int k = 1; k <= 6; ++k)
    {
        const auto id = "band" + std::to_string(k) + "_enabled";
        if (auto* param = p.apvts.getParameter(id))
            param->setValueNotifyingHost(0.0f);
    }

    // ONYX parameters must have real, measurable signal effects at full rotation.
    auto setParam = [&](const char* id, float value) {
        auto* param = p.apvts.getParameter(id);
        if (param == nullptr) return false;
        param->setValueNotifyingHost(param->convertTo0to1(value));
        return true;
    };
    const char* controls[] { "onyx_sparkle", "onyx_shine", "onyx_smooth", "onyx_tape" };
    for (const auto* control : controls)
    {
        p.prepareToPlay(48000.0, 512);
        setParam("ceiling", 0.0f);
        setParam("onyx", 0.0f);
        for (const auto* id : controls) setParam(id, 0.0f);
        juce::AudioBuffer<float> baseline(2, 512), processed(2, 512);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
                baseline.setSample(c, i, 0.15f * std::sin((float)i * 0.85f)
                    + 0.10f * std::sin((float)i * 0.11f));
        processed.makeCopyOf(baseline);
        setParam(control, 100.0f);
        p.processBlock(processed, midi);
        double difference = 0.0;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
            {
                const float y = processed.getSample(c, i);
                if (!std::isfinite(y)) { std::cerr << "Nonfinite ONYX output\\n"; return 3; }
                difference += std::abs(y - baseline.getSample(c, i));
            }
        if (difference < 0.05) {
            std::cerr << "ONYX control is not processing: " << control << "\\n";
            return 4;
        }
    }

    // Threshold must affect dynamics with an enabled dynamic EQ band.
    for (const auto* id : controls) setParam(id, 0.0f);
    for (int k = 2; k <= 6; ++k)
        setParam(("band" + std::to_string(k) + "_enabled").c_str(), 0.0f);
    setParam("band1_enabled", 1.0f);
    setParam("band1_freq", 1000.0f);
    setParam("band1_q", 1.0f);
    setParam("band1_gain", 0.0f);
    setParam("band1_dyn_range", 12.0f);
    setParam("band1_ratio", 8.0f);
    setParam("band1_attack", 1.0f);
    setParam("band1_release", 30.0f);
    auto renderLevel = [&](float threshold) {
        p.prepareToPlay(48000.0, 512);
        setParam("band1_threshold", threshold);
        juce::AudioBuffer<float> signal(2, 512);
        double energy = 0.0;
        for (int block = 0; block < 25; ++block)
        {
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 512; ++i)
                    signal.setSample(c, i, 0.3f * std::sin(
                        juce::MathConstants<float>::twoPi * 1000.0f * (float)(block * 512 + i) / 48000.0f));
            p.processBlock(signal, midi);
            if (block == 24)
                for (int i = 0; i < 512; ++i)
                    energy += signal.getSample(0, i) * signal.getSample(0, i);
        }
        return energy;
    };
    const double compressed = renderLevel(-48.0f);
    const double uncompressed = renderLevel(0.0f);
    if (!(compressed < uncompressed * 0.98)) {
        std::cerr << "Compressor threshold not influencing output\\n";
        return 5;
    }

    std::cout << "PRISMVST DSP smoke tests passed\n";
    return 0;
}
