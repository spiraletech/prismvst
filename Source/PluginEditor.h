#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

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

class PRISMVSTAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PRISMVSTAudioProcessorEditor(PRISMVSTAudioProcessor&);
    ~PRISMVSTAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct BandUI {
        juce::Label title;
        PrecisionSlider scrape, depth, tone, density, trim;
        juce::ToggleButton solo { "S" }, bypass { "B" };
        std::unique_ptr<SliderAttachment> aScrape, aDepth, aTone, aDensity, aTrim;
        std::unique_ptr<ButtonAttachment> aSolo, aBypass;
    };

    PRISMVSTAudioProcessor& processor;
    std::array<BandUI, 5> bandUi;
    PrecisionSlider masterTrim, ceiling;
    juce::ToggleButton masterBypass { "BYPASS" };
    juce::Label peakLabel, lufsShortLabel, lufsIntLabel;

    std::unique_ptr<SliderAttachment> masterTrimA, ceilingA;
    std::unique_ptr<ButtonAttachment> masterBypassA;

    void configureSlider(juce::Slider&, const juce::String& suffix = {});
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessorEditor)
};
