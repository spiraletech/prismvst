#pragma once
// Original Spiral Ether Tech surgical instrument control skin.
// Tactile light guides, accessible hit areas, high-DPI vector rendering.
#include <JuceHeader.h>
#include <cmath>
#include "OnyxRackSkin.h"
class SurgeryLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    SurgeryLookAndFeel(){
        setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff10191e));
        setColour(juce::PopupMenu::textColourId,juce::Colour(0xffd7e4e8));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff153d47));
        setColour(juce::PopupMenu::highlightedTextColourId,juce::Colour(0xff83e7ef));
        setColour(juce::ComboBox::textColourId,juce::Colour(0xffd7e4e8));
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override {return juce::Font(juce::FontOptions(13.0f));}
    juce::Font getPopupMenuFont() override {return juce::Font(juce::FontOptions(13.0f));}
    void positionComboBoxText(juce::ComboBox& box,juce::Label& label) override {
        label.setBounds(10,1,juce::jmax(0,box.getWidth()-36),box.getHeight()-2);
        label.setFont(getComboBoxFont(box));
        label.setJustificationType(juce::Justification::centredLeft);
    }
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox& box,juce::Label&) override {
        // Target component carries host DPI and our uniform editor transform.
        // No item-to-align: JUCE can place the complete list below, then fall
        // back above/clamp to its display work area when space is insufficient.
        return juce::PopupMenu::Options().withTargetComponent(&box)
            .withMinimumWidth(box.getWidth()).withMaximumNumColumns(1)
            .withStandardItemHeight(28).withInitiallySelectedItem(box.getSelectedId())
            .withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::downwards);
    }
    bool shouldPopupMenuScaleWithTargetComponent(const juce::PopupMenu::Options&) override {return true;}
    void drawPopupMenuItem(juce::Graphics& g,const juce::Rectangle<int>& area,bool separator,bool active,
                          bool highlighted,bool ticked,bool submenu,const juce::String& text,
                          const juce::String& shortcut,const juce::Drawable* icon,const juce::Colour* colour) override {
        juce::LookAndFeel_V4::drawPopupMenuItem(g,area,separator,active,highlighted||ticked,ticked,submenu,text,shortcut,icon,colour);
        if(ticked&&!separator){g.setColour(juce::Colour(0xff43d6e4));g.fillRect(area.getX()+2,area.getY()+4,2,area.getHeight()-8);}
    }
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
