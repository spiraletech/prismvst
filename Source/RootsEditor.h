#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SurgerySpectrumFill.h"
#include "SurgeryLookAndFeel.h"
#include "SurgeryHeaderLayout.h"
#include "SurgeryNodeFocus.h"
#include "SurgeryPrecision.h"
#include "SurgeryResponse.h"
#include "SurgeryAnalysis.h"
#include <array>
#include <functional>
#include <memory>

class RootSelectorButton final : public juce::TextButton {
public:
    std::function<void()> onContext;
    void mouseDown(const juce::MouseEvent& event) override {
        if(event.mods.isPopupMenu()) {
            if(onContext)onContext();
            return;
        }
        juce::TextButton::mouseDown(event);
    }
};

class RootsGraph final : public juce::Component {
public:
    explicit RootsGraph(PRISMVSTAudioProcessor& owner);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
    void updateAnalyzer(const std::array<float,PRISMVSTAudioProcessor::labBars>&,
                        const std::array<float,PRISMVSTAudioProcessor::labBars>&);
    std::function<void(int)> onSelectNode;
    std::function<void(int)> onSelectCarve;
    int selectedNode() const noexcept{return selected_;}
    void selectNode(int node);
    void showRootMenu(int root);
    surgery::SpectralSnapshot spectralMetrics() const noexcept {
        return surgery::summarizeSpectrum(bands_);
    }
    void setFrozen(bool frozen) noexcept {frozen_=frozen;repaint();}
    bool isFrozen() const noexcept {return frozen_;}
    void setPresentation(int theme,int mode,int slot) {
        theme_=theme;mode_=mode;slot_=slot;repaint();
    }
private:
    PRISMVSTAudioProcessor& processor_;
    std::array<float,PRISMVSTAudioProcessor::labBars> bands_{},peaks_{};
    int selected_=0,dragging_=-1,draggingCarve_=-1;
    float dragStartX_=0.0f,dragStartHz_=0.0f;
    bool fineDragging_=false;
    int theme_=0,mode_=0,slot_=0;
    surgery::LiveSpectrumEnvelope<PRISMVSTAudioProcessor::labBars> spectrumEnvelope_;
    bool hover_=false,frozen_=false;
    float chromaPhase_=0.0f; // display-only foil phase; held during freeze
    juce::Point<float> mouse_;
    juce::Rectangle<float> area() const;
    float freqToX(float f) const;
    float xToFreq(float x) const;
    float param(int i,const char* suffix) const;
    void write(int i,const char* suffix,float value);
    int hitNode(juce::Point<float>) const;
    int hitCarveNode(juce::Point<float>) const;
    int hitCarveTab(juce::Point<float>) const;
    void popup();
    void clampCarvesToRoot(int root);
    juce::Colour colour(int territory) const;
};

class RootsEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit RootsEditor(PRISMVSTAudioProcessor&);
    ~RootsEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SliderA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonA=juce::AudioProcessorValueTreeState::ButtonAttachment;
    PRISMVSTAudioProcessor& processor_;
    RootsGraph graph_;
    SurgeryLookAndFeel lookAndFeel_;
    std::array<RootSelectorButton,7> rootButtons_;
    juce::Slider gain_,threshold_,analyzerSlope_,analyzerMemory_;
    juce::ComboBox analyzerDepth_;
    juce::TextButton heatToggle_{"SONAR ON"},freezeButton_{"FREEZE"};
    juce::TextButton mono_{"MONO"},on_{"ON"},solo_{"SOLO"};
    juce::TextButton themeButton_{"THEME"};
    int themeIndex_=0,mode_=0,carveSlot_=0;
    std::array<juce::TextButton,4> modeButtons_;
    std::array<juce::TextButton,4> carveSlotButtons_;
    juce::Slider carveFrequency_,carveDepth_,carveQ_,carveThreshold_;
    std::array<juce::Slider,4> onyxSliders_;
    std::array<juce::Label,4> onyxLabels_;
    std::array<std::unique_ptr<SliderA>,4> onyxAttachments_;
    juce::TextButton carveOn_{"CARVE ON"},carveDynamic_{"DYNAMIC"};
    juce::Label carveFrequencyLabel_,carveDepthLabel_,carveQLabel_,carveThresholdLabel_;
    std::unique_ptr<SliderA> carveFrequencyA_,carveDepthA_,carveQA_,carveThresholdA_;
    std::unique_ptr<ButtonA> carveOnA_,carveDynamicA_;
    juce::Label title_,selection_,status_,gainLabel_,thresholdLabel_,
        analyzerDepthLabel_,analyzerSlopeLabel_,analyzerMemoryLabel_,analyzerLegend_;
    using ComboA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderA> gainA_,thresholdA_,analyzerSlopeA_,analyzerMemoryA_;
    std::unique_ptr<ComboA> analyzerDepthA_;
    std::unique_ptr<ButtonA> heatA_;
    std::unique_ptr<ButtonA> monoA_,onA_,soloA_;
    int node_=0;
    void bindNode(int n);
    void bindCarve();
    void setMode(int mode);
    void updateRootButtons();
    void timerCallback() override;
};
