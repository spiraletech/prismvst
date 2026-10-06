#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class PRISMVSTAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int numSections = 7;
    static constexpr int numCrossovers = numSections - 1;
    static constexpr int spectrumBins = 512;

    PRISMVSTAudioProcessor();
    ~PRISMVSTAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PRISM Alpha 001"; }
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
    float getPeakDb() const noexcept { return peakDb.load(); }
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }
    float getLufsShort() const noexcept { return lufsShort.load(); }
    float getLufsIntegrated() const noexcept { return lufsIntegrated.load(); }
    void copySpectrum(std::array<float, spectrumBins>& destination) const noexcept;

    static juce::String sectionId(int section, const juce::String& suffix);
    static juce::String crossoverId(int crossover);
    static const char* sectionName(int section) noexcept;

private:
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        float process(float x) noexcept;
        void reset() noexcept { z1 = z2 = 0.0f; }
        void setFirstOrderLowPass(double sampleRate, float frequency) noexcept;
        void setFirstOrderHighPass(double sampleRate, float frequency) noexcept;
        void setSecondOrderLowPass(double sampleRate, float frequency, float q) noexcept;
        void setSecondOrderHighPass(double sampleRate, float frequency, float q) noexcept;
        void setFirstOrderAllPass(double sampleRate, float frequency) noexcept;
        void setSecondOrderAllPass(double sampleRate, float frequency, float q) noexcept;
    };

    // A fixed 36 dB/oct Linkwitz-Riley-style electrical crossover:
    // two cascaded 3rd-order Butterworth sections on each branch.
    // The high branch is polarity-corrected so low+high is all-pass / flat magnitude.
    struct LR6Splitter
    {
        std::array<Biquad, 4> low;
        std::array<Biquad, 4> high;

        void reset() noexcept;
        void setCutoff(double sampleRate, float frequency) noexcept;
        void process(float x, float& lowOut, float& highOut) noexcept;
    };

    // The low+high sum of an LR6 split is a 3rd-order all-pass. Applying this
    // to earlier bands phase-aligns a serial multiband bank without changing magnitude.
    struct ThirdOrderAllPass
    {
        Biquad first;
        Biquad second;

        void reset() noexcept;
        void setCutoff(double sampleRate, float frequency) noexcept;
        float process(float x) noexcept;
    };

    struct SectionState
    {
        float envelope = 0.0f;
        float gainReductionDb = 0.0f;
    };

    std::array<std::array<LR6Splitter, 2>, numCrossovers> splitters;
    std::array<std::array<std::array<ThirdOrderAllPass, 2>, numSections>, numCrossovers> compensators;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>, numCrossovers> crossoverSmoothers;
    std::array<SectionState, numSections> sectionStates;

    std::array<juce::dsp::IIR::Filter<float>, 2> loudHp;
    std::array<juce::dsp::IIR::Filter<float>, 2> loudShelf;

    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> fftWindow { fftSize, juce::dsp::WindowingFunction<float>::hann };
    static constexpr int fftHopSize = 768;
    static constexpr int analyzerQueueSize = 65536;

    class AnalyzerWorker final : public juce::Thread
    {
    public:
        explicit AnalyzerWorker(PRISMVSTAudioProcessor& ownerIn)
            : juce::Thread("PRISM Analyzer"), owner(ownerIn) {}
        void run() override { owner.runAnalyzerThread(); }

    private:
        PRISMVSTAudioProcessor& owner;
    };

    juce::AbstractFifo analyzerFifo { analyzerQueueSize };
    std::array<float, analyzerQueueSize> analyzerQueue {};
    juce::WaitableEvent analyzerEvent;
    AnalyzerWorker analyzerWorker { *this };

    std::array<float, fftSize> fftFifo {};
    std::array<float, fftSize * 2> fftData {};
    int fftWritePos = 0;
    int fftHopCounter = 0;
    std::array<std::atomic<float>, spectrumBins> spectrum {};

    double currentSampleRate = 44100.0;
    double integratedEnergy = 0.0;
    uint64_t integratedSamples = 0;

    std::atomic<float> inputPeakDb { -144.0f };
    std::atomic<float> peakDb { -144.0f };
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<float> lufsShort { -144.0f };
    std::atomic<float> lufsIntegrated { -144.0f };

    void sanitizeCrossovers();
    void updateCrossoverCoefficients(int blockSamples);
    void processSevenSectionEngine(juce::AudioBuffer<float>& buffer);
    void updateMeters(const juce::AudioBuffer<float>& buffer);
    void pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer);
    void runAnalyzerThread();
    void consumeAnalyzerSample(float sample);
    void renderSpectrumFrame();

    static float read(const juce::AudioProcessorValueTreeState& state, const juce::String& id);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessor)
};
