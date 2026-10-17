#pragma once
// Original Spiral Ether Tech surgical instrument control skin.
// Tactile light guides, accessible hit areas, high-DPI vector rendering.
#include <JuceHeader.h>
#include <cmath>
class SurgeryLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void setTheme(int t) noexcept {theme_=juce::jlimit(0,3,t);}
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,
                          float start,float end,juce::Slider&) override {
        const float cx=x+w*0.5f,cy=y+h*0.48f;
        const float r=juce::jmax(13.0f,juce::jmin(w,h)*0.36f);
        const auto shadow=juce::Rectangle<float>(cx-r-3,cy-r+2,2*r+6,2*r+6);
        g.setColour(juce::Colour(0xff03070b).withAlpha(0.65f));
        g.fillEllipse(shadow);
        juce::ColourGradient body(
            theme_==1?juce::Colour(0xfff7f8f7):juce::Colour(0xffa5b5bd),
            cx-r,cy-r,theme_==1?juce::Colour(0xff586970):juce::Colour(0xff263742),
            cx+r,cy+r,false);
        body.addColour(0.42,theme_==1?juce::Colour(0xffcfd8d9):juce::Colour(0xff637884));
        g.setGradientFill(body);
        g.fillEllipse(cx-r,cy-r,2*r,2*r);
        g.setColour(juce::Colour(0xff14202a));
        g.drawEllipse(cx-r,cy-r,2*r,2*r,1.4f);
        g.setColour(juce::Colour(0xff091319));
        g.fillEllipse(cx-r*.74f,cy-r*.74f,1.48f*r,1.48f*r);
        juce::ColourGradient inner(juce::Colour(0xff566a76),cx-r,cy-r,
                                   juce::Colour(0xff111e29),cx+r,cy+r,false);
        g.setGradientFill(inner);
        g.fillEllipse(cx-r*.65f,cy-r*.65f,1.3f*r,1.3f*r);
        const auto accent=theme_==2?juce::Colour(0xff76ebba):
                          theme_==3?juce::Colour(0xffddaa7d):
                                    juce::Colour(0xff7adbe8);
        juce::Path outer;
        outer.addCentredArc(cx,cy,r*1.08f,r*1.08f,0.0f,start,end,true);
        g.setColour(juce::Colour(0xff344d56));
        g.strokePath(outer,juce::PathStrokeType(2.8f));
        juce::Path active;
        active.addCentredArc(cx,cy,r*1.08f,r*1.08f,0.0f,start,start+position*(end-start),true);
        g.setColour(accent);
        g.strokePath(active,juce::PathStrokeType(3.0f));
        const float theta=start+position*(end-start);
        g.setColour(juce::Colours::white);
        g.drawLine(cx,cy,cx+std::sin(theta)*r*.53f,
                   cy-std::cos(theta)*r*.53f,2.1f);
        g.setColour(juce::Colours::white.withAlpha(.13f));
        g.drawEllipse(cx-r*.56f,cy-r*.56f,1.12f*r,1.12f*r,.8f);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& button,
                              const juce::Colour&,bool over,bool down) override {
        const auto r=button.getLocalBounds().toFloat().reduced(1.5f);
        const bool enabled=button.getToggleState();
        const auto accent=theme_==2?juce::Colour(0xff50dcb1):
                          theme_==3?juce::Colour(0xffdda67a):
                                    juce::Colour(0xff65bdd4);
        juce::ColourGradient metallic(enabled?juce::Colour(0xff2b5964):juce::Colour(0xff33444d),
                                      r.getX(),r.getY(),
                                      juce::Colour(0xff0a131a),r.getX(),r.getBottom(),false);
        g.setGradientFill(metallic);
        g.fillRoundedRectangle(r,4.0f);
        g.setColour(enabled?accent:juce::Colour(0xff62737b));
        g.drawRoundedRectangle(r,4.0f,enabled?1.5f:0.9f);
        if(enabled){
            g.setColour(accent.withAlpha(.93f));
            g.fillRoundedRectangle(r.getX()+5,r.getY()+3,r.getWidth()-10,2.0f,1.0f);
        }
        if(down||over){
            g.setColour(juce::Colours::white.withAlpha(down?.08f:.035f));
            g.fillRoundedRectangle(r,4.0f);
        }
    }
    void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) override {
        g.setColour(b.isEnabled()?juce::Colour(0xffe4f0f1):juce::Colour(0xff83969c));
        g.setFont(juce::Font(juce::FontOptions(juce::jlimit(11.0f,13.0f,b.getHeight()*.43f),juce::Font::bold)));
        g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(5,2),
                         juce::Justification::centred,1);
    }
private:
    int theme_=0;
};
