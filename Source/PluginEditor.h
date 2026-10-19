#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <memory>

class PrismLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PrismLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool highlighted, bool down) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;
};

class SpectrumDisplay final : public juce::Component
{
public:
    explicit SpectrumDisplay(PRISMVSTAudioProcessor&);

    void paint(juce::Graphics&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

    void pushSpectrum(const std::array<float, PRISMVSTAudioProcessor::spectrumBins>&);
    void setSelectedBand(int band);

private:
    PRISMVSTAudioProcessor& processor;
    int selectedBand = 0;

    std::array<float, PRISMVSTAudioProcessor::spectrumBins> latestSpectrum {};
    std::array<float, 192> auraPersistence {};
    juce::Point<float> hoverPoint {};
    bool hovering = false;

    juce::Rectangle<float> graphBounds() const;
    float frequencyToX(float frequency) const;
    float xToFrequency(float x) const;
    float displayDbToY(float db) const;
    float rawDbAtFrequency(float frequency) const;
    float parameter(const juce::String& id) const;
    juce::String bandId(int band, const juce::String& suffix) const;
    juce::String noteForFrequency(float frequency) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumDisplay)
};

class PRISMVSTAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor&);
    ~PRISMVSTAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    PRISMVSTAudioProcessor& processor;
    PrismLookAndFeel lookAndFeel;
    SpectrumDisplay spectrumDisplay;

    int selectedBand = 0;

    std::array<std::unique_ptr<juce::TextButton>, PRISMVSTAudioProcessor::numEqBands> bandButtons;
    juce::TextButton enabledButton { "ON" };
    juce::TextButton bypassButton { "BYPASS" };

    juce::Slider frequencySlider;
    juce::Slider gainSlider;
    juce::Slider dynamicSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider onyxSlider;
    juce::Slider precisionLever;

    juce::Label selectedBandLabel;
    juce::Label statusLabel;
    juce::Label peakLabel;
    juce::Label lufsShortLabel;
    juce::Label lufsIntegratedLabel;

    std::unique_ptr<SliderAttachment> frequencyA;
    std::unique_ptr<SliderAttachment> gainA;
    std::unique_ptr<SliderAttachment> dynamicA;
    std::unique_ptr<SliderAttachment> attackA;
    std::unique_ptr<SliderAttachment> releaseA;
    std::unique_ptr<SliderAttachment> onyxA;
    std::unique_ptr<ButtonAttachment> enabledA;
    std::unique_ptr<ButtonAttachment> bypassA;

    void configureKnob(juce::Slider&, const juce::String& suffix,
                       double defaultValue, int decimals = 2);
    void configureLever();
    void bindSelectedBand(int band);
    void updatePrecisionSensitivity();
    void timerCallback() override;

    void paintMetalPanel(juce::Graphics&, juce::Rectangle<float>, bool lighter) const;
    void paintMeter(juce::Graphics&, juce::Rectangle<float>, float db) const;

    juce::String bandId(int band, const juce::String& suffix) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessorEditor)
};
