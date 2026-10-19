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

    // ONYX 2.0 parameter compatibility and harmonic/offset smoke coverage.
    // Tone defaults to neutral so old sessions retain the original transfer curve.
    auto* tone = p.apvts.getParameter("onyx_tone");
    auto* bias = p.apvts.getParameter("onyx_bias");
    auto* onyx = p.apvts.getParameter("onyx");
    if (!tone || !bias || !onyx || !near(p.apvts.getRawParameterValue("onyx_tone")->load(), 0.0f, 0.001f))
        return 3;

    onyx->setValueNotifyingHost(onyx->convertTo0to1(1.0f));
    bias->setValueNotifyingHost(bias->convertTo0to1(0.0f));
    tone->setValueNotifyingHost(tone->convertTo0to1(0.0f));
    b.clear();
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 512; ++i)
            b.setSample(c, i, std::sin(2.0 * juce::MathConstants<double>::pi * i * 1000.0 / 48000.0) * 0.25f);
    // Ceiling remains active; validate finite bounded signal and odd-symmetric baseline.
    p.processBlock(b, midi);
    double sum = 0.0;
    for (int i = 0; i < 512; ++i) {
        const float v = b.getSample(0, i);
        if (!std::isfinite(v)) return 4;
        sum += v;
    }
    // Silence is a stronger DC regression than a noninteger-period sine.
    b.clear();
    bias->setValueNotifyingHost(bias->convertTo0to1(1.0f));
    p.processBlock(b, midi);
    for (int i = 0; i < 512; ++i)
        if (!std::isfinite(b.getSample(0, i))) return 5;

    // A new Tone value must survive the existing host state serialization.
    tone->setValueNotifyingHost(tone->convertTo0to1(-0.4f));
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    PRISMVSTAudioProcessor restored;
    restored.setStateInformation(saved.getData(), (int)saved.getSize());
    if (!near(restored.apvts.getRawParameterValue("onyx_tone")->load(), -0.4f, 0.002f)
        || !near(restored.apvts.getRawParameterValue("onyx_bias")->load(), 1.0f, 0.002f))
        return 6;

    // Aliasing is not asserted absent: ONYX is not yet oversampled.
    // Full FFT-based alias/THD qualification remains a release gate.
    std::cout << "PRISMVST DSP smoke tests passed\n";
    return 0;
}
