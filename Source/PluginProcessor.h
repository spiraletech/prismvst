#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class PRISMVSTAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int numEqBands = 6;
    static constexpr int spectrumBins = 512;

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

    float getPeakDb() const noexcept { return peakDb.load(); }
    float getLufsShort() const noexcept { return lufsShort.load(); }
    float getLufsIntegrated() const noexcept { return lufsIntegrated.load(); }
    void copySpectrum(std::array<float, spectrumBins>& destination) const noexcept;

private:
    struct DynamicBand
    {
        std::array<juce::dsp::IIR::Filter<float>, 2> eq;
        std::array<juce::dsp::IIR::Filter<float>, 2> detector;
        float envelope = 0.0f;
        float smoothedGainDb = 0.0f;
    };

    std::array<DynamicBand, numEqBands> bands;
    std::array<juce::dsp::IIR::Filter<float>, 2> loudHp;
    std::array<juce::dsp::IIR::Filter<float>, 2> loudShelf;

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> fftWindow { fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, fftSize * 2> fftData {};
    int fftWritePos = 0;
    std::array<std::atomic<float>, spectrumBins> spectrum {};

    double currentSampleRate = 44100.0;
    double integratedEnergy = 0.0;
    uint64_t integratedSamples = 0;

    std::atomic<float> peakDb { -100.0f };
    std::atomic<float> lufsShort { -100.0f };
    std::atomic<float> lufsIntegrated { -100.0f };

    void updateBandCoefficients(int bandIndex);
    void processDynamicEq(juce::AudioBuffer<float>& buffer);
    void processOnyx(juce::AudioBuffer<float>& buffer);
    void updateMeters(const juce::AudioBuffer<float>& buffer);
    void pushAnalyzerSamples(const juce::AudioBuffer<float>& buffer);
    void renderSpectrumFrame();

    static juce::String bandId(int band, const juce::String& suffix);
    static float read(const juce::AudioProcessorValueTreeState& state, const juce::String& id);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessor)
};
