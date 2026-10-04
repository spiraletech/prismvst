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


    // Six nodes must remain independent and expose their own transfer slope.
    if (PRISMVSTAudioProcessor::numEqBands != 6) {
        std::cerr << "Expected six independent nodes\n";
        return 3;
    }

    auto* node1Slope = p.apvts.getParameter("band1_ratio");
    auto* node2Slope = p.apvts.getParameter("band2_ratio");
    auto* node6Slope = p.apvts.getParameter("band6_ratio");
    auto* node1ReleaseB = p.apvts.getParameter("band1_release2");
    auto* node1Detector = p.apvts.getParameter("band1_detector_mix");

    if (node1Slope == nullptr || node2Slope == nullptr || node6Slope == nullptr
        || node1ReleaseB == nullptr || node1Detector == nullptr) {
        std::cerr << "Missing per-node dynamics parameters\n";
        return 4;
    }

    const float node2Before = node2Slope->getValue();
    node1Slope->setValueNotifyingHost(node1Slope->convertTo0to1(7.5f));
    if (!near(node2Slope->getValue(), node2Before, 1.0e-7f)) {
        std::cerr << "Node parameter state leaked across nodes\n";
        return 5;
    }

    // Extreme detector/timing settings must remain finite.
    node1ReleaseB->setValueNotifyingHost(node1ReleaseB->convertTo0to1(4000.0f));
    node1Detector->setValueNotifyingHost(node1Detector->convertTo0to1(1.0f));
    if (auto* curve = p.apvts.getParameter("band1_curve"))
        curve->setValueNotifyingHost(curve->convertTo0to1(1.0f));

    b.clear();
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            b.setSample(c, i, std::sin((float)i * 0.13f) * 0.95f);

    p.processBlock(b, midi);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            if (!std::isfinite(b.getSample(c, i))) {
                std::cerr << "Non-finite DSP output\n";
                return 6;
            }


    // Processed mode must produce a materially audible change when a node is
    // deliberately driven. This prevents a compile-green but sonically inert UI.
    for (int i = 0; i < PRISMVSTAudioProcessor::numEqBands; ++i)
        if (auto* enabled = p.apvts.getParameter("band" + juce::String(i + 1) + "_enabled"))
            enabled->setValueNotifyingHost(i == 2 ? 1.0f : 0.0f);

    if (auto* gain = p.apvts.getParameter("band3_gain"))
        gain->setValueNotifyingHost(gain->convertTo0to1(12.0f));
    if (auto* depth = p.apvts.getParameter("band3_dyn_range"))
        depth->setValueNotifyingHost(depth->convertTo0to1(0.0f));
    if (auto* q = p.apvts.getParameter("band3_q"))
        q->setValueNotifyingHost(q->convertTo0to1(1.2f));
    if (auto* ceiling = p.apvts.getParameter("ceiling"))
        ceiling->setValueNotifyingHost(ceiling->convertTo0to1(0.0f));

    b.clear();
    juce::AudioBuffer<float> audibleDry(2, 512);
    for (int c = 0; c < 2; ++c)
    {
        for (int i = 0; i < 512; ++i)
        {
            const float x = 0.08f * std::sin(
                2.0f * juce::MathConstants<float>::pi * 240.0f
                * (float)i / 48000.0f);
            b.setSample(c, i, x);
            audibleDry.setSample(c, i, x);
        }
    }

    p.processBlock(b, midi);

    double dryEnergy = 0.0;
    double wetEnergy = 0.0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
        {
            dryEnergy += (double)audibleDry.getSample(c, i) * audibleDry.getSample(c, i);
            wetEnergy += (double)b.getSample(c, i) * b.getSample(c, i);
        }

    if (!(wetEnergy > dryEnergy * 1.35)) {
        std::cerr << "Node processing is not materially changing audio\n";
        return 7;
    }

    // Legacy/out-of-order states must be repaired without swapping node identity.
    if (auto* n5 = p.apvts.getParameter("band5_freq"))
        n5->setValueNotifyingHost(n5->convertTo0to1(11100.0f));
    if (auto* n6 = p.apvts.getParameter("band6_freq"))
        n6->setValueNotifyingHost(n6->convertTo0to1(900.0f));

    p.prepareToPlay(48000.0, 512);

    const auto* n5v = p.apvts.getRawParameterValue("band5_freq");
    const auto* n6v = p.apvts.getRawParameterValue("band6_freq");
    if (n5v == nullptr || n6v == nullptr || !(n6v->load() > n5v->load())) {
        std::cerr << "Node ordering sanitizer failed\n";
        return 8;
    }

    std::cout << "PRISM Alpha 001 DSP smoke tests passed\n";
    return 0;
}
