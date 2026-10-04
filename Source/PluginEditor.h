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
    void setPrecisionMode(int mode) noexcept { precisionMode = juce::jlimit(0, 2, mode); }

private:
    double dragStartProportion = 0.0;
    int dragStartY = 0;
    int precisionMode = 0;

    static constexpr double normalPixelsForFullRange = 700.0;
    static constexpr double finePixelsForFullRange = 3000.0;
    static constexpr double microPixelsForFullRange = 6000.0;
};

class EtherTechLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
};

class SpectrumAuraDisplay final : public juce::Component
{
public:
    explicit SpectrumAuraDisplay(PRISMVSTAudioProcessor&);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

    void pushSpectrum(const std::array<float, PRISMVSTAudioProcessor::spectrumBins>&);
    void setSelectedBand(int band);
    int getSelectedBand() const noexcept { return selectedBand; }

    std::function<void(int)> onBandSelected;

private:
    static constexpr int auraColumns = 160;

    PRISMVSTAudioProcessor& processor;
    int selectedBand = 0;
    std::array<float, PRISMVSTAudioProcessor::spectrumBins> latestSpectrum {};
    std::array<float, auraColumns> auraEnergy {};
    bool hasHover = false;
    juce::Point<float> hoverPoint;

    juce::Rectangle<float> graphBounds() const;
    float frequencyToX(float frequency) const;
    float xToFrequency(float x) const;
    float levelToY(float db) const;
    float displayFloorDb() const;
    float parameter(const juce::String& id) const;
    float spectrumDbAt(float frequency) const;
    float displayedSpectrumDbAt(float frequency) const;
    int domainForFrequency(float frequency) const;
    float domainBumperFrequency(int leftDomain) const;
    juce::Colour auraColourFor(float frequency, float& affinity, float& referenceHz) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAuraDisplay)
};

class DynamicsTransferDisplay final : public juce::Component
{
public:
    explicit DynamicsTransferDisplay(PRISMVSTAudioProcessor&);

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    void setSelectedBand(int band);

private:
    enum class DragTarget { none, threshold, ratio };

    PRISMVSTAudioProcessor& processor;
    int selectedBand = 0;
    DragTarget dragTarget = DragTarget::none;
    float dragStartValue = 0.0f;
    float dragStartPixel = 0.0f;

    juce::Rectangle<float> graphBounds() const;
    juce::String id(const juce::String& suffix) const;
    float parameter(const juce::String& suffix) const;
    void setParameter(const juce::String& suffix, float value);
    juce::RangedAudioParameter* rangedParameter(const juce::String& suffix) const;

    float dbToX(float db) const;
    float dbToY(float db) const;
    float xToDb(float x) const;
    float yToDb(float y) const;
    juce::Point<float> thresholdPoint() const;
    juce::Point<float> ratioPoint() const;
    float dragScale(const juce::ModifierKeys&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicsTransferDisplay)
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
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    PRISMVSTAudioProcessor& processor;
    EtherTechLookAndFeel lookAndFeel;
    SpectrumAuraDisplay spectrumDisplay;
    DynamicsTransferDisplay transferDisplay;

    int selectedBand = 0;

    std::array<juce::TextButton, PRISMVSTAudioProcessor::numEqBands> domainButtons;
    juce::Label selectedBandLabel;

    std::array<juce::Label, 6> controlLabels;
    PrecisionSlider frequency, gain, q, dynamicRange, attack, release;

    PrecisionSlider onyx, onyxDrive, masterTrim, ceiling;
    PrecisionSlider analyzerSlope, auraMemory, colorAffinity;
    juce::ComboBox analyzerDepth, precisionMode;

    juce::ToggleButton solfeggio { "AURA COLOR" };
    juce::ToggleButton masterBypass { "BYPASS" };

    juce::Label peakLabel, lufsShortLabel, lufsIntLabel;
    juce::Label onyxLabel, driveLabel, trimLabel, ceilingLabel;
    juce::Label analyzerSlopeLabel, auraMemoryLabel, affinityLabel, depthLabel, precisionLabel;

    std::unique_ptr<SliderAttachment> frequencyA, gainA, qA, dynamicRangeA;
    std::unique_ptr<SliderAttachment> attackA, releaseA;
    std::unique_ptr<SliderAttachment> onyxA, onyxDriveA, masterTrimA, ceilingA;
    std::unique_ptr<SliderAttachment> analyzerSlopeA, auraMemoryA, colorAffinityA;
    std::unique_ptr<ComboBoxAttachment> analyzerDepthA, precisionModeA;
    std::unique_ptr<ButtonAttachment> solfeggioA, masterBypassA;

    void configureRotary(juce::Slider&, const juce::String& suffix = {});
    void enableDefaultReset(juce::Slider&, const juce::String& parameterId);
    void bindSelectedBand(int band);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PRISMVSTAudioProcessorEditor)
};
