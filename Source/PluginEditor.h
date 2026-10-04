#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>

class PrecisionSlider final : public juce::Slider
{
public:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    double dragStartValue = 0.0;
    int dragStartY = 0;
    bool precisionDrag = false;
    static constexpr double precisionPixelsForFullRange = 3000.0;
};

class SpectrumEQDisplay final : public juce::Component
{
public:
    explicit SpectrumEQDisplay(PRISMVSTAudioProcessor&);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    void pushSpectrum(const std::array<float, PRISMVSTAudioProcessor::spectrumBins>&);
    void setSelectedBand(int band);
    int getSelectedBand() const noexcept { return selectedBand; }

    std::function<void(int)> onBandSelected;

private:
    static constexpr int heatColumns = 144;
    static constexpr int heatRows = 42;

    PRISMVSTAudioProcessor& processor;
    int selectedBand = 0;
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> latestSpectrum {};
    std::array<std::array<float, heatColumns>, heatRows> heatHistory {};
    int heatWriteRow = 0;

    juce::Rectangle<float> graphBounds() const;
    float frequencyToX(float frequency) const;
    float xToFrequency(float x) const;
    float gainToY(float gainDb) const;
    float yToGain(float y) const;
    juce::Point<float> nodePosition(int band) const;
    int findNearestNode(juce::Point<float>) const;
    float parameter(const juce::String& id) const;
    void setParameter(const juce::String& id, float value);
    juce::String id(int band, const juce::String& suffix) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumEQDisplay)
};

class PRISMVSTAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor&);
    ~PRISMVSTAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    PRISMVSTAudioProcessor& processor;
    SpectrumEQDisplay spectrumDisplay;

    int selectedBand = 0;

    juce::Label selectedBandLabel;
    std::array<juce::Label, 8> controlLabels;
    PrecisionSlider frequency, gain, q, dynamicRange, threshold, ratio, attack, release;

    PrecisionSlider onyx, onyxDrive, masterTrim, ceiling;
    juce::ToggleButton solfeggio { "SOLFEGGIO" };
    juce::ToggleButton masterBypass { "BYPASS" };

    juce::Label peakLabel, lufsShortLabel, lufsIntLabel;
    juce::Label onyxLabel, driveLabel, trimLabel, ceilingLabel;

    std::unique_ptr<SliderAttachment> frequencyA, gainA, qA, dynamicRangeA;
    std::unique_ptr<SliderAttachment> thresholdA, ratioA, attackA, releaseA;
    std::unique_ptr<SliderAttachment> onyxA, onyxDriveA, masterTrimA, ceilingA;
    std::unique_ptr<ButtonAttachment> solfeggioA, masterBypassA;

    void configureRotary(juce::Slider&, const juce::String& suffix = {});
    void bindSelectedBand(int band);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessorEditor)
};
