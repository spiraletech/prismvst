#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <cmath>

static bool near(float a, float b, float tol) { return std::abs(a - b) <= tol; }

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
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

    auto set = [](PRISMVSTAudioProcessor& processor, const juce::String& id, float value)
    {
        auto* parameter = processor.apvts.getParameter(id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };

    // Identical input streams must not change with host block partitioning.
    auto render = [&](int blockSize)
    {
        PRISMVSTAudioProcessor processor;
        set(processor, "band4_dyn_range", 12.0f);
        set(processor, "band4_threshold", -35.0f);
        set(processor, "band4_gain", 3.0f);
        processor.prepareToPlay(48000, blockSize);
        std::vector<float> output;
        for (int start = 0; start < 8192; start += blockSize)
        {
            const int count = juce::jmin(blockSize, 8192 - start);
            juce::AudioBuffer<float> block(2, count);
            for (int i = 0; i < count; ++i)
                for (int c = 0; c < 2; ++c)
                    block.setSample(c, i, 0.25f * std::sin((float)(start + i) * 0.157f));
            processor.processBlock(block, midi);
            output.insert(output.end(), block.getReadPointer(0), block.getReadPointer(0) + count);
        }
        return output;
    };
    const auto reference = render(512);
    for (int size : { 1, 31, 64, 127, 1024 })
    {
        const auto output = render(size);
        for (size_t i = 0; i < reference.size(); ++i)
            if (!near(reference[i], output[i], 1.0e-6f))
            { std::cerr << "Block partition mismatch at " << size << "\n"; return 3; }
    }

    // A bin-centred sine verifies the analyzer's amplitude calibration.
    PRISMVSTAudioProcessor analyzer;
    analyzer.prepareToPlay(48000, 2048);
    juce::AudioBuffer<float> tone(2, 2048);
    for (int i = 0; i < tone.getNumSamples(); ++i)
        for (int c = 0; c < 2; ++c)
            tone.setSample(c, i, 0.25f * std::sin(juce::MathConstants<float>::twoPi * 128.0f * (float)i / 2048.0f));
    juce::AudioBuffer<float> neutralInput;
    neutralInput.makeCopyOf(tone);
    analyzer.processBlock(tone, midi);
    for (int i = 0; i < tone.getNumSamples(); ++i)
        if (!near(tone.getSample(0, i), neutralInput.getSample(0, i), 1.0e-5f))
        { std::cerr << "Neutral EQ changed input\n"; return 6; }
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> spectrum;
    analyzer.copySpectrum(spectrum);
    if (!near(spectrum[64], -12.0412f, 0.15f))
    { std::cerr << "Analyzer calibration failed: " << spectrum[64] << "\n"; return 7; }
    analyzer.prepareToPlay(48000, 512);
    if (analyzer.getPeakDb() > -99.0f || analyzer.getLufsIntegrated() > -99.0f)
    { std::cerr << "Meter reset failed\n"; return 8; }

    // Full parameter state round-trips; malformed data leaves it intact.
    set(p, "band3_q", 7.5f);
    juce::MemoryBlock state;
    p.getStateInformation(state);
    PRISMVSTAudioProcessor restored;
    restored.setStateInformation(state.getData(), (int)state.getSize());
    restored.setStateInformation("invalid", 7);
    if (!near(restored.apvts.getRawParameterValue("band3_q")->load(), 7.5f, 0.001f))
    { std::cerr << "State roundtrip failed\n"; return 4; }

    for (double rate : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        PRISMVSTAudioProcessor extreme;
        for (int band = 1; band <= 6; ++band)
        {
            const auto prefix = "band" + juce::String(band) + "_";
            set(extreme, prefix + "freq", 20000.0f);
            set(extreme, prefix + "q", 20.0f);
            set(extreme, prefix + "gain", 18.0f);
            set(extreme, prefix + "dyn_range", 18.0f);
        }
        set(extreme, "onyx", 1.0f);
        extreme.prepareToPlay(rate, 512);
        for (int block = 0; block < 20; ++block)
        {
            b.clear();
            b.setSample(0, 0, 1.0f);
            b.setSample(1, 0, -1.0f);
            extreme.processBlock(b, midi);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < b.getNumSamples(); ++i)
                    if (!std::isfinite(b.getSample(c, i)) || std::abs(b.getSample(c, i)) > 1.00001f)
                    { std::cerr << "Extreme setting instability\n"; return 5; }
        }
        juce::AudioBuffer<float> empty(2, 0);
        extreme.processBlock(empty, midi);
    }

    // Render the actual editor at both supported extremes for visual inspection.
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    for (const auto size : { juce::Point<int>(1080, 820), juce::Point<int>(1280, 920) })
    {
        editor->setSize(size.x, size.y);
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        juce::File file = juce::File::getCurrentWorkingDirectory().getChildFile("prism-ui-" + juce::String(size.x) + ".png");
        juce::FileOutputStream stream(file);
        stream.setPosition(0);
        stream.truncate();
        juce::PNGImageFormat().writeImageToStream(image, stream);
    }
    std::cout << "PRISMVST regression tests passed\n";
    return 0;
}
