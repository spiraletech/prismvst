#pragma once
// Original Spiral Ether Tech surgical instrument control skin.
// Tactile light guides, accessible hit areas, high-DPI vector rendering.
#include <JuceHeader.h>
#include <cmath>
#include "OnyxRackSkin.h"
class SurgeryLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,
                          float start,float end,juce::Slider&) override {
        surgery::rack::knob(g,x,y,w,h,position,start,end);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& button,
                              const juce::Colour&,bool over,bool down) override {
        surgery::rack::bezel(g,button.getLocalBounds().toFloat().reduced(2.0f),button.getToggleState(),3);
        if(over||down){g.setColour(juce::Colours::white.withAlpha(down?.1f:.04f));g.fillPath(surgery::rack::cut(button.getLocalBounds().toFloat().reduced(3),3));}
    }
    void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) override {
        g.setColour(b.isEnabled()?juce::Colour(0xffe4f0f1):juce::Colour(0xff83969c));
        g.setFont(juce::Font(juce::FontOptions(juce::jlimit(11.0f,13.0f,b.getHeight()*.43f),juce::Font::bold)));
        g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(5,2),
                         juce::Justification::centred,1);
    }
    void drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox& box) override {
        surgery::rack::bezel(g,juce::Rectangle<float>(1,1,(float)w-2,(float)h-2),false,2);
        g.setColour(juce::Colour(0xffd7eff4));
        juce::Path arrow;arrow.startNewSubPath((float)w-20,h*.42f);arrow.lineTo((float)w-15,h*.6f);arrow.lineTo((float)w-10,h*.42f);
        g.strokePath(arrow,juce::PathStrokeType(1.6f));
    }
    void drawLabel(juce::Graphics& g,juce::Label& label) override {
        if(dynamic_cast<juce::Slider*>(label.getParentComponent())!=nullptr){
            const auto r=label.getLocalBounds().toFloat().reduced(.5f);
            g.setColour(juce::Colour(0xff010608));g.fillRoundedRectangle(r,1.5f);
            g.setColour(juce::Colour(0xff00bbd3).withAlpha(.65f));g.drawRoundedRectangle(r,1.5f,.7f);
            if(!label.isBeingEdited()){
                g.setColour(juce::Colour(0xffe9f9fc));g.setFont(juce::Font(juce::FontOptions(12.5f,juce::Font::bold)));
                g.drawFittedText(label.getText(),label.getLocalBounds().reduced(2),juce::Justification::centred,1);
            }
            return;
        }
        juce::LookAndFeel_V4::drawLabel(g,label);
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float pos,float min,float max,
                          juce::Slider::SliderStyle style,juce::Slider& slider) override {
        if(style!=juce::Slider::LinearHorizontal){juce::LookAndFeel_V4::drawLinearSlider(g,x,y,width,height,pos,min,max,style,slider);return;}
        auto track=juce::Rectangle<float>((float)x,(float)y+height*.35f,(float)width,height*.3f);
        g.setColour(juce::Colour(0xff010506));g.fillRoundedRectangle(track.expanded(2),3);
        g.setColour(juce::Colour(0xff19353d));g.fillRoundedRectangle(track,2);
        auto fill=track.withWidth(juce::jlimit(0.f,(float)width,pos-x));
        juce::ColourGradient light(juce::Colour(0xff00efff),fill.getX(),fill.getY(),juce::Colour(0xff008ea9),fill.getRight(),fill.getY(),false);
        g.setGradientFill(light);g.fillRoundedRectangle(fill,2);
        g.setColour(juce::Colour(0xff00d9ec));g.fillEllipse(pos-6,track.getCentreY()-6,12,12);
        g.setColour(juce::Colour(0xffb2f5ff));g.drawEllipse(pos-6,track.getCentreY()-6,12,12,.7f);
    }
};
