#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
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
    int selectedNode() const noexcept{return selected_;}
    void selectNode(int node);
    void showRootMenu(int root);
private:
    PRISMVSTAudioProcessor& processor_;
    std::array<float,PRISMVSTAudioProcessor::labBars> bands_{},peaks_{};
    int selected_=0,dragging_=-1;
    bool hover_=false;
    juce::Point<float> mouse_;
    juce::Rectangle<float> area() const;
    float freqToX(float f) const;
    float xToFreq(float x) const;
    float param(int i,const char* suffix) const;
    void write(int i,const char* suffix,float value);
    int hitNode(juce::Point<float>) const;
    void popup();
    static juce::Colour colour(int territory);
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
    std::array<RootSelectorButton,7> rootButtons_;
    juce::Slider gain_,bandSpace_,space_,threshold_;
    juce::TextButton mono_{"MONO"},on_{"ON"},solo_{"SOLO"};
    juce::TextButton themeButton_{"THEME"};
    int themeIndex_=0;
    juce::Label title_,selection_,status_,gainLabel_,bandSpaceLabel_,
        spaceLabel_,thresholdLabel_;
    std::unique_ptr<SliderA> gainA_,bandSpaceA_,spaceA_,thresholdA_;
    std::unique_ptr<ButtonA> monoA_,onA_,soloA_;
    int node_=0;
    void bindNode(int n);
    void updateRootButtons();
    void timerCallback() override;
};
