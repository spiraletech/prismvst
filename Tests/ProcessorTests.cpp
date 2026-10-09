#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>

static bool near(float a, float b, float tolerance)
{
    return std::abs(a - b) <= tolerance;
}

int main()
{
    PRISMVSTAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buffer(2, 512);

    // Default state must be neutral: all section gains are unity,
    // width is 100%, and ONYX is 0%.
    for (int channel = 0; channel < 2; ++channel)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(channel, i,
                std::sin((float)i * 0.071f + (float)channel * 0.19f) * 0.65f);

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf(buffer);
    processor.processBlock(buffer, midi);

    for (int channel = 0; channel < 2; ++channel)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!near(buffer.getSample(channel, i), dry.getSample(channel, i), 1.0e-6f))
            {
                std::cerr << "Neutral-state mismatch\n";
                return 1;
            }

    // Every visible section control must exist for all seven territories.
    for (int section = 0; section < PRISMVSTAudioProcessor::numSections; ++section)
    {
        for (const auto& suffix : { "enabled", "solo", "input", "output",
                                    "attack", "release", "width", "onyx" })
        {
            if (processor.apvts.getParameter(
                    PRISMVSTAudioProcessor::sectionId(section, suffix)) == nullptr)
            {
                std::cerr << "Missing parameter for section "
                          << section << ": " << suffix << "\n";
                return 2;
            }
        }
    }

    // Processing must remain finite when ONYX and width are engaged.
    if (auto* p = processor.apvts.getParameter(
            PRISMVSTAudioProcessor::sectionId(0, "onyx")))
        p->setValueNotifyingHost(p->convertTo0to1(75.0f));

    if (auto* p = processor.apvts.getParameter(
            PRISMVSTAudioProcessor::sectionId(0, "width")))
        p->setValueNotifyingHost(p->convertTo0to1(140.0f));

    buffer.makeCopyOf(dry);
    processor.processBlock(buffer, midi);

    for (int channel = 0; channel < 2; ++channel)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!std::isfinite(buffer.getSample(channel, i)))
            {
                std::cerr << "Non-finite DSP output\n";
                return 3;
            }

    std::cout << "PRISMVST v0.3.3 DSP smoke tests passed\n";
    return 0;
}
