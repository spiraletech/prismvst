#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class PRISMVSTAudioProcessor final : public juce::AudioProcessor
{
public:
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

private:
    static constexpr int numBands = 5;
    enum Band : int { Sub = 0, Kick, Low, Mid, High };

    struct BandState {
        juce::AudioBuffer<float> buffer;
        float envelope = 0.0f;
        float gainSmooth = 1.0f;
    };

    std::array<BandState, numBands> bands;
    std::array<std::array<juce::dsp::LinkwitzRileyFilter<float>, 2>, 4> lp;
    std::array<std::array<juce::dsp::LinkwitzRileyFilter<float>, 2>, 4> hp;

    std::array<juce::dsp::IIR::Filter<float>, 2> loudHp;
    std::array<juce::dsp::IIR::Filter<float>, 2> loudShelf;

    double currentSampleRate = 44100.0;
    double integratedEnergy = 0.0;
    uint64_t integratedSamples = 0;

    std::atomic<float> peakDb { -100.0f };
    std::atomic<float> lufsShort { -100.0f };
    std::atomic<float> lufsIntegrated { -100.0f };

    void splitBands(const juce::AudioBuffer<float>& source);
    void processBand(int bandIndex, int numSamples);
    void updateMeters(const juce::AudioBuffer<float>& buffer);

    static juce::String bandPrefix(int i);
    static float read(const juce::AudioProcessorValueTreeState& state, const juce::String& id);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessor)
};
