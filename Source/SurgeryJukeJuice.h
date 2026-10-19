#pragma once
#include <JuceHeader.h>
#include "SurgeryWeatherInterpretation.h"
#include <algorithm>
#include <cmath>

namespace surgery::juke {

// UI-thread-only presentation state. The existing Storm Symbiote analyzer
// supplies every input; the rack never reads audio or touches DSP parameters.
struct Liquid final {
    float level=0.f, bass=0.f, flux=0.f, width=0.f, colour=0.f, flash=0.f, phase=0.f;
    bool update(const surgery::weather::Meaning& m,double seconds,bool active,bool reduced,bool frozen) noexcept {
        const float oldLevel=level,oldBass=bass,oldFlux=flux,oldWidth=width,oldColour=colour,oldFlash=flash,oldPhase=phase;
        if (!active) {
            level=bass=flux=width=colour=flash=phase=0.f;
        } else if (!frozen) {
            const auto dt=(float)std::clamp(std::isfinite(seconds)?seconds:0.,0.,.25);
            auto soften=[dt](float& value,float next) {
                next=std::clamp(std::isfinite(next)?next:0.f,0.f,1.f);
                const float tau=next>value?.045f:.40f;
                value=next+(value-next)*std::exp(-dt/tau);
                if (value<.0005f)value=0.f;
            };
            soften(level,m.level);soften(bass,m.bass);soften(flux,m.flux);
            soften(width,m.width);soften(colour,m.centroid);
            soften(flash,reduced?0.f:m.flash);
            if (!reduced && level>.002f)
                phase=std::fmod(phase+dt*(.18f+.35f*bass+.20f*flux),6.283185307f);
        }
        return std::abs(oldLevel-level)>.0004f||std::abs(oldBass-bass)>.0004f
            ||std::abs(oldFlux-flux)>.0004f||std::abs(oldWidth-width)>.0004f
            ||std::abs(oldColour-colour)>.0004f||std::abs(oldFlash-flash)>.0004f
            ||std::abs(oldPhase-phase)>.0004f;
    }
};

inline void screw(juce::Graphics& g,float x,float y) {
    // Four understated store-bought silver Phillips X-head fasteners.
    const auto circle=juce::Rectangle<float>(x-14,y-14,28,28);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd8dde0),x-7,y-8,juce::Colour(0xff4e5961),x+10,y+12,false));
    g.fillEllipse(circle);
    g.setColour(juce::Colour(0xff0b1015));g.drawEllipse(circle,1.5f);
    g.setColour(juce::Colour(0xffa9b2b9));g.fillEllipse(x-9,y-9,18,18);
    g.setColour(juce::Colour(0xff273039));
    g.drawLine(x-4.8f,y-4.8f,x+4.8f,y+4.8f,3.f);
    g.drawLine(x-4.8f,y+4.8f,x+4.8f,y-4.8f,3.f);
    g.setColour(juce::Colour(0xffd5d8d9).withAlpha(.7f));
    g.drawEllipse(x-10,y-10,20,20,.8f);
}

inline void paintStatic(juce::Graphics& g,int width,int height) {
    // Simple black-carbon jukebox frame, anchored to real editor bounds.
    // No old hazard artwork, no new UI, and no large background PNG scaling.
    g.fillAll(juce::Colour(0xff030a11));
    const auto carbon=juce::Colour(0xff101820);
    g.setColour(carbon);
    g.fillRect(0,0,width,48);
    g.fillRect(0,height-48,width,48);
    g.fillRect(0,0,83,height);
    g.fillRect(width-83,0,83,height);
    {
        juce::Graphics::ScopedSaveState saved(g);
        juce::Path mask;
        mask.addRectangle(0.f,0.f,(float)width,48.f);
        mask.addRectangle(0.f,(float)height-48,(float)width,48.f);
        mask.addRectangle(0.f,0.f,83.f,(float)height);
        mask.addRectangle((float)width-83,0.f,83.f,(float)height);
        g.reduceClipRegion(mask);
        // Fine diagonal weave, pre-rendered only on resize/DPI changes.
        for(int x=-height;x<width+height;x+=10) {
            g.setColour(juce::Colour(0xff34414a).withAlpha(.38f));
            g.drawLine((float)x,0.f,(float)x+height,(float)height,.8f);
            g.setColour(juce::Colour(0xff010408).withAlpha(.65f));
            g.drawLine((float)x+3,0.f,(float)x+3+height,(float)height,.9f);
        }
    }
    const auto inner=juce::Rectangle<float>(82.f,46.f,(float)width-164.f,(float)height-94.f);
    g.setColour(juce::Colour(0xff253746));g.drawRoundedRectangle(inner,17.f,2.0f);
    g.setColour(juce::Colour(0xff01070d));g.drawRoundedRectangle(inner.expanded(2.f),18.f,2.f);

    const auto rail=[&](juce::Rectangle<float> r,float radius) {
        g.setColour(juce::Colour(0xff01060d));g.fillRoundedRectangle(r.expanded(4.f),radius+4.f);
        g.setColour(juce::Colour(0xff172a39));g.fillRoundedRectangle(r,radius);
        g.setColour(juce::Colour(0xff5b6a72).withAlpha(.55f));g.drawRoundedRectangle(r,radius,1.1f);
    };
    rail({116.f,9.f,(float)width-232.f,30.f},15.f);
    rail({23.f,102.f,41.f,(float)height-204.f},20.f);
    rail({(float)width-64.f,102.f,41.f,(float)height-204.f},20.f);
    rail({116.f,(float)height-28.f,(float)width-232.f,9.f},4.5f);
    for(float x:{42.f,(float)width-42.f})for(float y:{43.f,(float)height-43.f})
        screw(g,x,y);
}

inline void paintLiquid(juce::Graphics& g,int width,int height,const Liquid& state) {
    if(state.level<.001f && state.flash<.001f)return; // Actually dark at silence or SONAR OFF.
    const float energy=std::clamp(.18f+.82f*state.level,0.f,1.f)*state.level;
    const float hot=std::clamp(.23f+.45f*state.colour+.32f*state.flux,0.f,1.f);
    const auto cool=juce::Colour(0xff43bbff),mid=juce::Colour(0xff797ee8),
        warm=juce::Colour(0xfff6a6c1);
    const auto cyan=cool.interpolatedWith(mid,hot*.55f);
    const auto rose=mid.interpolatedWith(warm,hot);
    const auto tube=[&](juce::Rectangle<float> r,float radius,bool vertical,bool reverse) {
        juce::Graphics::ScopedSaveState saved(g);
        juce::Path clip;clip.addRoundedRectangle(r,radius);
        g.reduceClipRegion(clip);
        const auto a=reverse?rose:cyan,b=reverse?cyan:rose;
        const float alpha=std::clamp(.12f+.78f*energy+.08f*state.flash,0.f,.94f);
        g.setGradientFill(juce::ColourGradient(a.withAlpha(alpha),
            r.getX(),r.getY(),b.withAlpha(alpha*.87f),vertical?r.getX():r.getRight(),vertical?r.getBottom():r.getY(),false));
        g.fillRect(r);
        for(int track=0;track<3;++track) {
            juce::Path current;
            const float phase=state.phase+track*1.83f,amplitude=std::min((vertical?r.getWidth():r.getHeight())*.30f,
                2.f+12.f*state.bass+6.f*state.width);
            constexpr int segments=80;
            for(int i=0;i<=segments;++i) {
                const float u=(float)i/segments;
                const float wave=std::sin(u*(8.f+state.width*9.f)+phase)
                    +.32f*std::sin(u*19.f-phase*.4f);
                const float offset=wave*amplitude+(track-1)*(vertical?4.f:3.f);
                const float x=vertical?r.getCentreX()+offset:r.getX()+u*r.getWidth();
                const float y=vertical?r.getY()+u*r.getHeight():r.getCentreY()+offset;
                if(i==0)current.startNewSubPath(x,y);else current.lineTo(x,y);
            }
            const auto ink=(track==1?juce::Colour(0xffe2f4ff):a);
            g.setColour(ink.withAlpha(std::min(.70f,energy*(track==1?.60f:.35f)+state.flash*.20f)));
            g.strokePath(current,juce::PathStrokeType(track==1?3.0f:5.0f,juce::PathStrokeType::curved,
                        juce::PathStrokeType::rounded));
        }
        g.setColour(juce::Colours::white.withAlpha(.08f+.18f*energy));
        if(vertical)g.drawLine(r.getX()+r.getWidth()*.26f,r.getY()+10.f,r.getX()+r.getWidth()*.26f,r.getBottom()-10.f,1.3f);
        else g.drawLine(r.getX()+12.f,r.getY()+r.getHeight()*.24f,r.getRight()-12.f,r.getY()+r.getHeight()*.24f,1.1f);
    };
    tube({116.f,9.f,(float)width-232.f,30.f},15.f,false,false);
    tube({23.f,102.f,41.f,(float)height-204.f},20.f,true,false);
    tube({(float)width-64.f,102.f,41.f,(float)height-204.f},20.f,true,true);
    tube({116.f,(float)height-28.f,(float)width-232.f,9.f},4.5f,false,true);
}

} // namespace surgery::juke
