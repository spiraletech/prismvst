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
                          const juce::Slider::SliderStyle, juce::Slider&) override;
};

class SpectrumDisplay final : public juce::Component
{
public:
    explicit SpectrumDisplay(PRISMVSTAudioProcessor&);

    void paint(juce::Graphics&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;

    void pushSpectrum(const std::array<float, PRISMVSTAudioProcessor::spectrumBins>&);
    void setSelectedSection(int section);
    std::function<void(int)> onSectionSelected;

private:
    PRISMVSTAudioProcessor& processor;
    int selectedSection = 0;
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> latestSpectrum {};
    std::array<float, 256> auraEnvelope {};
    juce::Point<float> hoverPoint {};
    bool hovering = false;

    juce::Rectangle<float> graphBounds() const;
    float frequencyToX(float frequency) const;
    float xToFrequency(float x) const;
    float displayDbToY(float db) const;
    float rawDbAtFrequency(float frequency) const;
    int sectionForFrequency(float frequency) const;
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

    int selectedSection = 0;

    std::array<std::unique_ptr<juce::TextButton>, PRISMVSTAudioProcessor::numSections> sectionButtons;
    juce::TextButton enabledButton { "ON" };
    juce::TextButton soloButton { "SOLO" };

    juce::Slider inputSlider;
    juce::Slider outputSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider widthSlider;
    juce::Slider onyxSlider;
    juce::Slider precisionLever;

    juce::Label selectedSectionLabel;
    juce::Label statusLabel;
    juce::Label inputMeterLabel;
    juce::Label grMeterLabel;
    juce::Label outputMeterLabel;

    std::unique_ptr<SliderAttachment> inputA;
    std::unique_ptr<SliderAttachment> outputA;
    std::unique_ptr<SliderAttachment> attackA;
    std::unique_ptr<SliderAttachment> releaseA;
    std::unique_ptr<SliderAttachment> widthA;
    std::unique_ptr<SliderAttachment> onyxA;
    std::unique_ptr<ButtonAttachment> enabledA;
    std::unique_ptr<ButtonAttachment> soloA;

    void configureKnob(juce::Slider&, const juce::String& suffix,
                       double defaultValue, double interval = 0.01);
    void configureLever();
    void bindSelectedSection(int section);
    void updatePrecisionSensitivity();
    void timerCallback() override;
    void paintMetalPanel(juce::Graphics&, juce::Rectangle<float>, bool lighter) const;
    void paintOutputMeter(juce::Graphics&, juce::Rectangle<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessorEditor)
};
