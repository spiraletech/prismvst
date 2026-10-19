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

    // Host project/preset restore must preserve legacy parameter IDs and values.
    // A regression here silently changes the sound of previously saved sessions.
    struct SavedParameter { const char* id; float value; };
    constexpr SavedParameter saved[] {
        { "band1_freq", 173.0f },
        { "band3_gain", -4.25f },
        { "band6_dyn_range", 7.5f },
        { "onyx", 0.42f },
        { "master_trim", -2.0f }
    };

    for (const auto& entry : saved)
    {
        auto* param = p.apvts.getParameter(entry.id);
        if (param == nullptr) {
            std::cerr << "Missing legacy parameter: " << entry.id << "\\n";
            return 3;
        }
        param->setValueNotifyingHost(param->convertTo0to1(entry.value));
    }

    juce::MemoryBlock serialized;
    p.getStateInformation(serialized);
    if (serialized.getSize() == 0) {
        std::cerr << "Empty serialized state\\n";
        return 4;
    }

    PRISMVSTAudioProcessor restored;
    restored.setStateInformation(serialized.getData(), (int) serialized.getSize());
    for (const auto& entry : saved)
    {
        const auto* value = restored.apvts.getRawParameterValue(entry.id);
        if (value == nullptr || !near(value->load(), entry.value, 0.025f)) {
            std::cerr << "Preset round-trip mismatch: " << entry.id << "\\n";
            return 5;
        }
    }

    std::cout << "PRISMVST DSP smoke tests passed\n";
    return 0;
}
