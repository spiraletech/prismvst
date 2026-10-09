#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class PRISMVSTAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int numSections = 7;
    static constexpr int numSplits = 6;
    static constexpr int spectrumBins = 1024;

    inline static constexpr std::array<float, numSplits> splitFrequencies {
        60.0f, 120.0f, 250.0f, 500.0f, 2000.0f, 6000.0f
    };

    PRISMVSTAudioProcessor();
    ~PRISMVSTAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PRISMVST"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    float getInputPeakDb() const noexcept { return inputPeakDb.load(); }
    float getOutputPeakDb() const noexcept { return outputPeakDb.load(); }
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }
    void copySpectrum(std::array<float, spectrumBins>& destination) const noexcept;

    static juce::String sectionId(int section, const juce::String& suffix);

private:
    static constexpr int filterStages = 3;

    struct SectionState
    {
        std::array<std::array<juce::dsp::IIR::Filter<float>, filterStages>, 2> highPass;
        std::array<std::array<juce::dsp::IIR::Filter<float>, filterStages>, 2> lowPass;
        float envelope = 0.0f;
        float dynamicGainDb = 0.0f;
    };

    std::array<SectionState, numSections> sections;
    juce::AudioBuffer<float> dryScratch;
    juce::AudioBuffer<float> bandScratch;

    static constexpr int fftOrder = 14;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> fftWindow {
        fftSize, juce::dsp::WindowingFunction<float>::hann, true
    };
    std::array<float, fftSize * 2> fftData {};
    int fftWritePos = 0;
    std::array<std::atomic<float>, spectrumBins> spectrum {};

    double currentSampleRate = 44100.0;
    int currentMaxBlockSize = 512;

    std::atomic<float> inputPeakDb { -144.0f };
    std::atomic<float> outputPeakDb { -144.0f };
    std::atomic<float> gainReductionDb { 0.0f };

    void configureSectionFilters();
    void resetSectionFilters();
    void processTerritory(int sectionIndex, const juce::AudioBuffer<float>& dry,
                          juce::AudioBuffer<float>& output, bool soloMode);
    void pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer);
    void renderSpectrumFrame();

    static float read(const juce::AudioProcessorValueTreeState& state, const juce::String& id);
    static float peakDb(const juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessor)
};
